#include <SPI.h>

// Pin definitions
const int pinLDAC = 9;
const int pinSYNC = 10;
// MOSI (11) and SCLK (13) are handled by the hardware SPI library

void setup() {
  pinMode(pinLDAC, OUTPUT);
  pinMode(pinSYNC, OUTPUT);

  // Default states: Active-low pins should start HIGH
  digitalWrite(pinLDAC, HIGH);
  digitalWrite(pinSYNC, HIGH);

  // Initialize SPI
  SPI.begin();
  // 2MHz is plenty fast for testing. Mode 1 is required for this DAC.
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE1));

  // 1. Power up the internal reference so the DAC actually outputs voltage
  // Command: 0x08000001 
  // Breakdown: Prefix(0000) | Control(1000) | Address(0000) | Feature Data(...0001)
  digitalWrite(pinSYNC, LOW);
  SPI.transfer(0x08);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.transfer(0x01);
  digitalWrite(pinSYNC, HIGH);
  
  delay(10); // Give the internal reference a moment to settle
}

// Function to write to the input register WITHOUT updating the analog output
void writeDACRegister(byte channel, uint16_t value) {
  // Control 0000 = Write to input register n (Waits for ~LDAC to update)
  byte byte1 = 0x00; // Prefix (0000) + Control (0000)
  byte byte2 = (channel << 4) | (value >> 12); // Addr + Top 4 bits of value
  byte byte3 = (value >> 4) & 0xFF; // Middle 8 bits of value
  byte byte4 = (value << 4) & 0xF0; // Bottom 4 bits of value + Feature (0000)

  digitalWrite(pinSYNC, LOW);
  SPI.transfer(byte1);
  SPI.transfer(byte2);
  SPI.transfer(byte3);
  SPI.transfer(byte4);
  digitalWrite(pinSYNC, HIGH);
}

// Function to manually pull the trigger and update the analog outputs
void triggerLDAC() {
  digitalWrite(pinLDAC, LOW);
  delayMicroseconds(1); // Brief active-low pulse
  digitalWrite(pinLDAC, HIGH);
}

void loop() {
  // Generate a slow sine wave (breathing LED effect)
  // This validates the asynchronous ~LDAC timing and tests your LFO math
  
  static uint32_t t = 0;
  float phase = (float)t / 1000.0; // Time in seconds
  
  // Calculate sine wave from 0.0 to 1.0
  float sineValue = (sin(phase) + 1.0) / 2.0; 
  
  // Scale to 16-bit DAC range (0 to 65535)
  uint16_t dacValue = (uint16_t)(sineValue * 65535.0);

  // 1. Pre-load the data into DAC Channel A (0)
  writeDACRegister(0, dacValue);
  
  // 2. Fire the hardware trigger to update the LED!
  triggerLDAC();

  t += 10;
  delay(10); // Run loop roughly 100 times per second
}