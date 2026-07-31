/**
 * @file main.c
 * @brief SyntropicOS EC11 Rotary Encoder & Push-Button STM32 HAL Example.
 *
 * Demonstrates non-blocking quadrature rotary encoder decoding (`syn_encoder_update`),
 * push-button debouncing and click gesture detection (`syn_button_update`), and interactive
 * parameter adjustment (0..100% volume/brightness setpoint) using STM32 HAL GPIO drivers.
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Pin Definitions for EC11 Rotary Encoder and Integrated Push-Button */
#define ENC_PORT_A GPIOA
#define ENC_PIN_A  GPIO_PIN_0

#define ENC_PORT_B GPIOA
#define ENC_PIN_B  GPIO_PIN_1

#define ENC_PORT_SW GPIOA
#define ENC_PIN_SW  GPIO_PIN_2

/* Application UI State */
typedef struct {
    int32_t setpoint_value; /* Active parameter value (0..100) */
    uint8_t menu_mode;      /* 0 = Parameter Selection, 1 = Value Editing */
    bool    system_on;      /* Power state toggled via long press */
} App_UIState;

static App_UIState ui_state = {
    .setpoint_value = 50,
    .menu_mode = 0,
    .system_on = true
};

/* SyntropicOS Drivers */
static SYN_Encoder encoder;
static SYN_Button button;

/**
 * @brief Initialize Encoder, Button, and GPIO Hardware Pins.
 */
void encoder_button_app_init(void)
{
    /* Initialize Rotary Encoder on Phase A (PA0) and Phase B (PA1) */
    syn_encoder_init(&encoder, (SYN_GPIO_Pin)ENC_PIN_A, (SYN_GPIO_Pin)ENC_PIN_B);

    /* Set 4 quadrature transitions per physical detent click */
    syn_encoder_set_steps_per_detent(&encoder, 4);
    encoder.position = ui_state.setpoint_value;

    /* Initialize Push-Button on SW (PA2), Active Low (Pull-Up), 50ms debounce */
    syn_button_init(&button, (SYN_GPIO_Pin)ENC_PIN_SW, SYN_BUTTON_ACTIVE_LOW, 50);
}

/**
 * @brief Read hardware GPIO pin logic levels for Encoder & Button.
 */
static void poll_hardware_inputs(void)
{
    /* Step Encoder & Button State Machines */
    syn_encoder_update(&encoder);
    syn_button_update(&button);
}

/**
 * @brief Periodic 1ms Task (called from TIM or SYSTICK ISR).
 *
 * Samples encoder phases and button contact state at 1kHz.
 */
void encoder_button_sample_task_1ms(void)
{
    poll_hardware_inputs();
}

/**
 * @brief Process UI events from Encoder rotation and Button clicks.
 */
void encoder_button_ui_task(void)
{
    /* 1. Process Rotary Encoder Rotation Delta */
    int32_t delta = syn_encoder_get_delta(&encoder);
    if (delta != 0) {
        if (ui_state.system_on) {
            ui_state.setpoint_value += delta;

            /* Clamp setpoint within bounds (0..100) */
            if (ui_state.setpoint_value < 0) {
                ui_state.setpoint_value = 0;
            } else if (ui_state.setpoint_value > 100) {
                ui_state.setpoint_value = 100;
            }

            encoder.position = ui_state.setpoint_value;
        }
    }

    /* 2. Process Push-Button Click Gestures */
    uint8_t evts = syn_button_poll_events(&button);
    if (evts & SYN_BUTTON_EVT_SINGLE_CLICK) {
        /* Single click -> Toggle between Parameter Selection & Editing mode */
        ui_state.menu_mode ^= 1;
    }

    if (evts & SYN_BUTTON_EVT_DOUBLE_CLICK) {
        /* Double click -> Reset setpoint value to default 50 */
        ui_state.setpoint_value = 50;
        encoder.position = 50;
    }

    if (evts & SYN_BUTTON_EVT_LONG_PRESS) {
        /* Long press -> Toggle system power state ON / OFF */
        ui_state.system_on = !ui_state.system_on;
    }
}
