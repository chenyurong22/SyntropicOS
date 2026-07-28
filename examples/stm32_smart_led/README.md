# SyntropicOS WS2812B Smart Addressable RGB LED STM32 HAL Example

Demonstrates WS2812B / SK6812 / Neopixel addressable RGB LED strip control (`SYN_SmartLED`), HSV rainbow color wheel animation (`syn_smartled_set_pixel_hsv`), individual pixel RGB setting (`syn_smartled_set_pixel_rgb`), and global master brightness scaling (`syn_smartled_set_brightness`) using STM32 HAL drivers (`HAL_TIM_PWM_Start_DMA` / SPI).

## Architecture & Features

- **Smart LED Strip (WS2812B / Neopixel)**: 8-pixel addressable RGB LED strip controller supporting GRB / RGB color ordering (`SYN_SMARTLED_ORDER_GRB`).
- **HSV Color Wheel Animation**: Generates smooth 360-degree rainbow color wheel transitions (`syn_smartled_set_pixel_hsv`) across all addressable pixels.
- **Master Brightness Scaling**: Applies non-destructive master brightness limiting (0..255) to prevent power supply overload (`syn_smartled_set_brightness`).
- **Color Operations**: Includes whole-strip color fills (`syn_smartled_fill_rgb`) and strip clearing (`syn_smartled_clear`).

## Hardware Wiring

```
+--------------------+                    +---------------------+
|  STM32 Micro       |                    |  WS2812B RGB Strip  |
|  (e.g., STM32F4)   |                    |                     |
|                    |                    |                     |
|  TIM1_CH1    (PA8) ---------------------> DIN (Data In)       |
|                GND ---------------------> GND                 |
|               5.0V ---------------------> 5V Power Supply     |
+--------------------+                    +---------------------+
```

## Protocol & Timing Specifications

- **WS2812B Bit Timing**: 800kHz NRZ single-wire data stream ($T_{\text{BIT}} = 1.25\mu\text{s}$, $T_{0\text{H}} = 0.4\mu\text{s}$, $T_{1\text{H}} = 0.8\mu\text{s}$).
- **Pixel Count**: 8 addressable RGB LEDs (24 bytes RGB buffer).
- **Refresh Rate**: 50Hz (20ms animation frame update).
