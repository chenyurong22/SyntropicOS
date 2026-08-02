/**
 * @file syn_i2c.h
 * @brief Hardware-decoupled I2C driver supporting Master/Slave roles & register access.
 * @ingroup syn_drivers
 */

#ifndef SYN_I2C_H
#define SYN_I2C_H

#include "../common/syn_defs.h"
#include "../port/syn_port_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYN_I2C_ROLE_MASTER = 0,
    SYN_I2C_ROLE_SLAVE
} SYN_I2C_Role;

typedef struct {
    uint8_t i2c_id;           /**< Hardware I2C instance (0 = I2C1) */
    uint32_t clock_speed_hz;  /**< Bus speed in Hz (e.g. 100000, 400000) */
    SYN_I2C_Role role;        /**< Master or Slave mode */
    uint16_t own_address;     /**< 7-bit own slave address */
    bool use_dma;             /**< Enable DMA transfers */
} SYN_I2C_Config;

typedef struct {
    SYN_I2C_Config cfg;       /**< Instance configuration */
    bool initialized;         /**< Initialization state */
} SYN_I2C;

/**
 * @brief Initialize an I2C instance.
 *
 * @param i2c Pointer to SYN_I2C handle.
 * @param cfg Configuration structure.
 * @return SYN_OK on success.
 */
SYN_Status syn_i2c_init(SYN_I2C *i2c, const SYN_I2C_Config *cfg);

/**
 * @brief De-initialize an I2C instance.
 *
 * @param i2c Pointer to SYN_I2C handle.
 * @return SYN_OK on success.
 */
SYN_Status syn_i2c_deinit(SYN_I2C *i2c);

/**
 * @brief Perform raw buffer transfer (Write, Read, or Write-Read restart).
 *
 * @param i2c     Pointer to initialized I2C handle.
 * @param addr    Target 7-bit slave address.
 * @param tx      TX data buffer (can be NULL).
 * @param tx_len  TX byte count.
 * @param rx      RX data buffer (can be NULL).
 * @param rx_len  RX byte count.
 * @return SYN_OK on success.
 */
SYN_Status syn_i2c_transfer(SYN_I2C *i2c, uint16_t addr, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len);

/**
 * @brief Read an 8-bit register from a slave device.
 *
 * @param i2c   Pointer to initialized I2C handle.
 * @param addr  7-bit slave address.
 * @param reg   Register index byte.
 * @param val   Pointer to store output register byte.
 * @return SYN_OK on success.
 */
SYN_Status syn_i2c_read_reg(SYN_I2C *i2c, uint16_t addr, uint8_t reg, uint8_t *val);

/**
 * @brief Write an 8-bit register on a slave device.
 *
 * @param i2c   Pointer to initialized I2C handle.
 * @param addr  7-bit slave address.
 * @param reg   Register index byte.
 * @param val   Byte to write into register.
 * @return SYN_OK on success.
 */
SYN_Status syn_i2c_write_reg(SYN_I2C *i2c, uint16_t addr, uint8_t reg, uint8_t val);

#ifdef __cplusplus
}
#endif

#endif /* SYN_I2C_H */
