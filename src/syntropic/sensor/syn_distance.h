/**
 * @file syn_distance.h
 * @brief Generic Distance & Proximity Sensor Driver (HC-SR04, VL53L0X, Sharp IR).
 * @ingroup syn_sensor
 */

#ifndef SYN_DISTANCE_H
#define SYN_DISTANCE_H

#include "../common/syn_defs.h"
#include "../port/syn_port_gpio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Distance Sensor Type.
 */
typedef enum {
    SYN_DISTANCE_ULTRASONIC = 0, /**< Pulse trigger/echo (HC-SR04, JSN-SR04T) */
    SYN_DISTANCE_TOF_LASER = 1,  /**< Time-of-Flight Laser (VL53L0X, VL53L1X) */
    SYN_DISTANCE_INFRARED = 2    /**< Optical Analog IR (Sharp GP2Y0A21) */
} SYN_DistanceType;

/**
 * @brief Generic Distance Sensor Context.
 */
typedef struct {
    SYN_DistanceType type;        /**< Distance sensor model type */
    SYN_GPIO_Pin trig_pin;        /**< Trigger pulse / SCL GPIO pin */
    SYN_GPIO_Pin echo_pin;        /**< Echo input / SDA GPIO pin */
    uint32_t last_distance_mm;    /**< Calculated distance in millimeters */
    uint32_t min_range_mm;        /**< Min sensor range in mm */
    uint32_t max_range_mm;        /**< Max sensor range in mm */
    bool obstacle_detected;       /**< True if object within proximity threshold */
    uint32_t proximity_thresh_mm; /**< Proximity alarm threshold in mm */
} SYN_Distance;

/**
 * @brief Initialize Distance Sensor.
 *
 * @param sensor    Distance sensor context.
 * @param trig_pin  Trigger GPIO pin (or SCL for I2C TOF).
 * @param echo_pin  Echo GPIO pin (or SDA for I2C TOF).
 * @param min_mm    Min valid distance in mm (e.g. 20mm).
 * @param max_mm    Max valid distance in mm (e.g. 4000mm).
 * @param type      Sensor type.
 * @return SYN_OK on success.
 */
SYN_Status syn_distance_init(SYN_Distance *sensor, SYN_GPIO_Pin trig_pin, SYN_GPIO_Pin echo_pin,
                             uint32_t min_mm, uint32_t max_mm, SYN_DistanceType type);

/**
 * @brief Feed measured pulse duration in microseconds (for Ultrasonic) or raw mm (for TOF).
 *
 * @param sensor   Distance sensor context.
 * @param pulse_us Echo pulse width in microseconds (or raw mm).
 */
void syn_distance_feed_pulse(SYN_Distance *sensor, uint32_t pulse_us);

/**
 * @brief Set proximity alarm threshold in mm.
 *
 * @param sensor    Distance sensor context.
 * @param thresh_mm Proximity threshold in mm.
 */
void syn_distance_set_proximity_threshold(SYN_Distance *sensor, uint32_t thresh_mm);

/**
 * @brief Get calculated distance in millimeters.
 *
 * @param sensor Distance sensor context.
 * @return Distance in mm.
 */
uint32_t syn_distance_get_mm(const SYN_Distance *sensor);

/**
 * @brief Get calculated distance in centimeters.
 *
 * @param sensor Distance sensor context.
 * @return Distance in cm.
 */
uint32_t syn_distance_get_cm(const SYN_Distance *sensor);

/**
 * @brief Check if an object is within proximity threshold.
 *
 * @param sensor Distance sensor context.
 * @return True if obstacle detected within threshold.
 */
bool syn_distance_is_obstacle_detected(const SYN_Distance *sensor);

#ifdef __cplusplus
}
#endif

#endif /* SYN_DISTANCE_H */
