/**
 * @file syn_usb_hid_keyboard.h
 * @brief Zero-Heap USB HID Boot Keyboard Device Helper Module.
 */

#ifndef SYN_USB_HID_KEYBOARD_H
#define SYN_USB_HID_KEYBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "syntropic/common/syn_defs.h"
#include "syntropic/drivers/syn_usb_hid.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** USB HID Keyboard Modifier Bitmasks per USB HID Usage Tables §10 */
#define SYN_USB_HID_MOD_LCTRL (1U << 0)
#define SYN_USB_HID_MOD_LSHIFT (1U << 1)
#define SYN_USB_HID_MOD_LALT (1U << 2)
#define SYN_USB_HID_MOD_LGUI (1U << 3)
#define SYN_USB_HID_MOD_RCTRL (1U << 4)
#define SYN_USB_HID_MOD_RSHIFT (1U << 5)
#define SYN_USB_HID_MOD_RALT (1U << 6)
#define SYN_USB_HID_MOD_RGUI (1U << 7)

/** Standard USB HID Keyboard Keycodes */
#define SYN_USB_HID_KEY_NONE 0x00U
#define SYN_USB_HID_KEY_A 0x04U
#define SYN_USB_HID_KEY_B 0x05U
#define SYN_USB_HID_KEY_C 0x06U
#define SYN_USB_HID_KEY_D 0x07U
#define SYN_USB_HID_KEY_E 0x08U
#define SYN_USB_HID_KEY_F 0x09U
#define SYN_USB_HID_KEY_G 0x0AU
#define SYN_USB_HID_KEY_H 0x0BU
#define SYN_USB_HID_KEY_I 0x0CU
#define SYN_USB_HID_KEY_J 0x0DU
#define SYN_USB_HID_KEY_K 0x0EU
#define SYN_USB_HID_KEY_L 0x0FU
#define SYN_USB_HID_KEY_M 0x10U
#define SYN_USB_HID_KEY_N 0x11U
#define SYN_USB_HID_KEY_O 0x12U
#define SYN_USB_HID_KEY_P 0x13U
#define SYN_USB_HID_KEY_Q 0x14U
#define SYN_USB_HID_KEY_R 0x15U
#define SYN_USB_HID_KEY_S 0x16U
#define SYN_USB_HID_KEY_T 0x17U
#define SYN_USB_HID_KEY_U 0x18U
#define SYN_USB_HID_KEY_V 0x19U
#define SYN_USB_HID_KEY_W 0x1AU
#define SYN_USB_HID_KEY_X 0x1BU
#define SYN_USB_HID_KEY_Y 0x1CU
#define SYN_USB_HID_KEY_Z 0x1DU
#define SYN_USB_HID_KEY_1 0x1EU
#define SYN_USB_HID_KEY_2 0x1FU
#define SYN_USB_HID_KEY_3 0x20U
#define SYN_USB_HID_KEY_4 0x21U
#define SYN_USB_HID_KEY_5 0x22U
#define SYN_USB_HID_KEY_6 0x23U
#define SYN_USB_HID_KEY_7 0x24U
#define SYN_USB_HID_KEY_8 0x25U
#define SYN_USB_HID_KEY_9 0x26U
#define SYN_USB_HID_KEY_0 0x27U
#define SYN_USB_HID_KEY_ENTER 0x28U
#define SYN_USB_HID_KEY_ESCAPE 0x29U
#define SYN_USB_HID_KEY_BACKSPACE 0x2AU
#define SYN_USB_HID_KEY_TAB 0x2BU
#define SYN_USB_HID_KEY_SPACE 0x2CU

/** Standard 8-byte USB HID Boot Keyboard IN Report structure */
typedef struct SYN_PACKED {
    uint8_t modifier;   /**< Modifier keys bitmask (CTRL, SHIFT, ALT, GUI) */
    uint8_t reserved;   /**< Reserved byte (0x00)                           */
    uint8_t keycode[6]; /**< Keycodes for up to 6 pressed keys              */
} SYN_USB_HID_KeyboardReport;

/** Standard USB HID Keyboard Report Descriptor (63 Bytes) */
extern const uint8_t SYN_USB_HID_KEYBOARD_REPORT_DESC[63];

/**
 * @brief Send a USB HID Keyboard report containing active keycodes.
 *
 * @param hid       Pointer to USB HID instance.
 * @param modifier  Modifier bitmask (CTRL, SHIFT, ALT, GUI).
 * @param keycodes  Array of up to 6 pressed keycodes (or NULL for no key).
 * @return SYN_OK on success.
 */
SYN_Status syn_usb_hid_keyboard_send(SYN_USB_HID *hid, uint8_t modifier, const uint8_t keycodes[6]);

/**
 * @brief Send a single keystroke (press 1 key + optional modifier).
 *
 * @param hid       Pointer to USB HID instance.
 * @param modifier  Modifier bitmask (CTRL, SHIFT, ALT, GUI).
 * @param keycode   HID keycode (e.g. SYN_USB_HID_KEY_A).
 * @return SYN_OK on success.
 */
SYN_Status syn_usb_hid_keyboard_press(SYN_USB_HID *hid, uint8_t modifier, uint8_t keycode);

/**
 * @brief Send a keyboard release report (all keys released).
 *
 * @param hid Pointer to USB HID instance.
 * @return SYN_OK on success.
 */
SYN_Status syn_usb_hid_keyboard_release(SYN_USB_HID *hid);

#ifdef __cplusplus
}
#endif

#endif /* SYN_USB_HID_KEYBOARD_H */
