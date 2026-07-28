/**
 * @file test_rc_curve.c
 * @brief Unit tests for Zero-Heap RC Expo & Dual-Rate Curve Mapper.
 */

#include "syntropic/control/syn_rc_curve.h"
#include "unity/unity.h"

void test_rc_curve_linear_no_deadband(void)
{
    SYN_RCCurve_Config cfg = {
        .deadband_us = 0,
        .expo = 0,           /* Linear */
        .dual_rate = Q16_ONE /* 100% rate */
    };

    TEST_ASSERT_EQUAL_UINT16(1500, syn_rc_curve_apply(1500, &cfg));
    TEST_ASSERT_EQUAL_UINT16(1000, syn_rc_curve_apply(1000, &cfg));
    TEST_ASSERT_EQUAL_UINT16(2000, syn_rc_curve_apply(2000, &cfg));
}

void test_rc_curve_deadband(void)
{
    SYN_RCCurve_Config cfg = {.deadband_us = 10, /* +/- 10us deadband */
                              .expo = 0,
                              .dual_rate = Q16_ONE};

    TEST_ASSERT_EQUAL_UINT16(1500, syn_rc_curve_apply(1500, &cfg));
    TEST_ASSERT_EQUAL_UINT16(1500, syn_rc_curve_apply(1508, &cfg));
    TEST_ASSERT_EQUAL_UINT16(1500, syn_rc_curve_apply(1492, &cfg));
}

void test_rc_curve_expo_and_dual_rate(void)
{
    SYN_RCCurve_Config cfg = {
        .deadband_us = 0,
        .expo = Q16_FROM_FLOAT(0.5f),     /* 50% Expo */
        .dual_rate = Q16_FROM_FLOAT(0.8f) /* 80% Dual Rate */
    };

    TEST_ASSERT_EQUAL_UINT16(1500, syn_rc_curve_apply(1500, &cfg));
    TEST_ASSERT_INT_WITHIN(1, 1100, syn_rc_curve_apply(1000, &cfg));
    TEST_ASSERT_INT_WITHIN(1, 1900, syn_rc_curve_apply(2000, &cfg));
}

void test_rc_curve_null_config(void)
{
    TEST_ASSERT_EQUAL_UINT16(1750, syn_rc_curve_apply(1750, NULL));
}
