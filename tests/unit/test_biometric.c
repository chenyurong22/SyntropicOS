/**
 * @file test_biometric.c
 * @brief Unity tests for syn_biometric module.
 */

#include "mocks/mock_port.h"
#include "syntropic/sensor/syn_biometric.h"
#include "unity/unity.h"

static void test_biometric_operations(void)
{
    mock_port_reset();
    SYN_Biometric bio;

    SYN_Status st = syn_biometric_init(&bio, 0, 1, 0x57, SYN_BIOMETRIC_MAX30102);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    /* Test no finger (< 50000) */
    syn_biometric_feed_samples(&bio, 10000, 10000);
    TEST_ASSERT_FALSE(syn_biometric_is_finger_detected(&bio));

    /* Test finger present */
    syn_biometric_feed_samples(&bio, 150000, 160000);
    TEST_ASSERT_TRUE(syn_biometric_is_finger_detected(&bio));
    TEST_ASSERT_EQUAL_UINT16(72, syn_biometric_get_bpm(&bio));
    TEST_ASSERT_GREATER_THAN(80.0f, syn_biometric_get_spo2(&bio));

    /* Test SpO2 upper clamping (> 100%) */
    syn_biometric_feed_samples(&bio, 10000, 100000);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, syn_biometric_get_spo2(&bio));

    /* Test SpO2 lower clamping (< 70%) */
    syn_biometric_feed_samples(&bio, 300000, 60000);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 70.0f, syn_biometric_get_spo2(&bio));

    /* NULL guards */
    syn_biometric_feed_samples(NULL, 0, 0);
    TEST_ASSERT_EQUAL_UINT16(0, syn_biometric_get_bpm(NULL));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, syn_biometric_get_spo2(NULL));
    TEST_ASSERT_FALSE(syn_biometric_is_finger_detected(NULL));
}

void run_biometric_tests(void)
{
    RUN_TEST(test_biometric_operations);
}
