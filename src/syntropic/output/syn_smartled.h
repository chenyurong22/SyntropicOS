/**
 * @file syn_smartled.h
 * @brief Generic Addressable RGB/RGBW Smart LED Strip Driver (WS2812B, SK6812, APA102, SK9822).
 * @ingroup syn_output
 */

#ifndef SYN_SMARTLED_H
#define SYN_SMARTLED_H

#include "../common/syn_defs.h"
#include "../port/syn_port_gpio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Smart LED Color Channel Order.
 */
typedef enum {
    SYN_SMARTLED_ORDER_GRB = 0, /**< Standard WS2812B (Green, Red, Blue) */
    SYN_SMARTLED_ORDER_RGB = 1, /**< Standard RGB */
    SYN_SMARTLED_ORDER_RGBW = 2 /**< SK6812 RGBW (4 channels) */
} SYN_SmartLEDOrder;

/**
 * @brief 24-bit RGB Color Struct.
 */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t w;
} SYN_SmartLEDColor;

/**
 * @brief Generic Smart LED Strip Instance Context.
 */
typedef struct {
    SYN_GPIO_Pin data_pin;
    SYN_SmartLEDOrder order;
    uint16_t num_leds;
    uint8_t brightness;           /**< Global brightness scale (0 to 255) */
    SYN_SmartLEDColor *pixel_buf; /**< Caller-allocated pixel buffer */
} SYN_SmartLED;

/**
 * @brief Initialize a Smart LED Strip context.
 *
 * @param led       Smart LED context.
 * @param data_pin  GPIO data pin.
 * @param num_leds  Total LED count in strip.
 * @param pixel_buf Array of SYN_SmartLEDColor structs (length = num_leds).
 * @param order     Color channel order (GRB, RGB, RGBW).
 * @return SYN_OK on success.
 */
SYN_Status syn_smartled_init(SYN_SmartLED *led, SYN_GPIO_Pin data_pin, uint16_t num_leds,
                             SYN_SmartLEDColor *pixel_buf, SYN_SmartLEDOrder order);

/**
 * @brief Set global brightness scale (0 = off, 255 = 100% full brightness).
 *
 * @param led        Smart LED context.
 * @param brightness Brightness scale (0 to 255).
 */
void syn_smartled_set_brightness(SYN_SmartLED *led, uint8_t brightness);

/**
 * @brief Set RGB color of a specific LED pixel.
 *
 * @param led   Smart LED context.
 * @param index Pixel index (0 to num_leds-1).
 * @param r     Red channel (0 to 255).
 * @param g     Green channel (0 to 255).
 * @param b     Blue channel (0 to 255).
 */
void syn_smartled_set_pixel_rgb(SYN_SmartLED *led, uint16_t index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Set HSV color of a specific LED pixel (0-255 hue, sat, val).
 *
 * @param led   Smart LED context.
 * @param index Pixel index (0 to num_leds-1).
 * @param h     Hue (0 to 255).
 * @param s     Saturation (0 to 255).
 * @param v     Value / Intensity (0 to 255).
 */
void syn_smartled_set_pixel_hsv(SYN_SmartLED *led, uint16_t index, uint8_t h, uint8_t s, uint8_t v);

/**
 * @brief Fill entire strip with a single RGB color.
 *
 * @param led Smart LED context.
 * @param r   Red channel.
 * @param g   Green channel.
 * @param b   Blue channel.
 */
void syn_smartled_fill_rgb(SYN_SmartLED *led, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Clear all pixels in strip (turn off all LEDs).
 *
 * @param led Smart LED context.
 */
void syn_smartled_clear(SYN_SmartLED *led);

#ifdef __cplusplus
}
#endif

#endif /* SYN_SMARTLED_H */
