#if __has_include("syn_config.h")
#include "syn_config.h"
#endif

#if !defined(SYN_USE_SPI) || SYN_USE_SPI

/**
 * @file syn_spi.c
 * @brief Hardware-decoupled SPI driver implementation.
 */

#include "../util/syn_assert.h"
#include "syn_spi.h"

#include <string.h>

SYN_Status syn_spi_init(SYN_SPI *spi, const SYN_SPI_Config *cfg)
{
    if (spi == NULL || cfg == NULL) {
        return SYN_INVALID_PARAM;
    }

    memset(spi, 0, sizeof(*spi));
    spi->cfg = *cfg;

    if (spi->cfg.baudrate_hz == 0) {
        spi->cfg.baudrate_hz = 1000000; /* Default 1 MHz */
    }

    SYN_Status status =
        syn_port_spi_init(cfg->spi_id, cfg->baudrate_hz, (uint8_t)cfg->mode, (uint8_t)cfg->role);
    if (status != SYN_OK) {
        return status;
    }

    spi->initialized = true;
    return SYN_OK;
}

SYN_Status syn_spi_deinit(SYN_SPI *spi)
{
    if (spi == NULL) {
        return SYN_INVALID_PARAM;
    }

    if (!spi->initialized) {
        return SYN_OK;
    }

    SYN_Status status = syn_port_spi_deinit(spi->cfg.spi_id);
    spi->initialized = false;
    return status;
}

SYN_Status syn_spi_transfer(SYN_SPI *spi, const uint8_t *tx, uint8_t *rx, size_t len)
{
    if (spi == NULL || !spi->initialized || len == 0) {
        return SYN_INVALID_PARAM;
    }

    return syn_port_spi_transfer(spi->cfg.spi_id, tx, rx, len);
}

SYN_Status syn_spi_write(SYN_SPI *spi, const uint8_t *tx, size_t len)
{
    return syn_spi_transfer(spi, tx, NULL, len);
}

SYN_Status syn_spi_read(SYN_SPI *spi, uint8_t *rx, size_t len)
{
    return syn_spi_transfer(spi, NULL, rx, len);
}

#endif /* SYN_USE_SPI */
