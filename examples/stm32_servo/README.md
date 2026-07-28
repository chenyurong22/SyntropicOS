# SyntropicOS RC Servo Motor STM32 HAL Example

Demonstrates RC servo motor pulse-width positioning (`SYN_Servo`), 180-degree angle-to-pulse conversion (`syn_servo_set_angle`), smooth timed angular movement ramping (`syn_servo_move_to`), and 50Hz PWM signal updates using STM32 HAL Timer PWM drivers (`HAL_TIM_PWM_Start` / `TIMx->CCR1`).

## Architecture & Features

- **Pulse Width Bounds**: Configures minimum pulse width ($1000\mu\text{s}$ for $0^\circ$), maximum pulse width ($2000\mu\text{s}$ for $180^\circ$), and angular travel range ($180^\circ$) via `syn_servo_init`.
- **Smooth Timed Movement Ramping**: Interpolates target angle over a specified duration in milliseconds (e.g. move to $180^\circ$ over 1000ms) to eliminate mechanical jerk (`syn_servo_move_to`).
- **50Hz PWM Output**: Converts calculated pulse width in microseconds directly into Timer Compare Register CCR ticks (`TIM2->CCR1`).
- **Target Status Verification**: Queries target angular positioning (`syn_servo_at_target`).

## Hardware Wiring

```
+--------------------+                    +---------------------+
|  STM32 Micro       |                    |  RC Servo Motor     |
|  (e.g., STM32F4)   |                    |  (SG90 / MG996R)    |
|                    |                    |                     |
|  TIM2_CH1    (PA0) ---------------------> PWM Signal (Yellow) |
|               5.0V ---------------------> VCC Power (Red)     |
|                GND ---------------------> GND (Brown/Black)   |
+--------------------+                    +---------------------+
```

## Protocol & Timing Specifications

- **PWM Frequency**: 50Hz ($20\text{ms}$ frame period).
- **Pulse Width Range**: $1000\mu\text{s}$ ($0^\circ$) to $2000\mu\text{s}$ ($180^\circ$).
- **Center Position**: $1500\mu\text{s}$ ($90^\circ$).
- **Update Rate**: 50Hz (20ms animation frame update).
