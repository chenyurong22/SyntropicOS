/**
 * @file syn_joystick.h
 * @brief Generic Analog Joystick & Multi-Axis Potentiometer Driver.
 * @ingroup syn_input
 */

#ifndef SYN_JOYSTICK_H
#define SYN_JOYSTICK_H

#include "../common/syn_defs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 8-Way D-Pad Directional State.
 */
typedef enum {
    SYN_JOYSTICK_DIR_CENTER = 0,
    SYN_JOYSTICK_DIR_UP = 1,
    SYN_JOYSTICK_DIR_UP_RIGHT = 2,
    SYN_JOYSTICK_DIR_RIGHT = 3,
    SYN_JOYSTICK_DIR_DOWN_RIGHT = 4,
    SYN_JOYSTICK_DIR_DOWN = 5,
    SYN_JOYSTICK_DIR_DOWN_LEFT = 6,
    SYN_JOYSTICK_DIR_LEFT = 7,
    SYN_JOYSTICK_DIR_UP_LEFT = 8
} SYN_JoystickDir;

/**
 * @brief Generic Analog Joystick Context.
 */
typedef struct {
    uint16_t center_x;     /**< ADC zero-point center X */
    uint16_t center_y;     /**< ADC zero-point center Y */
    uint16_t adc_max;      /**< ADC resolution max (e.g. 4095) */
    uint16_t deadband;     /**< Noise deadband radius */
    int16_t current_x_pct; /**< Calculated X percentage (-100 to +100%) */
    int16_t current_y_pct; /**< Calculated Y percentage (-100 to +100%) */
    SYN_JoystickDir dir;   /**< 8-way directional state */
    bool button_pressed;   /**< Digital push button state */
} SYN_Joystick;

/**
 * @brief Initialize Analog Joystick instance.
 *
 * @param joy      Joystick context.
 * @param center_x Expected center X raw ADC reading.
 * @param center_y Expected center Y raw ADC reading.
 * @param adc_max  Max ADC full-scale value (e.g. 4095 for 12-bit ADC).
 * @param deadband Deadband threshold radius (e.g. 100).
 * @return SYN_OK on success.
 */
SYN_Status syn_joystick_init(SYN_Joystick *joy, uint16_t center_x, uint16_t center_y,
                             uint16_t adc_max, uint16_t deadband);

/**
 * @brief Feed raw ADC readings for X and Y axes.
 *
 * @param joy   Joystick context.
 * @param raw_x Raw X axis ADC sample.
 * @param raw_y Raw Y axis ADC sample.
 * @param btn   Pushbutton logical state.
 */
void syn_joystick_feed_adc(SYN_Joystick *joy, uint16_t raw_x, uint16_t raw_y, bool btn);

/**
 * @brief Get X axis position percentage (-100% to +100%).
 *
 * @param joy Joystick context.
 * @return X percentage.
 */
int16_t syn_joystick_get_x_pct(const SYN_Joystick *joy);

/**
 * @brief Get Y axis position percentage (-100% to +100%).
 *
 * @param joy Joystick context.
 * @return Y percentage.
 */
int16_t syn_joystick_get_y_pct(const SYN_Joystick *joy);

/**
 * @brief Get 8-way directional state.
 *
 * @param joy Joystick context.
 * @return Directional state enum.
 */
SYN_JoystickDir syn_joystick_get_dir(const SYN_Joystick *joy);

#ifdef __cplusplus
}
#endif

#endif /* SYN_JOYSTICK_H */
