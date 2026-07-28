/**
 * @file syn_dipswitch.h
 * @brief DIP Switch & Multi-Bit Rotary Selector Driver.
 * @ingroup syn_input
 */

#ifndef SYN_DIPSWITCH_H
#define SYN_DIPSWITCH_H

#include "../common/syn_defs.h"
#include "../port/syn_port_gpio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYN_DIPSWITCH_MAX_PINS 16 /**< Maximum supported GPIO pins in DIP switch array (16) */

/**
 * @brief DIP Switch Context.
 */
typedef struct {
    SYN_GPIO_Pin pins[SYN_DIPSWITCH_MAX_PINS]; /**< Array of GPIO pins */
    uint8_t count;                             /**< Number of configured switch pins */
    bool active_low;                           /**< True if switch ON grounds input pin */
    uint32_t current_value;                    /**< Packed binary integer value */
    uint32_t previous_value;                   /**< Previously read value */
    bool changed;                              /**< True if value changed since last read */
} SYN_DipSwitch;

/**
 * @brief Initialize a DIP Switch instance.
 *
 * Configures input GPIO pins with internal pull-ups or pull-downs.
 *
 * @param ds         DIP switch context.
 * @param pins       Array of GPIO pin identifiers.
 * @param count      Number of switch pins (1 to 16).
 * @param active_low True if switch ON connects pin to GND (uses Pull-UP).
 * @return SYN_OK on success.
 */
SYN_Status syn_dipswitch_init(SYN_DipSwitch *ds, const SYN_GPIO_Pin *pins, uint8_t count,
                              bool active_low);

/**
 * @brief Read all DIP switch pins and update binary value.
 *
 * @param ds DIP switch context.
 */
void syn_dipswitch_read(SYN_DipSwitch *ds);

/**
 * @brief Get the current combined integer bitmask value of switches.
 *
 * @param ds DIP switch context.
 * @return Bitmask where bit N corresponds to pin N.
 */
uint32_t syn_dipswitch_get_value(const SYN_DipSwitch *ds);

/**
 * @brief Check if DIP switch values changed since last read.
 *
 * @param ds DIP switch context.
 * @return True if state changed.
 */
bool syn_dipswitch_has_changed(SYN_DipSwitch *ds);

#ifdef __cplusplus
}
#endif

#endif /* SYN_DIPSWITCH_H */
