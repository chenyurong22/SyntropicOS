# SyntropicOS Standard LED & WS2812B Smart LED STM32 HAL Example

Demonstrates GPIO status LED control (`SYN_LED`), WS2812B / SK6812 Smart Addressable RGB LED strip animation (`SYN_SmartLED`), HSV rainbow color cycling (`syn_smartled_set_pixel_hsv`), and global master brightness scaling (`syn_smartled_set_brightness`) using STM32 HAL drivers (`HAL_TIM_PWM_Start_DMA` / SPI / GPIO).

## Architecture & Features

- **Standard Status LED**: Non-blocking LED toggling, pulse patterns, and heartbeats (`syn_led_toggle`, `syn_led_blink`).
- **Smart LED Strip (WS2812B / Neopixel)**: 8-pixel addressable RGB LED strip controller supporting GRB / RGB color ordering (`SYN_SMARTLED_ORDER_GRB`).
- **HSV Color Wheel Animation**: Generates smooth 360-degree rainbow color wheel transitions (`syn_smartled_set_pixel_hsv`) across all addressable pixels.
- **Master Brightness Scaling**: Applies non-destructive master brightness limiting (0..255) to prevent power supply overload (`syn_smartled_set_brightness`).

## Hardware Wiring

```
+--------------------+                    +---------------------+
|  STM32 Micro       |                    |  WS2812B RGB Strip  |
|  (e.g., STM32F4)   |                    |  & Status LED       |
|                    |                    |                     |
|  GPIO_STATUS (PA0) ---------------------> Standard Status LED |
|  TIM1_CH1    (PA8) ---------------------> DIN (Data In)       |
|                    |                    |                     |
|                GND ---------------------> GND                 |
|               5.0V ---------------------> 5V Power Supply     |
+--------------------+                    +---------------------+
```

## Protocol & Timing Specifications

- **WS2812B Bit Timing**: 800kHz NRZ single-wire data stream ($T_{\text{BIT}} = 1.25\mu\text{s}$, $T_{0\text{H}} = 0.4\mu\text{s}$, $T_{1\text{H}} = 0.8\mu\text{s}$).
- **Pixel Count**: 8 addressable RGB LEDs (24 bytes RGB buffer).
- **Refresh Rate**: 50Hz (20ms animation frame update).
- **Status LED Blink Rate**: 1Hz heartbeat (500ms ON / 500ms OFF).
