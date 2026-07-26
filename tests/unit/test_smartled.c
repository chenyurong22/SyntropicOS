/**
 * @file test_smartled.c
 * @brief Unity tests for syn_smartled module.
 */

#include "mocks/mock_port.h"
#include "syntropic/output/syn_smartled.h"
#include "unity/unity.h"

static void test_smartled_operations(void)
{
    mock_port_reset();
    SYN_SmartLED led;
    SYN_SmartLEDColor buf[10];

    SYN_Status st = syn_smartled_init(&led, 5, 10, buf, SYN_SMARTLED_ORDER_GRB);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    /* Set brightness to 50% (128) */
    syn_smartled_set_brightness(&led, 128);

    /* Set Pixel 0 to Red (255, 0, 0) -> scaled to ~128 */
    syn_smartled_set_pixel_rgb(&led, 0, 255, 0, 0);
    TEST_ASSERT_EQUAL_UINT8(128, buf[0].r);

    /* Set Pixel 1 to HSV Hue 0 (Red - Region 0) */
    syn_smartled_set_pixel_hsv(&led, 1, 0, 255, 255);
    TEST_ASSERT_EQUAL_UINT8(128, buf[1].r);

    /* HSV Region 1 (Hue 50) */
    syn_smartled_set_pixel_hsv(&led, 2, 50, 255, 255);

    /* HSV Region 2 (Hue 90) */
    syn_smartled_set_pixel_hsv(&led, 3, 90, 255, 255);

    /* HSV Region 3 (Hue 135) */
    syn_smartled_set_pixel_hsv(&led, 4, 135, 255, 255);

    /* HSV Region 4 (Hue 180) */
    syn_smartled_set_pixel_hsv(&led, 5, 180, 255, 255);

    /* HSV Region 5 (Default - Hue 220) */
    syn_smartled_set_pixel_hsv(&led, 6, 220, 255, 255);

    /* Set Pixel 7 to HSV Saturation 0 (White) */
    syn_smartled_set_pixel_hsv(&led, 7, 0, 0, 255);

    /* Fill strip */
    syn_smartled_fill_rgb(&led, 0, 255, 0);

    /* Clear strip */
    syn_smartled_clear(&led);
    TEST_ASSERT_EQUAL_UINT8(0, buf[0].r);

    /* NULL guards */
    syn_smartled_set_brightness(NULL, 0);
    syn_smartled_set_pixel_rgb(NULL, 0, 0, 0, 0);
    syn_smartled_set_pixel_rgb(&led, 999, 0, 0, 0); /* OOB */
    syn_smartled_set_pixel_hsv(NULL, 0, 0, 0, 0);
    syn_smartled_set_pixel_hsv(&led, 999, 0, 0, 0); /* OOB */
    syn_smartled_fill_rgb(NULL, 0, 0, 0);
    syn_smartled_clear(NULL);
}

void run_smartled_tests(void)
{
    RUN_TEST(test_smartled_operations);
}
