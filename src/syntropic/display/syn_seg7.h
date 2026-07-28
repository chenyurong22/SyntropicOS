/**
 * @file syn_seg7.h
 * @brief 7-Segment LED Display & Multi-Digit Array Driver.
 * @ingroup syn_display
 */

#ifndef SYN_SEG7_H
#define SYN_SEG7_H

#include "../common/syn_defs.h"
#include "../port/syn_port_gpio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYN_SEG7_MAX_DIGITS 8 /**< Maximum supported display digits (8) */

/**
 * @brief 7-segment display wiring polarity.
 */
typedef enum {
    SYN_SEG7_COMMON_CATHODE = 0, /**< Active HIGH segments, active LOW digit selects */
    SYN_SEG7_COMMON_ANODE = 1    /**< Active LOW segments, active HIGH digit selects */
} SYN_Seg7Type;

/**
 * @brief 7-Segment Display Instance Context.
 */
typedef struct {
    SYN_GPIO_Pin segment_pins[8];                 /**< Segment pins: A, B, C, D, E, F, G, DP */
    SYN_GPIO_Pin digit_pins[SYN_SEG7_MAX_DIGITS]; /**< Digit selection pins */
    uint8_t num_digits;                           /**< Total number of digits (1 to 8) */
    SYN_Seg7Type type;                            /**< Common Cathode or Common Anode */
    uint8_t digit_buffers[SYN_SEG7_MAX_DIGITS];   /**< Raw segment bitmask per digit */
    uint8_t active_digit;                         /**< Active scanning digit index */
    bool leading_zeros;                           /**< True to display leading zeros */
} SYN_Seg7;

/**
 * @brief Initialize a 7-Segment Display instance.
 *
 * @param seg        7-segment context.
 * @param segments   Array of 8 segment GPIO pins (A, B, C, D, E, F, G, DP).
 * @param digits     Array of digit selector GPIO pins (up to 8).
 * @param num_digits Number of digits in the array (1 to 8).
 * @param type       Wiring type (Common Cathode / Common Anode).
 * @return SYN_OK on success.
 */
SYN_Status syn_seg7_init(SYN_Seg7 *seg, const SYN_GPIO_Pin segments[8], const SYN_GPIO_Pin *digits,
                         uint8_t num_digits, SYN_Seg7Type type);

/**
 * @brief Non-blocking multiplex scan tick. Call periodically in main/scheduler loop.
 *
 * @param seg 7-segment context.
 */
void syn_seg7_scan(SYN_Seg7 *seg);

/**
 * @brief Clear display (turn off all segments).
 *
 * @param seg 7-segment context.
 */
void syn_seg7_clear(SYN_Seg7 *seg);

/**
 * @brief Display an integer number.
 *
 * @param seg 7-segment context.
 * @param val Signed integer value to display.
 */
void syn_seg7_print_int(SYN_Seg7 *seg, int32_t val);

/**
 * @brief Display a floating-point number with specified decimal precision.
 *
 * @param seg      7-segment context.
 * @param val      Float value to display.
 * @param decimals Number of fractional digits after decimal point.
 */
void syn_seg7_print_float(SYN_Seg7 *seg, float val, uint8_t decimals);

/**
 * @brief Display an integer in hexadecimal format.
 *
 * @param seg 7-segment context.
 * @param val Unsigned integer to display in hex.
 */
void syn_seg7_print_hex(SYN_Seg7 *seg, uint32_t val);

/**
 * @brief Display a text string (best-effort 7-segment ASCII mapping).
 *
 * @param seg 7-segment context.
 * @param str ASCII string.
 */
void syn_seg7_print_str(SYN_Seg7 *seg, const char *str);

/**
 * @brief Set raw segment bitmask for a specific digit.
 *
 * @param seg       7-segment context.
 * @param digit_idx Digit index (0 to num_digits-1).
 * @param seg_mask  Raw segment bitmask (Bit 0=A, Bit 1=B, ..., Bit 7=DP).
 */
void syn_seg7_set_digit_raw(SYN_Seg7 *seg, uint8_t digit_idx, uint8_t seg_mask);

#ifdef __cplusplus
}
#endif

#endif /* SYN_SEG7_H */
