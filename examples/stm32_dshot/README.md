# STM32 DShot Digital ESC & Telemetry Example (`syn_dshot`)

This example demonstrates how to **encode 16-bit DShot (DShot150/300/600)** digital motor commands and decode **20-bit Bidirectional DShot (BDShot)** telemetry from Brushless ESCs using an **STM32 microcontroller** and SyntropicOS **`syn_dshot`**.

---

## Overview

DShot is a high-speed digital communication protocol used to control Brushless DC (BLDC) motor ESCs in drones, robotics, and high-performance motion control. Unlike traditional PWM ($50 - 400\,\text{Hz}$), DShot transmits digital packets with 4-bit CRC protection and zero pulse-width jitter.

### Features
- **Speed Support**: DShot150 ($150\,\text{kbit/s}$), DShot300 ($300\,\text{kbit/s}$), DShot600 ($600\,\text{kbit/s}$).
- **11-Bit Throttle Precision**: 2048 throttle levels ($0 = \text{Disarm}$, $1..47 = \text{Special Commands}$, $48..2047 = \text{Throttle}$).
- **4-Bit CRC Protection**: Nibble-XOR checksum appended to every frame.
- **Bidirectional DShot (BDShot)**: Decodes 20-bit GCR (Group Code Recording) telemetry packets for live motor eRPM and mechanical RPM feedback.

---

## Hardware Configuration & Wiring

### ESC Connection
Connect the STM32 Timer DMA pin to the ESC signal input:

| Microcontroller Pin | ESC Connection | Signal |
|---|---|---|
| **PB0** | **ESC Signal Pin** | TIM3 CH3 PWM/DMA Digital Signal Wire |
| **GND** | **ESC Signal Ground** | Ground Reference |

---

## DShot Frame Architecture

$$\text{Frame (16 bits)} = \underbrace{\text{Throttle [15:5]}}_{11\text{ bits}} \;\Vert\; \underbrace{\text{Telemetry Req [4]}}_{1\text{ bit}} \;\Vert\; \underbrace{\text{CRC [3:0]}}_{4\text{ bits}}$$

```c
#include "syntropic/syntropic.h"

SYN_DShot_Packet pkt;
uint16_t throttle = 1000; /* 0..2047 */
bool req_telemetry = false;

if (syn_dshot_encode(throttle, req_telemetry, &pkt) == SYN_OK) {
    /* pkt.raw_frame contains the 16-bit frame ready for TIM DMA transfer */
    send_dshot_dma(pkt.raw_frame);
}
```

---

## Output Preview

```text
[DShot ESC] Encoding DShot600 Frame: Throttle=1000 (48.8%), Telemetry=0, CRC=0x9 -> Raw=0x7D09
[BDShot RX] GCR Telemetry Frame Decoded:
  Period: 450 us
  eRPM:   133333
  RPM:    19047 (7 Pole Pairs)
```
