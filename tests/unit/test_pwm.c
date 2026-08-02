/**
 * @file test_pwm.c
 * @brief Unit tests for Hardware PWM and High-Resolution PWM drivers.
 */

#include "syntropic/drivers/syn_hrpwm.h"
#include "syntropic/drivers/syn_pwm.h"
#include "unity/unity.h"

static void test_pwm_init_and_control(void)
{
    SYN_PWM pwm;

    /* Invalid parameters */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_pwm_init(NULL, 0, 1000));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_pwm_init(&pwm, 0, 0));

    /* Valid initialization */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_pwm_init(&pwm, 1, 5000));
    TEST_ASSERT_EQUAL_UINT8(1, pwm.channel);
    TEST_ASSERT_EQUAL_UINT32(5000, pwm.freq_hz);

    /* Duty cycle percentage control & clamping */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_pwm_set_duty(NULL, 50));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_pwm_set_duty(&pwm, 50));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_pwm_set_duty(&pwm, 120)); /* Clamped to 100% */

    /* Duty cycle raw 16-bit control */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_pwm_set_duty_raw(NULL, 32768));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_pwm_set_duty_raw(&pwm, 32768));

    /* Frequency runtime change */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_pwm_set_freq(NULL, 10000));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_pwm_set_freq(&pwm, 0));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_pwm_set_freq(&pwm, 10000));
    TEST_ASSERT_EQUAL_UINT32(10000, pwm.freq_hz);

    /* Output enable / disable */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_pwm_enable(NULL, true));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_pwm_enable(&pwm, true));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_pwm_enable(&pwm, false));
}

static void test_hrpwm_init_and_control(void)
{
    SYN_HRPWM hrpwm;

    /* Invalid parameters */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_hrpwm_init(NULL, 0, 500000));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_hrpwm_init(&hrpwm, 0, 0));

    /* Valid initialization */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_hrpwm_init(&hrpwm, 2, 200000));
    TEST_ASSERT_EQUAL_UINT8(2, hrpwm.channel);
    TEST_ASSERT_EQUAL_UINT32(200000, hrpwm.freq_hz);
    TEST_ASSERT_FALSE(hrpwm.enabled);

    /* Duty cycle Q16 control & clamping */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_hrpwm_set_duty_q16(NULL, 32768));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_hrpwm_set_duty_q16(&hrpwm, -100));   /* Clamped to 0 */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_hrpwm_set_duty_q16(&hrpwm, 100000)); /* Clamped to 65536 */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_hrpwm_set_duty_q16(&hrpwm, 32768));  /* 50% */

    /* Duty cycle float control & clamping */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_hrpwm_set_duty_float(NULL, 0.5f));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_hrpwm_set_duty_float(&hrpwm, -0.5f)); /* Clamped to 0.0 */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_hrpwm_set_duty_float(&hrpwm, 1.5f));  /* Clamped to 1.0 */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_hrpwm_set_duty_float(&hrpwm, 0.75f));

    /* Dead-time insertion */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_hrpwm_set_deadtime_ns(NULL, 100, 100));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_hrpwm_set_deadtime_ns(&hrpwm, 120, 150));
    TEST_ASSERT_EQUAL_UINT16(120, hrpwm.rise_ns);
    TEST_ASSERT_EQUAL_UINT16(150, hrpwm.fall_ns);

    /* Phase shift */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_hrpwm_set_phase_deg(NULL, 90));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_hrpwm_set_phase_deg(&hrpwm, 361));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_hrpwm_set_phase_deg(&hrpwm, 180));
    TEST_ASSERT_EQUAL_UINT16(180, hrpwm.phase_deg);

    /* Fault trip binding */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_hrpwm_bind_fault(NULL, 1));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_hrpwm_bind_fault(&hrpwm, 1));

    /* Enable / disable */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_hrpwm_enable(NULL, true));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_hrpwm_enable(&hrpwm, true));
    TEST_ASSERT_TRUE(hrpwm.enabled);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_hrpwm_enable(&hrpwm, false));
    TEST_ASSERT_FALSE(hrpwm.enabled);
}

void run_pwm_tests(void)
{
    RUN_TEST(test_pwm_init_and_control);
    RUN_TEST(test_hrpwm_init_and_control);
}
