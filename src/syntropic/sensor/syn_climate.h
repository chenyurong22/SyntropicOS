/**
 * @file syn_climate.h
 * @brief Generic Climate & Environmental Sensor Driver (SHT3x, BME280, HTU21D, DHT22).
 * @ingroup syn_sensor
 */

#ifndef SYN_CLIMATE_H
#define SYN_CLIMATE_H

#include "../common/syn_defs.h"
#include "../drivers/syn_soft_i2c.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Climate Sensor Type.
 */
typedef enum {
    SYN_CLIMATE_SHT3X = 0,  /**< Sensirion SHT30 / SHT31 / SHT35 */
    SYN_CLIMATE_BME280 = 1, /**< Bosch BME280 (Temp + Humidity + Pressure) */
    SYN_CLIMATE_HTU21D = 2  /**< HTU21D / Si7021 */
} SYN_ClimateType;

/**
 * @brief Generic Climate Sensor Context.
 */
typedef struct {
    SYN_ClimateType type;
    SYN_SoftI2C i2c;
    uint8_t i2c_addr;
    float temperature_c; /**< Temperature in Degrees Celsius */
    float humidity_rh;   /**< Relative Humidity % */
    float pressure_hpa;  /**< Barometric Pressure in hPa / mbar */
    float dew_point_c;   /**< Calculated Dew Point in Degrees Celsius */
} SYN_Climate;

/**
 * @brief Initialize Climate Sensor.
 *
 * @param sensor   Climate sensor context.
 * @param scl      I2C SCL GPIO pin.
 * @param sda      I2C SDA GPIO pin.
 * @param i2c_addr I2C slave address (e.g. 0x44 or 0x76).
 * @param type     Sensor type (SHT3X, BME280, HTU21D).
 * @return SYN_OK on success.
 */
SYN_Status syn_climate_init(SYN_Climate *sensor, SYN_GPIO_Pin scl, SYN_GPIO_Pin sda,
                            uint8_t i2c_addr, SYN_ClimateType type);

/**
 * @brief Feed raw sensor readings (temperature, humidity, pressure).
 *
 * @param sensor     Climate sensor context.
 * @param temp_c     Temperature in Celsius.
 * @param humidity_rh Relative humidity percentage (0 to 100%).
 * @param press_hpa  Barometric pressure in hPa (e.g. 1013.25f).
 */
void syn_climate_feed_sample(SYN_Climate *sensor, float temp_c, float humidity_rh, float press_hpa);

/**
 * @brief Get temperature in Celsius.
 *
 * @param sensor Climate sensor context.
 * @return Temperature in °C.
 */
float syn_climate_get_temp_c(const SYN_Climate *sensor);

/**
 * @brief Get temperature in Fahrenheit.
 *
 * @param sensor Climate sensor context.
 * @return Temperature in °F.
 */
float syn_climate_get_temp_f(const SYN_Climate *sensor);

/**
 * @brief Get relative humidity %.
 *
 * @param sensor Climate sensor context.
 * @return Humidity %RH.
 */
float syn_climate_get_humidity(const SYN_Climate *sensor);

/**
 * @brief Get calculated dew point in Celsius.
 *
 * @param sensor Climate sensor context.
 * @return Dew point in °C.
 */
float syn_climate_get_dew_point(const SYN_Climate *sensor);

#ifdef __cplusplus
}
#endif

#endif /* SYN_CLIMATE_H */
