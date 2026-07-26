/**
 * @file syn_touch.h
 * @brief Capacitive Touch Sensor Driver (relaxation/ADC charge sensing).
 * @ingroup syn_input
 */

#ifndef SYN_TOUCH_H
#define SYN_TOUCH_H

#include "../common/syn_defs.h"
#include "../port/syn_port_gpio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Capacitive Touch Button Instance Context.
 */
typedef struct {
    SYN_GPIO_Pin pin;     /**< GPIO pin identifier */
    uint16_t baseline;    /**< Dynamic baseline capacitance value */
    uint16_t threshold;   /**< Touch detection threshold Delta */
    uint16_t current_val; /**< Current raw sample value */
    uint16_t hysteresis;  /**< Hysteresis for press/release stability */
    bool is_pressed;      /**< True if touch button is pressed */
    uint32_t press_count; /**< Total press count */
} SYN_Touch;

/**
 * @brief Initialize a Capacitive Touch Sensor instance.
 *
 * @param touch     Touch context.
 * @param pin       GPIO pin identifier.
 * @param threshold Touch detection delta threshold.
 * @return SYN_OK on success.
 */
SYN_Status syn_touch_init(SYN_Touch *touch, SYN_GPIO_Pin pin, uint16_t threshold);

/**
 * @brief Feed a raw capacitance charge-time / ADC sample.
 *
 * @param touch      Touch context.
 * @param raw_sample Raw charge time / capacitance sample.
 */
void syn_touch_feed_sample(SYN_Touch *touch, uint16_t raw_sample);

/**
 * @brief Calibrate baseline value from current environmental noise floor.
 *
 * @param touch    Touch context.
 * @param baseline Baseline raw value.
 */
void syn_touch_calibrate(SYN_Touch *touch, uint16_t baseline);

/**
 * @brief Check if touch sensor is pressed.
 *
 * @param touch Touch context.
 * @return True if currently touched/pressed.
 */
bool syn_touch_is_pressed(const SYN_Touch *touch);

#ifdef __cplusplus
}
#endif

#endif /* SYN_TOUCH_H */
