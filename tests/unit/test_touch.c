/**
 * @file test_touch.c
 * @brief Unity tests for syn_touch module.
 */

#include "mocks/mock_port.h"
#include "syntropic/input/syn_touch.h"
#include "unity/unity.h"

static void test_touch_sensing(void)
{
    mock_port_reset();
    SYN_Touch t;

    SYN_Status st = syn_touch_init(&t, 0, 100);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_FALSE(syn_touch_is_pressed(&t));

    /* Test calibration */
    syn_touch_calibrate(&t, 500);
    TEST_ASSERT_EQUAL_UINT16(500, t.baseline);

    /* Test negative delta (raw sample < baseline: 300 < 500 -> delta = -(-200) = 200 >= threshold
     * 100) */
    syn_touch_feed_sample(&t, 300);
    TEST_ASSERT_TRUE(syn_touch_is_pressed(&t));

    /* Reset press state */
    t.is_pressed = false;

    /* Sample above threshold (500 + 150 = 650) */
    syn_touch_feed_sample(&t, 650);
    TEST_ASSERT_TRUE(syn_touch_is_pressed(&t));

    /* Sample drops back to baseline */
    syn_touch_feed_sample(&t, 500);
    TEST_ASSERT_FALSE(syn_touch_is_pressed(&t));

    /* Test baseline auto-initialization when baseline is 0 */
    SYN_Touch t2;
    syn_touch_init(&t2, 1, 50);
    t2.baseline = 0;
    syn_touch_feed_sample(&t2, 200);
    TEST_ASSERT_EQUAL_UINT16(200, t2.baseline);

    /* Test idle baseline drift tracking (delta < threshold) */
    t2.is_pressed = false;
    syn_touch_feed_sample(&t2, 210); /* delta = 10 < 50 */
    TEST_ASSERT_FALSE(syn_touch_is_pressed(&t2));

    /* Test getters */
    TEST_ASSERT_EQUAL_UINT32(2, t.press_count);

    /* NULL guards */
    syn_touch_calibrate(NULL, 100);
    syn_touch_feed_sample(NULL, 100);
    TEST_ASSERT_FALSE(syn_touch_is_pressed(NULL));
}

void run_touch_tests(void)
{
    RUN_TEST(test_touch_sensing);
}
