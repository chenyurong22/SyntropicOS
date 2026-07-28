/**
 * @file main.c
 * @brief SyntropicOS 4x4 Matrix Keypad Scanner STM32 HAL Example.
 *
 * Demonstrates non-blocking row/col scanning (`syn_keypad_scan`), keypress event
 * callbacks (`syn_keypad_set_callback`), 16-key matrix layout (0-9, A-D, *, #),
 * and multi-digit PIN passcode entry verification using STM32 HAL GPIO drivers.
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

#include <stdio.h>
#include <string.h>

/* Pin Definitions for 4 Rows and 4 Columns */
#define ROW_PORT GPIOA
#define COL_PORT GPIOA

#define ROW0_PIN GPIO_PIN_0
#define ROW1_PIN GPIO_PIN_1
#define ROW2_PIN GPIO_PIN_2
#define ROW3_PIN GPIO_PIN_3

#define COL0_PIN GPIO_PIN_4
#define COL1_PIN GPIO_PIN_5
#define COL2_PIN GPIO_PIN_6
#define COL3_PIN GPIO_PIN_7

#define LOCK_PORT GPIOB
#define LOCK_PIN  GPIO_PIN_0

/* 4x4 Keymap Layout (16 characters) */
static const char KEYMAP_4X4[16] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D'
};

/* Secret Security PIN Code */
#define SECRET_PIN "1234"

/* Application Security Passcode State */
typedef struct {
    char entered_pin[8];
    uint8_t pin_idx;
    bool door_unlocked;
    uint32_t unlock_tick;
} Security_State;

static Security_State sec_state = {0};

/* SyntropicOS Keypad Context */
static SYN_Keypad keypad;

/**
 * @brief Keypad Event Callback invoked when a key is pressed or released.
 */
static void on_keypad_event(SYN_Keypad *kp, char key, bool pressed, void *user_ctx)
{
    (void)kp;
    (void)user_ctx;

    if (pressed) {
        if (key == '#') {
            /* '#' key submits entered PIN code */
            sec_state.entered_pin[sec_state.pin_idx] = '\0';

            if (strcmp(sec_state.entered_pin, SECRET_PIN) == 0) {
                /* Valid PIN -> Unlock solenoid lock */
                sec_state.door_unlocked = true;
                sec_state.unlock_tick = syn_port_get_tick_ms();
                HAL_GPIO_WritePin(LOCK_PORT, LOCK_PIN, GPIO_PIN_SET);
            }

            /* Reset PIN entry buffer */
            sec_state.pin_idx = 0;
            memset(sec_state.entered_pin, 0, sizeof(sec_state.entered_pin));
        } else if (key == '*') {
            /* '*' key clears current PIN buffer */
            sec_state.pin_idx = 0;
            memset(sec_state.entered_pin, 0, sizeof(sec_state.entered_pin));
        } else {
            /* Store digit in PIN entry buffer */
            if (sec_state.pin_idx < sizeof(sec_state.entered_pin) - 1) {
                sec_state.entered_pin[sec_state.pin_idx++] = key;
            }
        }
    }
}

/**
 * @brief Initialize Keypad Hardware Pins and SyntropicOS Keypad Engine.
 */
void keypad_app_init(void)
{
    SYN_GPIO_Pin row_pins[4] = {
        (SYN_GPIO_Pin)ROW0_PIN, (SYN_GPIO_Pin)ROW1_PIN,
        (SYN_GPIO_Pin)ROW2_PIN, (SYN_GPIO_Pin)ROW3_PIN
    };

    SYN_GPIO_Pin col_pins[4] = {
        (SYN_GPIO_Pin)COL0_PIN, (SYN_GPIO_Pin)COL1_PIN,
        (SYN_GPIO_Pin)COL2_PIN, (SYN_GPIO_Pin)COL3_PIN
    };

    /* Initialize 4x4 matrix keypad with keymap */
    syn_keypad_init(&keypad, row_pins, 4, col_pins, 4, KEYMAP_4X4);

    /* Register keypress event handler */
    syn_keypad_set_callback(&keypad, on_keypad_event, NULL);
}

/**
 * @brief Perform physical hardware GPIO row drive & col read scan pass.
 */
static void scan_hardware_matrix(void)
{
    /* Driven 1 row high at a time, read active columns */
    for (uint8_t r = 0; r < 4; r++) {
        /* Set active row HIGH, other rows LOW */
        HAL_GPIO_WritePin(ROW_PORT, ROW0_PIN, (r == 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(ROW_PORT, ROW1_PIN, (r == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(ROW_PORT, ROW2_PIN, (r == 2) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(ROW_PORT, ROW3_PIN, (r == 3) ? GPIO_PIN_SET : GPIO_PIN_RESET);

        /* Read columns */
        GPIO_PinState col0 = HAL_GPIO_ReadPin(COL_PORT, COL0_PIN);
        GPIO_PinState col1 = HAL_GPIO_ReadPin(COL_PORT, COL1_PIN);
        GPIO_PinState col2 = HAL_GPIO_ReadPin(COL_PORT, COL2_PIN);
        GPIO_PinState col3 = HAL_GPIO_ReadPin(COL_PORT, COL3_PIN);

        (void)col0; (void)col1; (void)col2; (void)col3;
    }
}

/**
 * @brief Periodic 100Hz Keypad Scanning Task (called every 10ms).
 */
void keypad_app_task_10ms(void)
{
    scan_hardware_matrix();

    /* Step SyntropicOS Keypad Scanner Engine */
    syn_keypad_scan(&keypad);

    /* Auto-relock solenoid lock after 5000ms */
    if (sec_state.door_unlocked) {
        if ((syn_port_get_tick_ms() - sec_state.unlock_tick) >= 5000U) {
            sec_state.door_unlocked = false;
            HAL_GPIO_WritePin(LOCK_PORT, LOCK_PIN, GPIO_PIN_RESET);
        }
    }
}
