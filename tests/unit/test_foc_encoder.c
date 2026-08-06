/**
 * @file test_foc_encoder.c
 * @brief Unit tests for sensored FOC rotor position & speed interface.
 */

#include "syntropic/motor/syn_foc_encoder.h"
#include "unity/unity.h"

static void test_foc_encoder_init_invalid(void)
{
    SYN_FOCEncoder enc;
    SYN_FOCEncoderConfig cfg = {.type = SYN_FOC_ENCODER_QUADRATURE, .pole_pairs = 0, .cpr = 4000};
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_foc_encoder_init(NULL, &cfg));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_foc_encoder_init(&enc, NULL));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_foc_encoder_init(&enc, &cfg));
}

static void test_foc_encoder_quadrature_and_getters(void)
{
    SYN_FOCEncoder enc;
    SYN_FOCEncoderConfig cfg = {.type = SYN_FOC_ENCODER_QUADRATURE,
                                .pole_pairs = 4,
                                .cpr = 4000,
                                .zero_offset_rad = Q16_2_PI,
                                .sample_rate_hz = 10000};
    TEST_ASSERT_EQUAL(SYN_OK, syn_foc_encoder_init(&enc, &cfg));

    /* Test null guards */
    syn_foc_encoder_update_quadrature(NULL, 100);
    enc.config.cpr = 0;
    syn_foc_encoder_update_quadrature(&enc, 100);
    enc.config.cpr = 4000;

    /* Positive count and wrapping above 2PI */
    enc.config.zero_offset_rad = 10 * Q16_2_PI;
    syn_foc_encoder_update_quadrature(&enc, 5000);
    q16_t angle = syn_foc_encoder_get_elec_angle(&enc);
    TEST_ASSERT_GREATER_OR_EQUAL(0, angle);

    /* Negative raw_count and wrapping below 0 */
    enc.config.zero_offset_rad = -10 * Q16_2_PI;
    syn_foc_encoder_update_quadrature(&enc, -500);
    angle = syn_foc_encoder_get_elec_angle(&enc);
    TEST_ASSERT_GREATER_OR_EQUAL(0, angle);

    q16_t speed = syn_foc_encoder_get_elec_speed(&enc);
    (void)speed;

    TEST_ASSERT_EQUAL(0, syn_foc_encoder_get_elec_angle(NULL));
    TEST_ASSERT_EQUAL(0, syn_foc_encoder_get_elec_speed(NULL));
}

static void test_foc_encoder_hall(void)
{
    SYN_FOCEncoder enc;
    SYN_FOCEncoderConfig cfg = {
        .type = SYN_FOC_ENCODER_HALL, .pole_pairs = 4, .cpr = 6, .zero_offset_rad = Q16_2_PI};
    TEST_ASSERT_EQUAL(SYN_OK, syn_foc_encoder_init(&enc, &cfg));

    /* State 4 (100) with offset > 2PI */
    syn_foc_encoder_update_hall(&enc, true, false, false);
    TEST_ASSERT_EQUAL(4, enc.hall_state);
    TEST_ASSERT_GREATER_OR_EQUAL(0, syn_foc_encoder_get_elec_angle(&enc));

    /* State 1 with negative offset */
    enc.config.zero_offset_rad = -Q16_2_PI;
    syn_foc_encoder_update_hall(&enc, false, false, true);

    syn_foc_encoder_update_hall(NULL, true, true, true);
}

static void test_foc_encoder_absolute(void)
{
    SYN_FOCEncoder enc;
    SYN_FOCEncoderConfig cfg = {.type = SYN_FOC_ENCODER_ABSOLUTE,
                                .pole_pairs = 2,
                                .cpr = 16384,
                                .zero_offset_rad = Q16_2_PI};
    TEST_ASSERT_EQUAL(SYN_OK, syn_foc_encoder_init(&enc, &cfg));

    syn_foc_encoder_update_absolute(&enc, 16000);
    TEST_ASSERT_GREATER_OR_EQUAL(0, syn_foc_encoder_get_elec_angle(&enc));

    enc.config.zero_offset_rad = -Q16_2_PI;
    syn_foc_encoder_update_absolute(&enc, 100);
    TEST_ASSERT_GREATER_OR_EQUAL(0, syn_foc_encoder_get_elec_angle(&enc));

    syn_foc_encoder_update_absolute(NULL, 1000);
}

void run_foc_encoder_tests(void)
{
    RUN_TEST(test_foc_encoder_init_invalid);
    RUN_TEST(test_foc_encoder_quadrature_and_getters);
    RUN_TEST(test_foc_encoder_hall);
    RUN_TEST(test_foc_encoder_absolute);
}
