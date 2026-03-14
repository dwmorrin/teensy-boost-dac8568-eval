#include <Arduino.h>
#include <SPI.h>

// PINS
const int pinLDAC = 9;
const int pinSYNC = 10;
// MOSI (11) and SCLK (13) are handled by the hardware SPI library

// Protocol Headers
const byte HEADER_DAC_WRITE = 0xB5;
const byte HEADER_LDAC_SYNC = 0xC5;
const byte HEADER_DONE      = 0xAA;

void setup() {
  pinMode(pinLDAC, OUTPUT);
  pinMode(pinSYNC, OUTPUT);

  // Default states: Active-low pins should start HIGH
  digitalWrite(pinLDAC, HIGH);
  digitalWrite(pinSYNC, HIGH);

  // Initialize USB Serial
  Serial.begin(9600); 

  // Initialize SPI
  SPI.begin();
  // Mode 1 (CPOL=0, CPHA=1) is required for the DAC8568
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE1));

  // Power up the internal 2.5V reference
  // Command: 0x08000001 
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
  // Wait until we have a full 5-byte packet in the buffer
  if (Serial.available() >= 5) {
    byte header = Serial.read();

    // If the header is invalid, discard it and wait for the next byte
    if (header != HEADER_DAC_WRITE && header != HEADER_LDAC_SYNC) {
      return; 
    }

    // Read the remaining 4 bytes of the payload
    byte payload[4];
    Serial.readBytes(reinterpret_cast<char*>(payload), 4);

    if (header == HEADER_DAC_WRITE) {
      // Packet: [0xB5] [Channel] [MSB] [LSB] [Padding]
      byte channel = payload[0];
      uint16_t value = (payload[1] << 8) | payload[2];
      
      if (channel < 8) {
        writeDACRegister(channel, value);
      }
    } 
    else if (header == HEADER_LDAC_SYNC) {
      // Packet: [0xC5] [Padding x4]
      triggerLDAC();
    }

    // Acknowledge receipt and execution
    // Send [0xAA] followed by the exact payload we just received
    Serial.write(HEADER_DONE);
    Serial.write(payload, 4);
  }
}