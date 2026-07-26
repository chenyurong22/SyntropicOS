/**
 * @file syn_keypad.h
 * @brief Non-blocking Matrix Keypad Scanner (3x4, 4x4, etc.).
 * @ingroup syn_input
 */

#ifndef SYN_KEYPAD_H
#define SYN_KEYPAD_H

#include "../common/syn_defs.h"
#include "../port/syn_port_gpio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYN_KEYPAD_MAX_ROWS 8
#define SYN_KEYPAD_MAX_COLS 8

/**
 * @brief Matrix Keypad Context.
 */
typedef struct SYN_Keypad SYN_Keypad;

typedef void (*SYN_KeypadCallback)(SYN_Keypad *kp, char key, bool pressed, void *user_ctx);

struct SYN_Keypad {
    SYN_GPIO_Pin rows[SYN_KEYPAD_MAX_ROWS];
    uint8_t num_rows;

    SYN_GPIO_Pin cols[SYN_KEYPAD_MAX_COLS];
    uint8_t num_cols;

    char keymap[SYN_KEYPAD_MAX_ROWS * SYN_KEYPAD_MAX_COLS];

    char active_key;
    bool is_pressed;
    char last_raw_key;
    uint32_t press_count;

    SYN_KeypadCallback on_event;
    void *user_ctx;
};

/**
 * @brief Initialize a Matrix Keypad instance.
 *
 * Configures row pins as outputs and col pins as inputs with pull-down/up.
 *
 * @param kp       Keypad context.
 * @param rows     Array of row GPIO pins.
 * @param num_rows Number of row pins (1-8).
 * @param cols     Array of column GPIO pins.
 * @param num_cols Number of column pins (1-8).
 * @param keymap   String keymap of length (num_rows * num_cols).
 * @return SYN_OK on success.
 */
SYN_Status syn_keypad_init(SYN_Keypad *kp, const SYN_GPIO_Pin *rows, uint8_t num_rows,
                           const SYN_GPIO_Pin *cols, uint8_t num_cols, const char *keymap);

/**
 * @brief Perform a non-blocking matrix scan.
 *
 * @param kp Keypad context.
 */
void syn_keypad_scan(SYN_Keypad *kp);

/**
 * @brief Get the currently pressed key character.
 *
 * @param kp      Keypad context.
 * @param out_key [out] Key character (if pressed).
 * @return True if a key is currently held pressed.
 */
bool syn_keypad_get_key(const SYN_Keypad *kp, char *out_key);

/**
 * @brief Set event callback for key press and release events.
 *
 * @param kp       Keypad context.
 * @param callback Event handler callback function.
 * @param user_ctx User context pointer.
 */
void syn_keypad_set_callback(SYN_Keypad *kp, SYN_KeypadCallback callback, void *user_ctx);

#ifdef __cplusplus
}
#endif

#endif /* SYN_KEYPAD_H */
