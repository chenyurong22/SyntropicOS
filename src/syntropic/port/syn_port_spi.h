/**
 * @file syn_port_spi.h
 * @brief Port contract for Serial Peripheral Interface (SPI) hardware.
 * @ingroup syn_port
 */

#ifndef SYN_PORT_SPI_H
#define SYN_PORT_SPI_H

#include "../common/syn_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize an SPI hardware instance.
 *
 * @param spi_id       Instance index (0 = SPI1, 1 = SPI2).
 * @param baudrate_hz  Baudrate frequency in Hz (e.g. 1000000, 18000000).
 * @param mode         SPI Mode: 0 (CPOL=0, CPHA=0), 1 (CPOL=0, CPHA=1), 2 (CPOL=1, CPHA=0), 3
 * (CPOL=1, CPHA=1).
 * @param role         0 = Master, 1 = Slave.
 * @return SYN_OK on success.
 */
SYN_Status syn_port_spi_init(uint8_t spi_id, uint32_t baudrate_hz, uint8_t mode, uint8_t role);

/**
 * @brief De-initialize an SPI hardware instance.
 *
 * @param spi_id Instance index.
 * @return SYN_OK on success.
 */
SYN_Status syn_port_spi_deinit(uint8_t spi_id);

/**
 * @brief Perform a full-duplex simultaneous SPI transfer.
 *
 * @param spi_id  Instance index.
 * @param tx      TX data buffer (can be NULL, transmits 0xFF dummy bytes).
 * @param rx      RX destination buffer (can be NULL).
 * @param len     Transfer byte count.
 * @return SYN_OK on success.
 */
SYN_Status syn_port_spi_transfer(uint8_t spi_id, const uint8_t *tx, uint8_t *rx, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* SYN_PORT_SPI_H */
