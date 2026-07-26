/**
 * @file test_distance.c
 * @brief Unity tests for syn_distance module.
 */

#include "mocks/mock_port.h"
#include "syntropic/sensor/syn_distance.h"
#include "unity/unity.h"

static void test_distance_operations(void)
{
    mock_port_reset();
    SYN_Distance dist;

    /* Ultrasonic Test */
    SYN_Status st = syn_distance_init(&dist, 0, 1, 20, 4000, SYN_DISTANCE_ULTRASONIC);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    syn_distance_set_proximity_threshold(&dist, 150);

    /* Feed 583 microseconds (~100mm) */
    syn_distance_feed_pulse(&dist, 583);
    TEST_ASSERT_EQUAL_UINT32(100, syn_distance_get_mm(&dist));
    TEST_ASSERT_EQUAL_UINT32(10, syn_distance_get_cm(&dist));
    TEST_ASSERT_TRUE(syn_distance_is_obstacle_detected(&dist));

    /* Feed 2915 microseconds (~500mm) */
    syn_distance_feed_pulse(&dist, 2915);
    TEST_ASSERT_EQUAL_UINT32(500, syn_distance_get_mm(&dist));
    TEST_ASSERT_FALSE(syn_distance_is_obstacle_detected(&dist));

    /* TOF Laser Test */
    st = syn_distance_init(&dist, 0, 1, 10, 2000, SYN_DISTANCE_TOF_LASER);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    syn_distance_feed_pulse(&dist, 250);
    TEST_ASSERT_EQUAL_UINT32(250, syn_distance_get_mm(&dist));

    /* NULL guards */
    syn_distance_set_proximity_threshold(NULL, 0);
    syn_distance_feed_pulse(NULL, 0);
    TEST_ASSERT_EQUAL_UINT32(0, syn_distance_get_mm(NULL));
    TEST_ASSERT_EQUAL_UINT32(0, syn_distance_get_cm(NULL));
    TEST_ASSERT_FALSE(syn_distance_is_obstacle_detected(NULL));
}

void run_distance_tests(void)
{
    RUN_TEST(test_distance_operations);
}
