/**
 * @file syn_shiftreg.h
 * @brief Generic Shift Register Driver (74HC595 Output Expander & 74HC165 Input Expander).
 * @ingroup syn_drivers
 */

#ifndef SYN_SHIFTREG_H
#define SYN_SHIFTREG_H

#include "../common/syn_defs.h"
#include "../port/syn_port_gpio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYN_SHIFTREG_MAX_CHIPS 8

/* ── Output Shift Register (e.g. 74HC595, CD4094, TPIC6C596) ───────────── */

/**
 * @brief Output Shift Register Context.
 */
typedef struct {
    SYN_GPIO_Pin data_pin;                  /**< Serial Data Out pin (DS / SER) */
    SYN_GPIO_Pin clock_pin;                 /**< Shift Clock pin (SH_CP / SRCLK) */
    SYN_GPIO_Pin latch_pin;                 /**< Storage Latch Clock pin (ST_CP / RCLK) */
    SYN_GPIO_Pin oe_pin;                    /**< Optional Output Enable pin (0 = unused) */
    uint8_t num_chips;                      /**< Number of cascaded 8-bit chips (1 to 8) */
    bool msb_first;                         /**< True if shifting MSB first */
    uint8_t buffer[SYN_SHIFTREG_MAX_CHIPS]; /**< Output state buffer bytes */
} SYN_ShiftRegOut;

/**
 * @brief Initialize an Output Shift Register (74HC595).
 *
 * @param sr        Shift register context.
 * @param data_pin  Serial Data GPIO pin.
 * @param clock_pin Shift Clock GPIO pin.
 * @param latch_pin Latch / Storage Clock GPIO pin.
 * @param num_chips Number of cascaded 8-bit chips (1 to 8).
 * @return SYN_OK on success.
 */
SYN_Status syn_shiftreg_out_init(SYN_ShiftRegOut *sr, SYN_GPIO_Pin data_pin,
                                 SYN_GPIO_Pin clock_pin, SYN_GPIO_Pin latch_pin,
                                 uint8_t num_chips);

/**
 * @brief Configure optional Output Enable (OE) pin.
 *
 * @param sr     Shift register context.
 * @param oe_pin Output Enable GPIO pin.
 */
void syn_shiftreg_out_set_oe_pin(SYN_ShiftRegOut *sr, SYN_GPIO_Pin oe_pin);

/**
 * @brief Set bit order mode.
 *
 * @param sr        Shift register context.
 * @param msb_first True for MSB-first, false for LSB-first.
 */
void syn_shiftreg_out_set_bit_order(SYN_ShiftRegOut *sr, bool msb_first);

/**
 * @brief Set logical state of a single pin in the shift register chain.
 *
 * @param sr        Shift register context.
 * @param bit_index Pin index (0 to num_chips*8 - 1).
 * @param state     Logical state.
 */
void syn_shiftreg_out_set_bit(SYN_ShiftRegOut *sr, uint16_t bit_index, bool state);

/**
 * @brief Set raw byte for a specific chip in the chain.
 *
 * @param sr         Shift register context.
 * @param chip_index Chip index (0 to num_chips-1).
 * @param val        8-bit byte value.
 */
void syn_shiftreg_out_write_byte(SYN_ShiftRegOut *sr, uint8_t chip_index, uint8_t val);

/**
 * @brief Atomically flush buffer contents to physical hardware output pins.
 *
 * @param sr Shift register context.
 */
void syn_shiftreg_out_flush(SYN_ShiftRegOut *sr);

/**
 * @brief Enable or disable outputs via Output Enable (OE) pin.
 *
 * @param sr     Shift register context.
 * @param enable True to enable outputs (OE LOW).
 */
void syn_shiftreg_out_set_enable(SYN_ShiftRegOut *sr, bool enable);

/* ── Input Shift Register (e.g. 74HC165, CD4021) ────────────────────────── */

/**
 * @brief Input Shift Register Context.
 */
typedef struct {
    SYN_GPIO_Pin data_pin;                  /**< Serial Data In pin (Q7 / SO) */
    SYN_GPIO_Pin clock_pin;                 /**< Shift Clock pin (CLK) */
    SYN_GPIO_Pin load_pin;                  /**< Parallel Load pin (PL / LD) */
    uint8_t num_chips;                      /**< Number of cascaded 8-bit chips (1 to 8) */
    bool msb_first;                         /**< True if shifting MSB first */
    uint8_t buffer[SYN_SHIFTREG_MAX_CHIPS]; /**< Sampled input state buffer bytes */
} SYN_ShiftRegIn;

/**
 * @brief Initialize an Input Shift Register (74HC165).
 *
 * @param sr        Shift register context.
 * @param data_pin  Serial Data In GPIO pin.
 * @param clock_pin Shift Clock GPIO pin.
 * @param load_pin  Parallel Load GPIO pin.
 * @param num_chips Number of cascaded 8-bit chips (1 to 8).
 * @return SYN_OK on success.
 */
SYN_Status syn_shiftreg_in_init(SYN_ShiftRegIn *sr, SYN_GPIO_Pin data_pin,
                                SYN_GPIO_Pin clock_pin, SYN_GPIO_Pin load_pin,
                                uint8_t num_chips);

/**
 * @brief Set bit order mode for input sampling.
 *
 * @param sr        Shift register context.
 * @param msb_first True for MSB-first, false for LSB-first.
 */
void syn_shiftreg_in_set_bit_order(SYN_ShiftRegIn *sr, bool msb_first);

/**
 * @brief Pulse Parallel Load pin and clock in all input bits from hardware.
 *
 * @param sr Shift register context.
 */
void syn_shiftreg_in_read(SYN_ShiftRegIn *sr);

/**
 * @brief Get logical state of a specific sampled input pin.
 *
 * @param sr        Shift register context.
 * @param bit_index Pin index (0 to num_chips*8 - 1).
 * @return Logical state (true/false).
 */
bool syn_shiftreg_in_get_bit(const SYN_ShiftRegIn *sr, uint16_t bit_index);

/**
 * @brief Get sampled byte for a specific chip in the chain.
 *
 * @param sr         Shift register context.
 * @param chip_index Chip index (0 to num_chips-1).
 * @return 8-bit byte value.
 */
uint8_t syn_shiftreg_in_get_byte(const SYN_ShiftRegIn *sr, uint8_t chip_index);

#ifdef __cplusplus
}
#endif

#endif /* SYN_SHIFTREG_H */
