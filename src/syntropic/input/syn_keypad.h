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

#define SYN_KEYPAD_MAX_ROWS 8 /**< Maximum supported keypad rows (8) */
#define SYN_KEYPAD_MAX_COLS 8 /**< Maximum supported keypad columns (8) */

/**
 * @brief Matrix Keypad Context.
 */
typedef struct SYN_Keypad SYN_Keypad;

/**
 * @brief Keypad Event Callback function prototype.
 * @param kp       Pointer to keypad context.
 * @param key      Ascii character key from keymap.
 * @param pressed  True on key press down, false on release.
 * @param user_ctx User-defined callback context pointer.
 */
typedef void (*SYN_KeypadCallback)(SYN_Keypad *kp, char key, bool pressed, void *user_ctx);

/** Keypad State Context. */
struct SYN_Keypad {
    SYN_GPIO_Pin rows[SYN_KEYPAD_MAX_ROWS]; /**< Array of row GPIO pins */
    uint8_t num_rows;                       /**< Number of active row pins */

    SYN_GPIO_Pin cols[SYN_KEYPAD_MAX_COLS]; /**< Array of column GPIO pins */
    uint8_t num_cols;                       /**< Number of active column pins */

    char keymap[SYN_KEYPAD_MAX_ROWS * SYN_KEYPAD_MAX_COLS]; /**< Flat keymap character mapping */

    char active_key;      /**< Currently pressed debounced key */
    bool is_pressed;      /**< True if key currently down */
    char last_raw_key;    /**< Last scanned raw key */
    uint32_t press_count; /**< Total keypress count */

    SYN_KeypadCallback on_event; /**< Key event callback function */
    void *user_ctx;              /**< User callback context */
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
