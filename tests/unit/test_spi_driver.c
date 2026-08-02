/**
 * @file test_spi_driver.c
 * @brief Unity unit tests for hardware-decoupled Master/Slave SPI driver (syn_spi).
 */

#include "mocks/mock_port.h"
#include "syntropic/drivers/syn_spi.h"
#include "unity/unity.h"

extern bool mock_spi_init_ok;

static void test_spi_init_and_transfer(void)
{
    SYN_SPI spi;
    SYN_SPI_Config cfg = {.spi_id = 0,
                          .baudrate_hz = 2000000,
                          .mode = SYN_SPI_MODE_0,
                          .role = SYN_SPI_ROLE_MASTER,
                          .use_dma = false};

    /* Invalid parameters */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_spi_init(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_spi_init(&spi, NULL));

    /* Port init failure */
    mock_spi_init_ok = false;
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_spi_init(&spi, &cfg));
    mock_spi_init_ok = true;

    /* Valid initialization */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spi_init(&spi, &cfg));
    TEST_ASSERT_TRUE(spi.initialized);
    TEST_ASSERT_EQUAL_UINT8(0, spi.cfg.spi_id);
    TEST_ASSERT_EQUAL_UINT32(2000000, spi.cfg.baudrate_hz);

    /* Default baudrate fallback */
    SYN_SPI spi_default;
    SYN_SPI_Config cfg_zero_baud = {.spi_id = 1,
                                    .baudrate_hz = 0,
                                    .mode = SYN_SPI_MODE_3,
                                    .role = SYN_SPI_ROLE_SLAVE,
                                    .use_dma = false};
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spi_init(&spi_default, &cfg_zero_baud));
    TEST_ASSERT_EQUAL_UINT32(1000000, spi_default.cfg.baudrate_hz);
    syn_spi_deinit(&spi_default);

    /* Full duplex transfer */
    uint8_t tx_buf[] = {0x01, 0x02, 0x03};
    uint8_t rx_buf[3] = {0};
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_spi_transfer(NULL, tx_buf, rx_buf, 3));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_spi_transfer(&spi, tx_buf, rx_buf, 0));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spi_transfer(&spi, tx_buf, rx_buf, 3));

    /* Write & Read convenience wrappers */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spi_write(&spi, tx_buf, 3));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spi_read(&spi, rx_buf, 3));

    /* De-init */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_spi_deinit(NULL));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spi_deinit(&spi));
    TEST_ASSERT_FALSE(spi.initialized);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spi_deinit(&spi)); /* Uninitialized deinit */

    /* Transfer when uninitialized */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_spi_transfer(&spi, tx_buf, rx_buf, 3));
}

void run_spi_driver_tests(void)
{
    RUN_TEST(test_spi_init_and_transfer);
}
