/**
 * @file test_joystick.c
 * @brief Unity tests for syn_joystick module.
 */

#include "mocks/mock_port.h"
#include "syntropic/input/syn_joystick.h"
#include "unity/unity.h"

static void test_joystick_operations(void)
{
    mock_port_reset();
    SYN_Joystick joy;

    SYN_Status st = syn_joystick_init(&joy, 2048, 2048, 4095, 100);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    /* Test Center */
    syn_joystick_feed_adc(&joy, 2048, 2048, false);
    TEST_ASSERT_EQUAL_INT16(0, syn_joystick_get_x_pct(&joy));
    TEST_ASSERT_EQUAL_INT16(0, syn_joystick_get_y_pct(&joy));
    TEST_ASSERT_EQUAL(SYN_JOYSTICK_DIR_CENTER, syn_joystick_get_dir(&joy));

    /* Test Up Right */
    syn_joystick_feed_adc(&joy, 3500, 3500, true);
    TEST_ASSERT_TRUE(joy.button_pressed);
    TEST_ASSERT_EQUAL(SYN_JOYSTICK_DIR_UP_RIGHT, syn_joystick_get_dir(&joy));

    /* Test Up Left */
    syn_joystick_feed_adc(&joy, 500, 3500, false);
    TEST_ASSERT_EQUAL(SYN_JOYSTICK_DIR_UP_LEFT, syn_joystick_get_dir(&joy));

    /* Test Down Right */
    syn_joystick_feed_adc(&joy, 3500, 500, false);
    TEST_ASSERT_EQUAL(SYN_JOYSTICK_DIR_DOWN_RIGHT, syn_joystick_get_dir(&joy));

    /* Test Down Left */
    syn_joystick_feed_adc(&joy, 500, 500, false);
    TEST_ASSERT_EQUAL(SYN_JOYSTICK_DIR_DOWN_LEFT, syn_joystick_get_dir(&joy));

    /* Test Straight Directions */
    syn_joystick_feed_adc(&joy, 2048, 3800, false);
    TEST_ASSERT_EQUAL(SYN_JOYSTICK_DIR_UP, syn_joystick_get_dir(&joy));

    syn_joystick_feed_adc(&joy, 2048, 200, false);
    TEST_ASSERT_EQUAL(SYN_JOYSTICK_DIR_DOWN, syn_joystick_get_dir(&joy));

    syn_joystick_feed_adc(&joy, 3800, 2048, false);
    TEST_ASSERT_EQUAL(SYN_JOYSTICK_DIR_RIGHT, syn_joystick_get_dir(&joy));

    syn_joystick_feed_adc(&joy, 200, 2048, false);
    TEST_ASSERT_EQUAL(SYN_JOYSTICK_DIR_LEFT, syn_joystick_get_dir(&joy));

    /* NULL guards */
    syn_joystick_feed_adc(NULL, 0, 0, false);
    TEST_ASSERT_EQUAL_INT16(0, syn_joystick_get_x_pct(NULL));
    TEST_ASSERT_EQUAL_INT16(0, syn_joystick_get_y_pct(NULL));
    TEST_ASSERT_EQUAL(SYN_JOYSTICK_DIR_CENTER, syn_joystick_get_dir(NULL));
}

void run_joystick_tests(void)
{
    RUN_TEST(test_joystick_operations);
}
