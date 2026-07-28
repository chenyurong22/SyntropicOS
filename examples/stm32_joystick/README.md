# SyntropicOS Dual-Axis Analog Joystick STM32 HAL Example

Demonstrates dual-axis analog joystick ADC sampling (`SYN_Joystick`), deadband noise filtering, normalized percentage scaling (-100% to +100%), 8-way D-pad directional classification, and integrated push-button handling using STM32 HAL ADC drivers (`HAL_ADC_...`).

## Architecture & Features

- **Dual-Axis ADC Ingestion**: Samples X-axis (VRX) and Y-axis (VRY) analog voltages using STM32 12-bit ADC channels (`HAL_ADC_PollForConversion` / DMA).
- **Deadband & Center Calibration**: Configures zero-point center offset (2048 raw ADC value) and noise deadband threshold (`syn_joystick_init`).
- **Normalized Percentages**: Converts raw 12-bit ADC readings (0..4095) to signed percentages (-100% to +100% X/Y) via `syn_joystick_get_x_pct` and `syn_joystick_get_y_pct`.
- **8-Way Directional Decoding**: Classifies analog stick deflection into 8 discrete directions (`CENTER`, `UP`, `UP_RIGHT`, `RIGHT`, `DOWN_RIGHT`, `DOWN`, `DOWN_LEFT`, `LEFT`, `UP_LEFT`).
- **Push-Button Z-Axis**: Debounces integrated push-button switch (SW).

## Hardware Wiring

```
+--------------------+                    +---------------------+
|  STM32 Micro       |                    |  Analog Joystick    |
|  (e.g., STM32F4)   |                    |  (e.g., HW-504)     |
|                    |                    |                     |
|  ADC1_IN0 (PA0)   <-------------------- VRX (X-Axis Analog)  |
|  ADC1_IN1 (PA1)   <-------------------- VRY (Y-Axis Analog)  |
|  GPIO_SW  (PA2)   <-------------------- SW  (Button Switch)   |
|                    |                    |                     |
|                GND ---------------------> GND                 |
|               3.3V ---------------------> VCC                 |
+--------------------+                    +---------------------+
```

## Protocol & ADC Specifications

- **ADC Resolution**: 12-bit (0..4095 raw count, 0V..3.3V full-scale).
- **Center Point**: 2048 (1.65V nominal center).
- **Deadband Radius**: 150 counts (~3.6% full scale).
- **Sampling Interval**: 20ms (50Hz polling rate).
