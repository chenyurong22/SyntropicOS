/**
 * @file syn_powermon.h
 * @brief Generic Power & Current Monitor Driver (INA219, INA226, INA3221).
 * @ingroup syn_sensor
 */

#ifndef SYN_POWERMON_H
#define SYN_POWERMON_H

#include "../common/syn_defs.h"
#include "../drivers/syn_soft_i2c.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Power Monitor IC Type.
 */
typedef enum {
    SYN_POWERMON_INA219 = 0, /**< INA219 Bidirectional Current/Power Monitor */
    SYN_POWERMON_INA226 = 1  /**< INA226 High Precision Current/Power Monitor */
} SYN_PowerMonType;

/**
 * @brief Generic Power Monitor Instance Context.
 */
typedef struct {
    SYN_PowerMonType type;
    SYN_SoftI2C i2c;
    uint8_t i2c_addr;
    float shunt_resistor_ohms;  /**< Shunt resistor value (e.g. 0.1 ohms) */
    float bus_voltage_v;        /**< Measured Bus Voltage in Volts */
    float shunt_current_ma;     /**< Measured Shunt Current in Milliamps */
    float power_mw;             /**< Measured Active Power in Milliwatts */
    float energy_mwh;           /**< Accumulated Energy in Milliwatt-hours */
} SYN_PowerMon;

/**
 * @brief Initialize Power Monitor IC.
 *
 * @param pm                  Power monitor context.
 * @param scl                 I2C SCL GPIO pin.
 * @param sda                 I2C SDA GPIO pin.
 * @param i2c_addr            I2C slave address (e.g. 0x40 or 0x41).
 * @param shunt_resistor_ohms Shunt resistor value in Ohms (e.g. 0.1f).
 * @param type                IC type (INA219 or INA226).
 * @return SYN_OK on success.
 */
SYN_Status syn_powermon_init(SYN_PowerMon *pm, SYN_GPIO_Pin scl, SYN_GPIO_Pin sda,
                             uint8_t i2c_addr, float shunt_resistor_ohms, SYN_PowerMonType type);

/**
 * @brief Feed raw I2C register samples (bus voltage & shunt voltage).
 *
 * @param pm            Power monitor context.
 * @param raw_bus_v     Raw bus voltage register value.
 * @param raw_shunt_mv  Raw shunt voltage in millivolts.
 */
void syn_powermon_feed_raw(SYN_PowerMon *pm, uint16_t raw_bus_v, float raw_shunt_mv);

/**
 * @brief Get measured bus voltage in Volts.
 *
 * @param pm Power monitor context.
 * @return Voltage in Volts.
 */
float syn_powermon_get_bus_voltage(const SYN_PowerMon *pm);

/**
 * @brief Get measured current in Milliamps.
 *
 * @param pm Power monitor context.
 * @return Current in mA.
 */
float syn_powermon_get_current_ma(const SYN_PowerMon *pm);

/**
 * @brief Get calculated power in Milliwatts.
 *
 * @param pm Power monitor context.
 * @return Power in mW.
 */
float syn_powermon_get_power_mw(const SYN_PowerMon *pm);

#ifdef __cplusplus
}
#endif

#endif /* SYN_POWERMON_H */
