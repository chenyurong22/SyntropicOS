/**
 * @file test_lux.c
 * @brief Unity tests for syn_lux module.
 */

#include "mocks/mock_port.h"
#include "syntropic/sensor/syn_lux.h"
#include "unity/unity.h"

static void test_lux_operations(void)
{
    mock_port_reset();
    SYN_Lux lux;

    /* BH1750 Test */
    SYN_Status st = syn_lux_init(&lux, 0, 1, 0x23, SYN_LUX_BH1750);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    syn_lux_feed_lux(&lux, 450.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 450.5f, syn_lux_get_lux(&lux));

    /* TCS34725 Test */
    st = syn_lux_init(&lux, 0, 1, 0x29, SYN_LUX_TCS34725);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    syn_lux_feed_rgbc(&lux, 1000, 2000, 1500, 4500);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 2250.0f, syn_lux_get_lux(&lux));
    TEST_ASSERT_GREATER_THAN(1000, syn_lux_get_color_temp_k(&lux));

    /* Zero clear channel (c = 0) */
    syn_lux_feed_rgbc(&lux, 10, 10, 10, 0);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, syn_lux_get_lux(&lux));

    /* CCT clamping low (cct < 1000K) */
    syn_lux_feed_rgbc(&lux, 500, 200, 0, 1000);
    TEST_ASSERT_EQUAL_UINT16(1000, syn_lux_get_color_temp_k(&lux));

    /* CCT clamping high (cct > 20000K) */
    syn_lux_feed_rgbc(&lux, 0, 3000, 6230, 1000);
    TEST_ASSERT_EQUAL_UINT16(20000, syn_lux_get_color_temp_k(&lux));

    /* Singularity branch (y ≈ 0.1858) */
    syn_lux_feed_rgbc(&lux, 1000, 200, 1000, 1000);

    /* NULL guards */
    syn_lux_feed_lux(NULL, 0.0f);
    syn_lux_feed_rgbc(NULL, 0, 0, 0, 0);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, syn_lux_get_lux(NULL));
    TEST_ASSERT_EQUAL_UINT16(0, syn_lux_get_color_temp_k(NULL));
}

void run_lux_tests(void)
{
    RUN_TEST(test_lux_operations);
}
