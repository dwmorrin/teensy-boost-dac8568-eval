# BOOST-DAC8568 Evaluation (Teensy)

## Overview

This repository contains firmware and documentation for evaluating the **TI DAC8568** (8-channel, 16-bit DAC) using a **Teensy 4.0/4.1**.

The goal of this evaluation is to validate the DAC for use on the "DNA Chip" Motherboard project for two specific roles:

1.  **Static Bias Generation:** Providing stable, programmable DC bias voltages to the custom biosensor IC.
2.  **Low-Frequency Signal Generation:** Creating clean 1–10 Hz sine waves (10 mVpp) for frequency response characterization of the biosensor.

## Hardware Setup

### Wiring Connection

The BOOST-DAC8568 is a breakout board designed for TI Launchpads, but we are interfacing it via standard SPI.

**Power Note:** While the DAC8568 can accept up to 5.5V, we drive it with **3.3V** to match the Teensy logic levels without level shifting.

| BOOST-DAC8568 Pin      | Teensy 4.0 / 4.1 Pin | Function    | Notes                                                                   |
| :--------------------- | :------------------- | :---------- | :---------------------------------------------------------------------- |
| **+3.3V** (J1.1)       | **3.3V**             | Power (VDD) | Do not connect 5V if communicating with Teensy 3.3V logic.              |
| **GND** (J2.1)         | **GND**              | Ground      | Common ground is critical.                                              |
| **SCLK** (J1.7)        | **13** (SCK)         | SPI Clock   | Up to 50 MHz supported.                                                 |
| **MOSI** (J1.15)       | **11** (MOSI)        | SPI Data In |                                                                         |
| **CS** (/SYNC) (J2.12) | **10** (CS)          | Chip Select | Active Low.                                                             |
| **/CLR** (J2.14)       | **3.3V**             | Clear       | Tie High to prevent accidental reset.                                   |
| **/LDAC** (J2.15)      | **GND** (or IO)      | Load DAC    | **Tie Low** for immediate update. Connect to IO for synchronous update. |

_(Note: Pin numbers J1.x/J2.x refer to the standard BoosterPack headers. Verify with board silkscreen.)_

### The "Attenuator" Circuit (For Sine Wave Test)

To generate high-fidelity **10 mVpp** sine waves, we do **not** output 10 mV directly from the DAC (which would result in poor resolution). Instead, we generate a **1.0 Vpp** signal and divide it down.

**Breadboard Circuit:**

```
DAC Output (Ch A)  ----[ 100k Resistor ]----+-----> To Oscilloscope / DNA Chip Input
                                            |
                                      [ 1k Resistor ]
                                            |
                                           GND
```

- **Division Ratio:** ~100:1
- **DAC Generation:** 0.5V to 1.5V swing (Using ~26,000 steps).
- **Final Output:** ~5mV to 15mV swing (High resolution, low noise).

## Firmware Implementation

### Dependencies

- `SPI.h` (Standard Arduino Library)

### Critical Initialization

**WARNING:** The DAC8568 powers up with its **Internal Reference turned OFF**. You must send a specific command to enable it, otherwise, the output will remain floating or near 0V.

```cpp
// Command to Enable Internal 2.5V Reference
// Prefix: 0x8, Control: 0x0, Data: 0x0001 (Ref ON)
writeDAC(0x08000001);
```

### 32-Bit SPI Protocol

The DAC8568 requires a 32-bit transaction.
`[ Prefix (4) | Control (4) | Address (4) | Data (16) | Feature (4) ]`

_Note: The Feature bits are often just "Don't Care" or part of the data depending on the map._

## Experiments

### 1. Static Bias Test (DC Accuracy)

- **Goal:** Validate stability and accuracy of voltage output.
- **Procedure:**
  1.  Initialize DAC with Internal Ref (2.5V).
  2.  Set Channel A to Mid-Scale (`0x8000`).
  3.  **Expectation:** Measure **1.250V** on the DMM.
  4.  Set Channel A to Full-Scale (`0xFFFF`).
  5.  **Expectation:** Measure **2.500V** on the DMM.

### 2. Frequency Response Signal Gen (AC Test)

- **Goal:** Generate a 10 Hz sine wave.
- **Procedure:**
  1.  Use a lookup table or `sin()` function to calculate 16-bit values.
  2.  Update the DAC in a loop using `micros()` for timing.
  3.  **Target Frequency:** 10 Hz.
  4.  **Target Amplitude:** 10 mVpp (measured _after_ the 100:1 attenuator).
- **Verification:** Connect to scope. Verify the waveform is smooth and free of "staircase" artifacts.

## Next Steps

- [ ] Verify 3.3V logic compatibility (ensure no ringing on SPI lines).
- [ ] Measure noise floor with the 100:1 attenuator installed.
- [ ] Draft library class for project integration.
