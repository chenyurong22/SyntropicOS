# SyntropicOS Rotary Encoder & Push Button STM32 HAL Example

Demonstrates quadrature rotary encoder decoding (`SYN_Encoder`), push-button debouncing (`SYN_Button`), multi-click gesture detection, and menu navigation using STM32 HAL GPIO / TIM hardware interfaces.

## Architecture & Features

- **Quadrature Encoder Decoding**: Non-blocking quadrature phase state machine (`syn_encoder_update`) handling Clockwise (CW) and Counter-Clockwise (CCW) detent tracking, position accumulation (`syn_encoder_position`), and step filtering (`syn_encoder_set_steps_per_detent`).
- **Debounced Push Button**: 10ms software debouncing state machine (`syn_button_update`) with single-click, double-click, long-press, and press-and-hold event detection (`syn_button_was_clicked`).
- **Menu & Value Control**: Controls menu selection index and numerical parameter adjustment via encoder rotation and push-button confirmation.
- **Hardware Interfacing**: Interfaces with standard EC11 rotary encoder modules (Pin A, Pin B, and Switch SW pins) connected to STM32 GPIO pins or Timer Encoder Interface (`TIMx->CNT`).

## Hardware Wiring

```
+--------------------+                    +---------------------+
|  STM32 Micro       |                    |  Rotary Encoder     |
|  (e.g., STM32F4)   |                    |  (e.g., EC11)       |
|                    |                    |                     |
|   GPIO_ENC_A (PA0) <-------------------- Phase A (CLK)        |
|   GPIO_ENC_B (PA1) <-------------------- Phase B (DT)         |
|   GPIO_ENC_SW(PA2) <-------------------- Switch SW (Button)   |
|                    |                    |                     |
|                GND ---------------------> GND                 |
|               3.3V ---------------------> VCC (10k pullups)   |
+--------------------+                    +---------------------+
```

## Protocol & Timing Specifications

- **Encoder Resolution**: 4 quadrature state transitions per detent (or 1–4 steps configurable).
- **Sampling Rate**: 1kHz (1ms timer interrupt or scheduler pass) for `syn_encoder_update` and `syn_button_update`.
- **Debounce Window**: 10ms for push-button noise filtering.
