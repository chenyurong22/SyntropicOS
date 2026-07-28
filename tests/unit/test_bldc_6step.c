/**
 * @file test_bldc_6step.c
 * @brief Unity tests for Zero-Heap 6-Step BLDC Motor Commutation Driver.
 */

#include "syntropic/motor/syn_bldc_6step.h"
#include "unity/unity.h"

void test_bldc_6step_init_defaults(void)
{
    SYN_BLDC_6Step bldc;
    syn_bldc_6step_init(&bldc, NULL);

    TEST_ASSERT_EQUAL_INT(SYN_BLDC_STATE_STOPPED, bldc.state);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_DIR_CW, bldc.direction);
    TEST_ASSERT_EQUAL_UINT8(4, bldc.config.pole_pairs);
    TEST_ASSERT_EQUAL_UINT16(0, bldc.duty);
    TEST_ASSERT_EQUAL_UINT32(0, bldc.rpm);
}

void test_bldc_6step_hall_commutation_cw(void)
{
    SYN_BLDC_6Step bldc;
    syn_bldc_6step_init(&bldc, NULL);

    SYN_BLDC_PhaseOutputs out;
    syn_bldc_6step_start(&bldc);
    syn_bldc_6step_set_duty(&bldc, 500);

    /* Step 1: Hall 0b101(5) -> U+ V- */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_bldc_6step_set_hall(&bldc, 5, &out));
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_PWM, out.u);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_LOW, out.v);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_OFF, out.w);
    TEST_ASSERT_EQUAL_UINT16(500, out.duty);

    /* Step 2: Hall 0b001(1) -> U+ W- */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_bldc_6step_set_hall(&bldc, 1, &out));
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_PWM, out.u);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_OFF, out.v);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_LOW, out.w);

    /* Step 3: Hall 0b011(3) -> V+ W- */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_bldc_6step_set_hall(&bldc, 3, &out));
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_OFF, out.u);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_PWM, out.v);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_LOW, out.w);

    /* Step 4: Hall 0b010(2) -> V+ U- */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_bldc_6step_set_hall(&bldc, 2, &out));
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_LOW, out.u);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_PWM, out.v);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_OFF, out.w);

    /* Step 5: Hall 0b110(6) -> W+ U- */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_bldc_6step_set_hall(&bldc, 6, &out));
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_LOW, out.u);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_OFF, out.v);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_PWM, out.w);

    /* Step 6: Hall 0b100(4) -> W+ V- */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_bldc_6step_set_hall(&bldc, 4, &out));
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_OFF, out.u);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_LOW, out.v);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_PWM, out.w);
}

void test_bldc_6step_invalid_hall_fault(void)
{
    SYN_BLDC_6Step bldc;
    syn_bldc_6step_init(&bldc, NULL);

    SYN_BLDC_PhaseOutputs out;
    syn_bldc_6step_start(&bldc);

    /* Invalid Hall 0b000 */
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_bldc_6step_set_hall(&bldc, 0, &out));
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_STATE_FAULT, bldc.state);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_OFF, out.u);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_OFF, out.v);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_OFF, out.w);

    /* Invalid Hall 0b111 */
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_bldc_6step_set_hall(&bldc, 7, &out));
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_STATE_FAULT, bldc.state);
}

void test_bldc_6step_direction_and_stop(void)
{
    SYN_BLDC_6Step bldc;
    syn_bldc_6step_init(&bldc, NULL);

    SYN_BLDC_PhaseOutputs out;
    syn_bldc_6step_start(&bldc);
    syn_bldc_6step_set_direction(&bldc, SYN_BLDC_DIR_CCW);
    syn_bldc_6step_set_duty(&bldc, 750);

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_bldc_6step_set_hall(&bldc, 5, &out));
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_LOW, out.u);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_PWM, out.v);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_OFF, out.w);

    syn_bldc_6step_stop(&bldc, &out);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_STATE_STOPPED, bldc.state);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_OFF, out.u);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_OFF, out.v);
    TEST_ASSERT_EQUAL_INT(SYN_BLDC_GATE_OFF, out.w);
}

void test_bldc_6step_speed_calculation(void)
{
    SYN_BLDC_6Step bldc;
    syn_bldc_6step_init(&bldc, NULL);
    syn_bldc_6step_start(&bldc);

    /* Initial tick */
    syn_bldc_6step_update_speed(&bldc, 1000, 0);

    /* Simulate 48 Hall transitions in 100ms (1000ms -> 1100ms) */
    SYN_BLDC_PhaseOutputs out;
    for (int i = 0; i < 48; i++) {
        syn_bldc_6step_set_hall(&bldc, 5, &out);
    }

    uint32_t rpm = syn_bldc_6step_update_speed(&bldc, 1100, 0);
    /* 48 transitions / (6 * 4 pole pairs) = 2 mechanical revs in 100ms = 1200 RPM */
    TEST_ASSERT_EQUAL_UINT32(1200, rpm);
}
