/**
 * @file syn_lux.h
 * @brief Generic Ambient Light & RGB Color Sensor Driver (BH1750, TSL2561, TCS34725).
 * @ingroup syn_sensor
 */

#ifndef SYN_LUX_H
#define SYN_LUX_H

#include "../common/syn_defs.h"
#include "../drivers/syn_soft_i2c.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Light & Color Sensor Type.
 */
typedef enum {
    SYN_LUX_BH1750   = 0, /**< BH1750 Ambient Light Sensor (Lux) */
    SYN_LUX_TSL2561  = 1, /**< TSL2561 Dual-channel IR/Vis Light Sensor */
    SYN_LUX_TCS34725 = 2  /**< TCS34725 RGB Color + Clear Light Sensor */
} SYN_LuxType;

/**
 * @brief Generic Light & Color Sensor Context.
 */
typedef struct {
    SYN_LuxType type;
    SYN_SoftI2C i2c;
    uint8_t i2c_addr;
    float illuminance_lux;      /**< Ambient Light in Lux */
    uint16_t color_r;           /**< Raw Red channel (TCS34725) */
    uint16_t color_g;           /**< Raw Green channel (TCS34725) */
    uint16_t color_b;           /**< Raw Blue channel (TCS34725) */
    uint16_t color_c;           /**< Raw Clear channel (TCS34725) */
    uint16_t color_temp_k;      /**< Calculated Correlated Color Temp in Kelvin */
} SYN_Lux;

/**
 * @brief Initialize Light & Color Sensor.
 *
 * @param sensor   Lux sensor context.
 * @param scl      I2C SCL GPIO pin.
 * @param sda      I2C SDA GPIO pin.
 * @param i2c_addr I2C slave address (e.g. 0x23 or 0x29).
 * @param type     Sensor type (BH1750, TSL2561, TCS34725).
 * @return SYN_OK on success.
 */
SYN_Status syn_lux_init(SYN_Lux *sensor, SYN_GPIO_Pin scl, SYN_GPIO_Pin sda,
                        uint8_t i2c_addr, SYN_LuxType type);

/**
 * @brief Feed raw illuminance reading in Lux (for BH1750/TSL2561).
 *
 * @param sensor Lux sensor context.
 * @param lux    Ambient light value in Lux.
 */
void syn_lux_feed_lux(SYN_Lux *sensor, float lux);

/**
 * @brief Feed raw RGBC channels (for TCS34725).
 *
 * @param sensor Lux sensor context.
 * @param r      Red channel raw ADC.
 * @param g      Green channel raw ADC.
 * @param b      Blue channel raw ADC.
 * @param c      Clear channel raw ADC.
 */
void syn_lux_feed_rgbc(SYN_Lux *sensor, uint16_t r, uint16_t g, uint16_t b, uint16_t c);

/**
 * @brief Get ambient illuminance in Lux.
 *
 * @param sensor Lux sensor context.
 * @return Lux value.
 */
float syn_lux_get_lux(const SYN_Lux *sensor);

/**
 * @brief Get Correlated Color Temperature in Kelvin.
 *
 * @param sensor Lux sensor context.
 * @return Color Temp in K.
 */
uint16_t syn_lux_get_color_temp_k(const SYN_Lux *sensor);

#ifdef __cplusplus
}
#endif

#endif /* SYN_LUX_H */
