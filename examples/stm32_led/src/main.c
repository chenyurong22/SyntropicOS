/**
 * @file main.c
 * @brief SyntropicOS Standard GPIO LED STM32 HAL Example.
 *
 * Demonstrates non-blocking status LED control (`syn_led_init`), heartbeat toggling (`syn_led_toggle`),
 * timed blinking (`syn_led_blink`), and diagnostic error flash patterns using STM32 HAL GPIO drivers.
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Pin Definitions */
#define LED_HEARTBEAT_PORT GPIOA
#define LED_HEARTBEAT_PIN  GPIO_PIN_0

#define LED_ERROR_PORT GPIOA
#define LED_ERROR_PIN  GPIO_PIN_1

/* SyntropicOS LED Handles */
static SYN_LED heartbeat_led;
static SYN_LED error_led;

/* LED State Machine Parameters */
typedef struct {
    uint32_t last_heartbeat_tick;
    uint32_t last_error_tick;
    uint8_t  error_flash_count;
} LED_AppState;

static LED_AppState led_app = {0};

/**
 * @brief Initialize Standard GPIO Status LEDs.
 */
void led_app_init(void)
{
    /* Initialize Heartbeat LED on PA0 (Active High) */
    syn_led_init(&heartbeat_led, (SYN_GPIO_Pin)LED_HEARTBEAT_PIN, SYN_LED_ACTIVE_HIGH);

    /* Initialize Error LED on PA1 (Active High) */
    syn_led_init(&error_led, (SYN_GPIO_Pin)LED_ERROR_PIN, SYN_LED_ACTIVE_HIGH);
}

/**
 * @brief Periodic 10ms Task for Status LED State Updates.
 */
void led_app_task_10ms(void)
{
    uint32_t now = syn_port_get_tick_ms();

    /* 1. Heartbeat LED: Toggle every 500ms (1Hz) */
    if ((now - led_app.last_heartbeat_tick) >= 500U) {
        led_app.last_heartbeat_tick = now;
        syn_led_toggle(&heartbeat_led);

        HAL_GPIO_WritePin(LED_HEARTBEAT_PORT, LED_HEARTBEAT_PIN,
                          syn_led_is_on(&heartbeat_led) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }

    /* 2. Error Diagnostic LED: 3 rapid flashes (100ms ON / 100ms OFF) every 2000ms */
    if ((now - led_app.last_error_tick) >= 100U) {
        led_app.last_error_tick = now;

        if (led_app.error_flash_count < 6) { /* 3 cycles = 6 toggles */
            syn_led_toggle(&error_led);
            led_app.error_flash_count++;
        } else if ((now - led_app.last_error_tick) >= 2000U) {
            led_app.error_flash_count = 0; /* Reset pattern sequence */
        }

        HAL_GPIO_WritePin(LED_ERROR_PORT, LED_ERROR_PIN,
                          syn_led_is_on(&error_led) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}
