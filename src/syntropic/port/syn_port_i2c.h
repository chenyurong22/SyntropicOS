/**
 * @file syn_port_i2c.h
 * @brief Port contract for Inter-Integrated Circuit (I2C) hardware.
 * @ingroup syn_port
 */

#ifndef SYN_PORT_I2C_H
#define SYN_PORT_I2C_H

#include "../common/syn_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize an I2C hardware instance.
 *
 * @param i2c_id          Instance index (0 = I2C1, 1 = I2C2).
 * @param clock_speed_hz  Bus clock frequency in Hz (e.g. 100000, 400000).
 * @param role            0 = Master, 1 = Slave.
 * @param own_addr        7-bit own slave address (used in Slave mode).
 * @return SYN_OK on success.
 */
SYN_Status syn_port_i2c_init(uint8_t i2c_id, uint32_t clock_speed_hz, uint8_t role, uint16_t own_addr);

/**
 * @brief De-initialize an I2C hardware instance.
 *
 * @param i2c_id Instance index.
 * @return SYN_OK on success.
 */
SYN_Status syn_port_i2c_deinit(uint8_t i2c_id);

/**
 * @brief Perform an I2C transaction (Write, Read, or Write-Then-Read restart).
 *
 * @param i2c_id  Instance index.
 * @param addr    Target 7-bit slave address.
 * @param tx      TX data buffer (can be NULL if rx_len > 0).
 * @param tx_len  Bytes to transmit.
 * @param rx      RX destination buffer (can be NULL if tx_len > 0).
 * @param rx_len  Bytes to receive.
 * @return SYN_OK on success.
 */
SYN_Status syn_port_i2c_transfer(uint8_t i2c_id, uint16_t addr, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len);

#ifdef __cplusplus
}
#endif

#endif /* SYN_PORT_I2C_H */
