# SyntropicOS Multi-Channel Software PWM STM32 HAL Example

Demonstrates software PWM signal generation on standard GPIO pins (`SYN_SoftPWM`), 100-step resolution duty cycle adjustments (`syn_soft_pwm_set_percent`), multi-channel array servicing (`syn_soft_pwm_service`), and smooth RGB LED breathing / DC motor speed control using STM32 HAL Timer interrupts (`TIMx->ISR`).

## Architecture & Features

- **Any-GPIO Software PWM**: Generates high-frequency pulse-width modulated outputs on non-timer GPIO pins without requiring hardware PWM channels.
- **Multi-Channel Array Servicing**: Services an array of independent PWM channels (e.g. 4 channels for RGBW LEDs) in a single high-frequency timer interrupt pass (`syn_soft_pwm_service`).
- **Configurable Frequency & Resolution**: Resolution set to 100 steps (0..100% duty cycle). Driven by a 10kHz hardware timer interrupt (`TIM2`), producing a 100Hz flicker-free PWM output frequency ($f_{\text{PWM}} = \frac{10\text{kHz}}{100} = 100\text{Hz}$).
- **Breathing / Fading Ramp Engine**: Implements smooth cosine/linear duty cycle transitions for LED dimming.

## Hardware Wiring

```
+--------------------+                    +---------------------+
|  STM32 Micro       |                    |  RGBW LED / Load    |
|  (e.g., STM32F4)   |                    |  (N-Channel MOSFETs)|
|                    |                    |                     |
|   GPIO_RED   (PA0) ---------------------> Red Channel MOSFET  |
|   GPIO_GREEN (PA1) ---------------------> Green Channel MOSFET|
|   GPIO_BLUE  (PA2) ---------------------> Blue Channel MOSFET |
|   GPIO_WHITE (PA3) ---------------------> White Channel MOSFET|
|                    |                    |                     |
|                GND ---------------------> Ground              |
+--------------------+                    +---------------------+
```

## Timing Specifications

- **Timer ISR Frequency**: 10kHz (100$\mu$s interrupt period).
- **PWM Period Resolution**: 100 steps (1% duty step resolution).
- **Output PWM Frequency**: 100Hz ($10\text{ms}$ period).
- **CPU Overhead**: $< 1.5\%$ at 168MHz ARM Cortex-M4 core clock.
