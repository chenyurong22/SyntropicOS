/**
 * @file main.c
 * @brief SyntropicOS Standard LED & WS2812B Smart LED STM32 HAL Example.
 *
 * Demonstrates non-blocking GPIO status LED heartbeat (`syn_led_toggle`),
 * WS2812B addressable RGB LED strip pixel buffer setup (`syn_smartled_init`),
 * HSV color wheel animation (`syn_smartled_set_pixel_hsv`), and master brightness
 * control (`syn_smartled_set_brightness`) using STM32 HAL drivers.
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Hardware Pin Definitions */
#define STATUS_LED_PORT GPIOA
#define STATUS_LED_PIN  GPIO_PIN_0

#define SMARTLED_DATA_PORT GPIOA
#define SMARTLED_DATA_PIN  GPIO_PIN_8

#define NUM_SMART_LEDS 8

/* SyntropicOS Driver Instances */
static SYN_LED status_led;
static SYN_SmartLED smart_led_strip;
static SYN_SmartLEDColor pixel_buffer[NUM_SMART_LEDS];

/* Application Rainbow Animation State */
typedef struct {
    uint8_t  hue_offset;
    uint8_t  master_brightness;
    uint32_t last_led_toggle_tick;
    uint32_t last_anim_tick;
} Animation_State;

static Animation_State anim = {
    .hue_offset = 0,
    .master_brightness = 128, /* 50% Master Brightness */
    .last_led_toggle_tick = 0,
    .last_anim_tick = 0
};

/**
 * @brief Transmit 24-bit RGB pixel buffer out to WS2812B LED strip.
 *
 * Transmits pixel color data using PWM DMA or SPI shift register.
 */
static void flush_smartled_hardware_buffer(const SYN_SmartLED *strip)
{
    (void)strip;
    /* Hardware DMA / PWM / SPI transfer stub for WS2812B data pin PA8 */
}

/**
 * @brief Initialize Standard LED and WS2812B Smart LED Strip.
 */
void smartled_app_init(void)
{
    /* 1. Initialize Standard Status GPIO LED (Active High) */
    syn_led_init(&status_led, (SYN_GPIO_Pin)STATUS_LED_PIN, SYN_LED_ACTIVE_HIGH);

    /* 2. Initialize 8-Pixel WS2812B Smart LED Strip (GRB Color Order) */
    syn_smartled_init(&smart_led_strip, (SYN_GPIO_Pin)SMARTLED_DATA_PIN,
                      NUM_SMART_LEDS, pixel_buffer, SYN_SMARTLED_ORDER_GRB);

    /* Set 50% Master Brightness */
    syn_smartled_set_brightness(&smart_led_strip, anim.master_brightness);

    /* Clear strip to black */
    syn_smartled_clear(&smart_led_strip);
    flush_smartled_hardware_buffer(&smart_led_strip);
}

/**
 * @brief Periodic 20ms Task (50Hz frame rate for Smart LED & 1Hz Status LED heartbeat).
 */
void smartled_app_task_20ms(void)
{
    uint32_t now = syn_port_get_tick_ms();

    /* 1. Status LED Heartbeat (Toggle every 500ms -> 1Hz) */
    if ((now - anim.last_led_toggle_tick) >= 500U) {
        anim.last_led_toggle_tick = now;
        syn_led_toggle(&status_led);

        /* Write GPIO physical output state */
        HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN,
                          status_led.is_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }

    /* 2. WS2812B HSV Rainbow Animation Frame (50Hz) */
    if ((now - anim.last_anim_tick) >= 20U) {
        anim.last_anim_tick = now;

        /* Increment base hue angle */
        anim.hue_offset++;

        /* Render HSV rainbow gradient across 8 pixels */
        for (uint16_t i = 0; i < NUM_SMART_LEDS; i++) {
            uint8_t pixel_hue = (uint8_t)(anim.hue_offset + (i * 255 / NUM_SMART_LEDS));

            /* Set pixel color using Hue (0..255), Full Saturation (255), Full Value (255) */
            syn_smartled_set_pixel_hsv(&smart_led_strip, i, pixel_hue, 255, 255);
        }

        /* Flush output pixel data to physical WS2812B LEDs */
        flush_smartled_hardware_buffer(&smart_led_strip);
    }
}
