/**
 * @file test_i2c_driver.c
 * @brief Unity unit tests for hardware-decoupled Master/Slave I2C driver (syn_i2c).
 */

#include "mocks/mock_port.h"
#include "syntropic/drivers/syn_i2c.h"
#include "unity/unity.h"

extern bool mock_i2c_init_ok;

static void test_i2c_init_and_transfer(void)
{
    SYN_I2C i2c;
    SYN_I2C_Config cfg = {.i2c_id = 0,
                          .clock_speed_hz = 400000,
                          .role = SYN_I2C_ROLE_MASTER,
                          .own_address = 0x50,
                          .use_dma = false};

    /* Invalid parameters */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_i2c_init(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_i2c_init(&i2c, NULL));

    /* Port init failure */
    mock_i2c_init_ok = false;
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_i2c_init(&i2c, &cfg));
    mock_i2c_init_ok = true;

    /* Valid initialization */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_i2c_init(&i2c, &cfg));
    TEST_ASSERT_TRUE(i2c.initialized);
    TEST_ASSERT_EQUAL_UINT8(0, i2c.cfg.i2c_id);
    TEST_ASSERT_EQUAL_UINT32(400000, i2c.cfg.clock_speed_hz);

    /* Default clock speed fallback */
    SYN_I2C i2c_default;
    SYN_I2C_Config cfg_zero_speed = {.i2c_id = 1,
                                     .clock_speed_hz = 0,
                                     .role = SYN_I2C_ROLE_SLAVE,
                                     .own_address = 0x42,
                                     .use_dma = false};
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_i2c_init(&i2c_default, &cfg_zero_speed));
    TEST_ASSERT_EQUAL_UINT32(100000, i2c_default.cfg.clock_speed_hz);
    syn_i2c_deinit(&i2c_default);

    /* Transfer raw */
    uint8_t tx_buf[] = {0x10, 0x20};
    uint8_t rx_buf[2] = {0};
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_i2c_transfer(NULL, 0x50, tx_buf, 2, rx_buf, 2));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_i2c_transfer(&i2c, 0x50, tx_buf, 2, rx_buf, 2));

    /* Read & Write reg */
    uint8_t reg_val = 0;
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_i2c_read_reg(&i2c, 0x50, 0x01, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_i2c_write_reg(&i2c, 0x50, 0x01, 0xAA));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_i2c_read_reg(&i2c, 0x50, 0x01, &reg_val));

    /* De-init */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_i2c_deinit(NULL));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_i2c_deinit(&i2c));
    TEST_ASSERT_FALSE(i2c.initialized);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_i2c_deinit(&i2c)); /* Uninitialized deinit */

    /* Read when uninitialized */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_i2c_transfer(&i2c, 0x50, tx_buf, 2, rx_buf, 2));
}

void run_i2c_driver_tests(void)
{
    RUN_TEST(test_i2c_init_and_transfer);
}
