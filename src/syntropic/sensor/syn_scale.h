/**
 * @file syn_scale.h
 * @brief Generic Weight Scale & Load Cell Driver (HX711 24-bit ADC, NAU7802).
 * @ingroup syn_sensor
 */

#ifndef SYN_SCALE_H
#define SYN_SCALE_H

#include "../common/syn_defs.h"
#include "../port/syn_port_gpio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Load Cell ADC IC Type.
 */
typedef enum {
    SYN_SCALE_HX711   = 0, /**< HX711 24-bit 2-wire serial ADC */
    SYN_SCALE_NAU7802 = 1  /**< NAU7802 24-bit I2C Scale ADC */
} SYN_ScaleType;

/**
 * @brief Generic Weight Scale Instance Context.
 */
typedef struct {
    SYN_ScaleType type;
    SYN_GPIO_Pin dout_pin;
    SYN_GPIO_Pin sck_pin;
    int32_t tare_offset;       /**< Zero-point tare offset raw reading */
    float scale_factor;        /**< Counts per gram scale factor */
    float last_weight_grams;   /**< Calculated weight in grams */
    bool is_stable;            /**< True if scale reading has settled */
} SYN_Scale;

/**
 * @brief Initialize Weight Scale context.
 *
 * @param scale    Scale context.
 * @param dout_pin Data GPIO pin (or SDA for I2C).
 * @param sck_pin  Clock GPIO pin (or SCL for I2C).
 * @param type     Scale ADC type (HX711 or NAU7802).
 * @return SYN_OK on success.
 */
SYN_Status syn_scale_init(SYN_Scale *scale, SYN_GPIO_Pin dout_pin, SYN_GPIO_Pin sck_pin, SYN_ScaleType type);

/**
 * @brief Feed raw 24-bit ADC reading from load cell chip.
 *
 * @param scale    Scale context.
 * @param raw_adc24 Signed 24-bit raw ADC reading.
 */
void syn_scale_feed_adc(SYN_Scale *scale, int32_t raw_adc24);

/**
 * @brief Set zero-point Tare offset.
 *
 * @param scale       Scale context.
 * @param tare_offset Zero reading value.
 */
void syn_scale_tare(SYN_Scale *scale, int32_t tare_offset);

/**
 * @brief Set calibration scale factor (counts per gram).
 *
 * @param scale  Scale context.
 * @param factor Calibration factor (e.g. 420.0f counts/gram).
 */
void syn_scale_set_calibration_factor(SYN_Scale *scale, float factor);

/**
 * @brief Get calculated weight in grams.
 *
 * @param scale Scale context.
 * @return Weight in grams.
 */
float syn_scale_get_grams(const SYN_Scale *scale);

/**
 * @brief Get calculated weight in kilograms.
 *
 * @param scale Scale context.
 * @return Weight in kg.
 */
float syn_scale_get_kg(const SYN_Scale *scale);

#ifdef __cplusplus
}
#endif

#endif /* SYN_SCALE_H */
