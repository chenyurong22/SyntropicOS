# SyntropicOS Capacitive Touch Sensing STM32 HAL Example

Demonstrates non-blocking capacitive touch sensing (`SYN_Touch`), baseline environmental noise calibration (`syn_touch_calibrate`), hysteresis filtering, and touch key / slider detection using STM32 HAL ADC or Touch Sensing Controller (TSC) hardware drivers.

## Architecture & Features

- **Capacitive Touch Engine**: Tracks raw charge transfer or ADC sampling values against a dynamic environmental baseline (`baseline`).
- **Baseline Calibration**: Automatically tracks environmental temperature and humidity shifts to prevent false touch triggers (`syn_touch_calibrate`).
- **Hysteresis Noise Suppression**: Implements threshold delta and release hysteresis (`hysteresis`) to ensure clean touch press and release transitions (`syn_touch_is_pressed`).
- **Multi-Button Keypad / Slider**: Samples 4 capacitive touch pads (Keys 1..4) and calculates touch position.
- **Hardware Interfacing**: Interfaces with discrete capacitive touch electrodes (copper PCB pads, glass overlay) connected to STM32 ADC channels (PA0..PA3) or digital touch ICs (TTP223, AT42QT1010).

## Hardware Wiring

```
+--------------------+                    +---------------------+
|  STM32 Micro       |                    |  Capacitive Touch   |
|  (e.g., STM32F4)   |                    |  Pads / Overlay     |
|                    |                    |                     |
|  ADC1_IN0 (PA0)   ---------------------> Touch Pad 1 (Key 1)  |
|  ADC1_IN1 (PA1)   ---------------------> Touch Pad 2 (Key 2)  |
|  ADC1_IN2 (PA2)   ---------------------> Touch Pad 3 (Key 3)  |
|  ADC1_IN3 (PA3)   ---------------------> Touch Pad 4 (Key 4)  |
|                    |                    |                     |
|  GPIO_LED (PB0)   ---------------------> Touch Indicator LED  |
|                GND ---------------------> Ground Plane / GND  |
+--------------------+                    +---------------------+
```

## Sensor Specifications

- **Baseline Sampling**: 100 raw ADC counts (nominal un-touched level).
- **Touch Delta Threshold**: 30 counts above baseline ($\Delta \ge 30$).
- **Hysteresis**: 5 counts release buffer.
- **Sampling Interval**: 10ms (100Hz scan rate).
