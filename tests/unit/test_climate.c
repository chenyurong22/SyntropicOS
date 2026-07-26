/**
 * @file test_climate.c
 * @brief Unity tests for syn_climate module.
 */

#include "mocks/mock_port.h"
#include "syntropic/sensor/syn_climate.h"
#include "unity/unity.h"

static void test_climate_operations(void)
{
    mock_port_reset();
    SYN_Climate climate;

    SYN_Status st = syn_climate_init(&climate, 0, 1, 0x44, SYN_CLIMATE_SHT3X);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    /* Feed 25.0C, 50% RH, 1013.25 hPa */
    syn_climate_feed_sample(&climate, 25.0f, 50.0f, 1013.25f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 25.0f, syn_climate_get_temp_c(&climate));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 77.0f, syn_climate_get_temp_f(&climate));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 50.0f, syn_climate_get_humidity(&climate));
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 13.8f, syn_climate_get_dew_point(&climate));

    /* Zero humidity edge case */
    syn_climate_feed_sample(&climate, 20.0f, 0.0f, 1013.25f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 20.0f, syn_climate_get_dew_point(&climate));

    /* b + temp_c == 0 edge case (-237.7 C) */
    syn_climate_feed_sample(&climate, -237.7f, 50.0f, 1000.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, -237.7f, syn_climate_get_dew_point(&climate));

    /* NULL guards */
    syn_climate_feed_sample(NULL, 0.0f, 0.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, syn_climate_get_temp_c(NULL));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 32.0f, syn_climate_get_temp_f(NULL));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, syn_climate_get_humidity(NULL));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, syn_climate_get_dew_point(NULL));
}

void run_climate_tests(void)
{
    RUN_TEST(test_climate_operations);
}
