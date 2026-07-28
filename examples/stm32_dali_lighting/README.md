# STM32 DALI (IEC 62386) Lighting Control Gear Example

This example demonstrates how to implement a **DALI (Digital Addressable Lighting Interface / IEC 62386-101/102)** Control Gear (LED Dimmer Slave) on STM32 microcontrollers using SyntropicOS **`syn_dali`**.

## Key Features

1. **IEC 62386-101/102 Protocol Processing**:
   - 16-bit Master Forward Frame decoding (`syn_dali_decode_forward`).
   - 8-bit Slave Backward Frame response encoding (`syn_dali_encode_backward`).
   - Short Address (0..63), Group Address (0..15), and Broadcast matching.

2. **Direct Arc Power Control (DAPC)**:
   - Direct Arc Power level setting ($0..254$).
   - PWM Dimming Output control mapped to logarithmic or linear LED intensity curves.

3. **Standard Lighting Commands**:
   - `RECALL_MAX`, `RECALL_MIN`, `OFF`, `UP`, `DOWN`, `STEP_UP`, `STEP_DOWN`.
   - DALI Query commands (`QUERY_STATUS`, `QUERY_ACTUAL_LEVEL`, `QUERY_LAMP_FAILURE`).

## Hardware Setup
- **Board**: STM32 Nucleo / Discovery (STM32F4 / STM32F1)
- **DALI Interface Pins**: DALI RX (EXTI / Input Pin), DALI TX (Output Pin / Transceiver)
- **LED PWM Pin**: TIM2_CH1 (PA0) for LED dimming control
