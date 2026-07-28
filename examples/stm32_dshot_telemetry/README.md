# STM32 Bidirectional DShot (BDShot) Motor Telemetry Example (`syn_dshot_telemetry`)

This example demonstrates how to **decode 20-bit GCR Bidirectional DShot (BDShot) telemetry frames** from ESCs to measure real-time motor eRPM, mechanical RPM, and commutation period using an **STM32 microcontroller** and SyntropicOS **`syn_dshot_telemetry`**.

---

## Overview

Bidirectional DShot (BDShot) enables the ESC to transmit motor rotation speed telemetry back to the microcontroller over the same signal wire used for motor control.

### Features
- **20-bit GCR (Group Code Recording)**: 5b/4b nibble decoding with invalid symbol detection.
- **4-Bit CRC Checksum**: Validates frame integrity before calculating RPM.
- **Floating-Point Free RPM Calculation**: Decodes 12-bit floating period format (9-bit mantissa, 3-bit exponent) into $\text{eRPM}$ and mechanical $\text{RPM}$.
- **Configurable Motor Pole Pairs**: Custom motor pole count support (e.g. 14 poles = 7 pole pairs).

---

## Hardware Configuration & Wiring

Connect the bidirectional DShot signal pin (e.g. PB0 / TIM3 CH3) on the STM32 to the ESC signal line:

| STM32 Pin | ESC Pin | Description |
|---|---|---|
| **PB0** | **ESC Signal** | Bidirectional DShot TX/RX Line (TIM DMA / Input Capture) |
| **GND** | **ESC Ground** | Common Ground Reference |

---

## BDShot Telemetry Frame Architecture

$$\text{Decoded Payload (16 bits)} = \underbrace{\text{eRPM Period Mantissa [11:3] \;\Vert\; Exponent [2:0]}}_{12\text{ bits}} \;\Vert\; \underbrace{\text{CRC [3:0]}}_{4\text{ bits}}$$

$$\text{eRPM} = \frac{60000000}{\text{period}_{\mu\text{s}}}, \qquad \text{RPM} = \frac{\text{eRPM}}{\text{pole\_pairs}}$$

---

## Code Example

```c
#include "syntropic/syntropic.h"

uint32_t raw_gcr_20bit = 0xAA555; /* Captured 20-bit GCR telemetry frame */
uint8_t pole_pairs = 7;           /* 14-pole motor */
SYN_DShot_Telemetry telemetry;

if (syn_dshot_parse_telemetry(raw_gcr_20bit, pole_pairs, &telemetry) == SYN_OK) {
    if (telemetry.valid) {
        uint32_t erpm      = telemetry.erpm;      /* Electrical RPM */
        uint32_t rpm       = telemetry.rpm;       /* Mechanical RPM */
        uint32_t period_us = telemetry.period_us; /* Commutation period (us) */
        (void)erpm; (void)rpm; (void)period_us;
    }
}
```

---

## Output Preview

```text
[BDShot Decoder] Frame Received (GCR: 0xAA555)
  CRC Status:      VALID
  Commutation:     450 us
  Electrical eRPM: 133333 eRPM
  Motor Speed:     19047 RPM (7 Pole Pairs / 14 Poles)
```
