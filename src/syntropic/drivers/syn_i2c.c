#if __has_include("syn_config.h")
#include "syn_config.h"
#endif

#if !defined(SYN_USE_I2C) || SYN_USE_I2C

/**
 * @file syn_i2c.c
 * @brief Hardware-decoupled I2C driver implementation.
 */

#include "syn_i2c.h"
#include "../util/syn_assert.h"
#include <string.h>

SYN_Status syn_i2c_init(SYN_I2C *i2c, const SYN_I2C_Config *cfg)
{
    SYN_ASSERT(i2c != NULL);
    SYN_ASSERT(cfg != NULL);

    memset(i2c, 0, sizeof(*i2c));
    i2c->cfg = *cfg;

    if (i2c->cfg.clock_speed_hz == 0) {
        i2c->cfg.clock_speed_hz = 100000; /* Default 100 kHz */
    }

    SYN_Status status = syn_port_i2c_init(cfg->i2c_id, cfg->clock_speed_hz, (uint8_t)cfg->role, cfg->own_address);
    if (status != SYN_OK) {
        return status;
    }

    i2c->initialized = true;
    return SYN_OK;
}

SYN_Status syn_i2c_deinit(SYN_I2C *i2c)
{
    SYN_ASSERT(i2c != NULL);

    if (!i2c->initialized) {
        return SYN_OK;
    }

    SYN_Status status = syn_port_i2c_deinit(i2c->cfg.i2c_id);
    i2c->initialized = false;
    return status;
}

SYN_Status syn_i2c_transfer(SYN_I2C *i2c, uint16_t addr, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len)
{
    SYN_ASSERT(i2c != NULL);
    if (!i2c->initialized) {
        return SYN_INVALID_PARAM;
    }

    return syn_port_i2c_transfer(i2c->cfg.i2c_id, addr, tx, tx_len, rx, rx_len);
}

SYN_Status syn_i2c_read_reg(SYN_I2C *i2c, uint16_t addr, uint8_t reg, uint8_t *val)
{
    SYN_ASSERT(val != NULL);
    return syn_i2c_transfer(i2c, addr, &reg, 1, val, 1);
}

SYN_Status syn_i2c_write_reg(SYN_I2C *i2c, uint16_t addr, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return syn_i2c_transfer(i2c, addr, buf, 2, NULL, 0);
}

#endif /* SYN_USE_I2C */
