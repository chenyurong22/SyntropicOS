# SyntropicOS Stepper Motor Motion Control STM32 HAL Example

Demonstrates non-blocking stepper motor acceleration & deceleration ramping (`SYN_Stepper`), step/direction pulse generation, absolute position targeting (`syn_stepper_move_to`), and driver enable pin control (`syn_stepper_set_enable_pin`) using STM32 HAL Timer interrupts or high-frequency tick loops (`TIMx->ISR`).

## Architecture & Features

- **Trapezoidal Speed Ramping**: Non-blocking state machine (`SYN_STEPPER_ACCEL`, `SYN_STEPPER_CRUISE`, `SYN_STEPPER_DECEL`) for smooth motor acceleration and deceleration.
- **Configurable Speed & Acceleration**: Configures maximum steps per second (`max_sps`, e.g. 2000 SPS) and acceleration rate (`accel_sps2`, e.g. 1000 $\text{SPS}^2$) via `syn_stepper_set_speed`.
- **Absolute & Relative Movement**: Supports relative step moves (`syn_stepper_move`) and absolute coordinate moves (`syn_stepper_move_to`).
- **Driver Enable Control**: Active-low driver EN control (`syn_stepper_enable`) to disable motor coils during idle states and reduce power dissipation.
- **Hardware Drivers**: Compatible with standard STEP/DIR drivers (A4988, DRV8825, TMC2208, TMC2209).

## Hardware Wiring

```
+--------------------+                    +---------------------+
|  STM32 Micro       |                    |  Stepper Driver     |
|  (e.g., STM32F4)   |                    |  (A4988 / TMC2209)  |
|                    |                    |                     |
|  GPIO_STEP (PA0)  ---------------------> STEP (Step Pulse)    |
|  GPIO_DIR  (PA1)  ---------------------> DIR  (Direction)     |
|  GPIO_EN   (PA2)  ---------------------> EN   (Enable Bar)    |
|                    |                    |                     |
|                GND ---------------------> GND                 |
|               3.3V ---------------------> VDD                 |
+--------------------+                    +---------------------+
```

## Motion Parameters

- **Max Speed**: 2,000 Steps / Sec (SPS).
- **Acceleration**: 1,000 Steps / $\text{Sec}^2$ ($\text{SPS}^2$).
- **Microstepping**: 1/16 Microstepping (3,200 steps per 360-degree motor revolution).
- **Timer Interrupt Rate**: 10kHz (100$\mu$s tick interval).
