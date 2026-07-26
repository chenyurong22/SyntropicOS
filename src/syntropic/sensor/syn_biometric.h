/**
 * @file syn_biometric.h
 * @brief Generic Pulse Oximeter & Heart Rate Sensor Driver (MAX30102, MAX30100).
 * @ingroup syn_sensor
 */

#ifndef SYN_BIOMETRIC_H
#define SYN_BIOMETRIC_H

#include "../common/syn_defs.h"
#include "../drivers/syn_soft_i2c.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Biometric Sensor Type.
 */
typedef enum {
    SYN_BIOMETRIC_MAX30102 = 0, /**< MAX30102 Red + IR Pulse Oximeter */
    SYN_BIOMETRIC_MAX30100 = 1  /**< MAX30100 Red + IR Pulse Oximeter */
} SYN_BiometricType;

/**
 * @brief Generic Biometric Sensor Context.
 */
typedef struct {
    SYN_BiometricType type;
    SYN_SoftI2C i2c;
    uint8_t i2c_addr;
    uint16_t heart_rate_bpm; /**< Measured Heart Rate in BPM (e.g. 72) */
    float spo2_pct;          /**< Measured Blood Oxygen Saturation % (e.g. 98.5%) */
    bool finger_detected;    /**< True if finger placed on sensor glass */
} SYN_Biometric;

/**
 * @brief Initialize Biometric Sensor.
 *
 * @param bio      Biometric sensor context.
 * @param scl      I2C SCL GPIO pin.
 * @param sda      I2C SDA GPIO pin.
 * @param i2c_addr I2C slave address (e.g. 0x57).
 * @param type     Sensor type (MAX30102 or MAX30100).
 * @return SYN_OK on success.
 */
SYN_Status syn_biometric_init(SYN_Biometric *bio, SYN_GPIO_Pin scl, SYN_GPIO_Pin sda,
                              uint8_t i2c_addr, SYN_BiometricType type);

/**
 * @brief Feed raw Red & IR photodiode samples.
 *
 * @param bio    Biometric sensor context.
 * @param red_raw Raw 18-bit Red LED sample.
 * @param ir_raw  Raw 18-bit IR LED sample.
 */
void syn_biometric_feed_samples(SYN_Biometric *bio, uint32_t red_raw, uint32_t ir_raw);

/**
 * @brief Get measured Heart Rate in BPM.
 *
 * @param bio Biometric sensor context.
 * @return Heart rate in Beats Per Minute.
 */
uint16_t syn_biometric_get_bpm(const SYN_Biometric *bio);

/**
 * @brief Get measured Blood Oxygen Saturation %.
 *
 * @param bio Biometric sensor context.
 * @return SpO2 percentage.
 */
float syn_biometric_get_spo2(const SYN_Biometric *bio);

/**
 * @brief Check if finger is detected.
 *
 * @param bio Biometric sensor context.
 * @return True if finger present.
 */
bool syn_biometric_is_finger_detected(const SYN_Biometric *bio);

#ifdef __cplusplus
}
#endif

#endif /* SYN_BIOMETRIC_H */
