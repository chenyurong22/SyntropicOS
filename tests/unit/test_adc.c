/**
 * @file test_adc.c
 * @brief Unity tests for general-purpose 12-bit ADC driver (syn_adc).
 */

#include "mocks/mock_port.h"
#include "syntropic/drivers/syn_adc.h"
#include "unity/unity.h"

extern bool mock_adc_init_ok;

static void test_adc_init_and_read(void)
{
    SYN_ADC adc;
    SYN_ADC_Config cfg = {.adc_id = 0, .channel_mask = 0x01, .vref_mv = 3300, .use_dma = false};

    /* Invalid parameters */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_adc_init(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_adc_init(&adc, NULL));

    /* Port init failure */
    mock_adc_init_ok = false;
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_adc_init(&adc, &cfg));
    mock_adc_init_ok = true;

    /* Valid initialization */
    mock_adc_value = 2048;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_adc_init(&adc, &cfg));
    TEST_ASSERT_TRUE(adc.initialized);
    TEST_ASSERT_EQUAL_UINT8(0, adc.cfg.adc_id);
    TEST_ASSERT_EQUAL_UINT32(3300, adc.cfg.vref_mv);

    /* Default vref_mv fallback */
    SYN_ADC adc_default;
    SYN_ADC_Config cfg_zero_vref = {
        .adc_id = 1, .channel_mask = 0x03, .vref_mv = 0, .use_dma = false};
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_adc_init(&adc_default, &cfg_zero_vref));
    TEST_ASSERT_EQUAL_UINT32(3300, adc_default.cfg.vref_mv);
    syn_adc_deinit(&adc_default);

    /* Read raw count */
    TEST_ASSERT_EQUAL_UINT16(0, syn_adc_read_raw(NULL, 0));
    TEST_ASSERT_EQUAL_UINT16(2048, syn_adc_read_raw(&adc, 0));

    /* Read calibrated mV (2048 * 3300 / 4095 = 1649) */
    TEST_ASSERT_EQUAL_UINT32(0, syn_adc_read_mv(NULL, 0));
    uint32_t mv = syn_adc_read_mv(&adc, 0);
    TEST_ASSERT_TRUE(mv >= 1640 && mv <= 1660);

    /* DMA scan */
    uint16_t buf[4] = {0};
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_adc_start_dma_scan(NULL, buf, 4));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_adc_start_dma_scan(&adc, NULL, 4));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_adc_start_dma_scan(&adc, buf, 4));

    /* De-init */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_adc_deinit(NULL));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_adc_deinit(&adc));
    TEST_ASSERT_FALSE(adc.initialized);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_adc_deinit(&adc)); /* Uninitialized deinit */

    /* Read when uninitialized */
    TEST_ASSERT_EQUAL_UINT16(0, syn_adc_read_raw(&adc, 0));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_adc_start_dma_scan(&adc, buf, 4));
}

void run_adc_tests(void)
{
    RUN_TEST(test_adc_init_and_read);
}
