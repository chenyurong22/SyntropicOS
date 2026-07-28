# STM32 Pulse-Position Modulation (PPM) RC Receiver Example (`syn_ppm`)

This example demonstrates how to **decode multi-channel RC PPM (Pulse-Position Modulation)** signals using an **STM32 microcontroller** (Timer Input Capture / EXTI) and SyntropicOS **`syn_ppm`**.

---

## Overview

PPM consolidates 4 to 12 RC receiver channels onto a single signal wire. Channels are encoded sequentially as variable-width pulse durations ($1000 - 2000\,\mu\text{s}$), delimited by short pulse separators. A long sync pulse ($> 2700\,\mu\text{s}$) marks frame boundaries.

### Features
- **Zero-Heap, Fixed-Memory**: Zero dynamic allocation, zero fragmentation.
- **Up to 12 Channels**: Support for standard RC transmitters (FlySky, FrSky, Futaba, RadioMaster).
- **Timer Input Capture Integration**: Processes pulse duration measurements in microsecond resolution ($1\,\mu\text{s}$ tick).
- **Failsafe & Range Clamping**: Clamps pulses to valid RC ranges ($750 - 2250\,\mu\text{s}$).

---

## Hardware Configuration & Wiring

### 1. PPM Receiver Wiring
Connect the single-wire PPM signal from an RC receiver (e.g., FlySky IA6B, FrSky D4R-II, RadioMaster R81) to an STM32 Timer Input Capture pin:

| RC Receiver Pin | STM32 Pin | Description |
|---|---|---|
| **PPM Output** | **PA0** | TIM2 Channel 1 Input Capture / EXTI0 pin |
| **VCC** | **5V / 3.3V** | Receiver power supply |
| **GND** | **GND** | Ground reference |

---

## Operating Modes

This example includes two execution variants selectable at compile time:

1. **Bare-Metal Loop (`main_bare.c`)**: Direct polling and processing loop with zero OS overhead (`-DUSE_BARE_LOOP`).
2. **`syn_sched` Coroutine (`main_sched.c`)**: Multi-tasking coroutine architecture using cooperative protothreads and periodic tasks.

---

## Code Example

```c
#include "syntropic/syntropic.h"

static SYN_PPM_Decoder ppm;

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        uint16_t pulse_us = (uint16_t)HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        __HAL_TIM_SET_COUNTER(htim, 0);

        if (syn_ppm_process_pulse(&ppm, pulse_us) == SYN_OK) {
            /* Full frame received! Access channel pulse widths */
            uint16_t roll     = syn_ppm_get_channel(&ppm, 0); // Ch 1
            uint16_t pitch    = syn_ppm_get_channel(&ppm, 1); // Ch 2
            uint16_t throttle = syn_ppm_get_channel(&ppm, 2); // Ch 3
            uint16_t yaw      = syn_ppm_get_channel(&ppm, 3); // Ch 4
            (void)roll; (void)pitch; (void)throttle; (void)yaw;
        }
    }
}
```

---

## Output Preview

```text
[PPM Receiver] Decoding PPM Frame #1042 (8 Channels)
  Ch 1 (Roll):     1500 us
  Ch 2 (Pitch):    1480 us
  Ch 3 (Throttle): 1050 us
  Ch 4 (Yaw):      1510 us
  Ch 5 (Aux 1):    2000 us
  Ch 6 (Aux 2):    1000 us
```
