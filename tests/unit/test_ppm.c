/**
 * @file test_ppm.c
 * @brief Unit tests for Zero-Heap PPM Multi-Channel RC Receiver Decoder.
 */

#include "syntropic/input/syn_ppm.h"
#include "unity/unity.h"

void test_ppm_init(void)
{
    SYN_PPM_Decoder ppm;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_ppm_init(&ppm));
    TEST_ASSERT_EQUAL_UINT8(0, ppm.channel_count);
    TEST_ASSERT_EQUAL_UINT32(0, ppm.frames_received);
    TEST_ASSERT_EQUAL_UINT16(1500, syn_ppm_get_channel(&ppm, 0));
}

void test_ppm_process_frame(void)
{
    SYN_PPM_Decoder ppm;
    syn_ppm_init(&ppm);

    /* Send Sync gap > 2700 us */
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_ppm_process_pulse(&ppm, 4000));
    TEST_ASSERT_TRUE(ppm.in_frame);

    /* Send 6 channels */
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_ppm_process_pulse(&ppm, 1000)); /* Ch 0: 1000 us */
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_ppm_process_pulse(&ppm, 1500)); /* Ch 1: 1500 us */
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_ppm_process_pulse(&ppm, 2000)); /* Ch 2: 2000 us */
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_ppm_process_pulse(&ppm, 1200)); /* Ch 3: 1200 us */
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_ppm_process_pulse(&ppm, 1800)); /* Ch 4: 1800 us */
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_ppm_process_pulse(&ppm, 1600)); /* Ch 5: 1600 us */

    /* Next sync gap finishes frame */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_ppm_process_pulse(&ppm, 4500));
    TEST_ASSERT_EQUAL_UINT8(6, ppm.channel_count);
    TEST_ASSERT_EQUAL_UINT32(1, ppm.frames_received);

    TEST_ASSERT_EQUAL_UINT16(1000, syn_ppm_get_channel(&ppm, 0));
    TEST_ASSERT_EQUAL_UINT16(1500, syn_ppm_get_channel(&ppm, 1));
    TEST_ASSERT_EQUAL_UINT16(2000, syn_ppm_get_channel(&ppm, 2));
    TEST_ASSERT_EQUAL_UINT16(1200, syn_ppm_get_channel(&ppm, 3));
    TEST_ASSERT_EQUAL_UINT16(1800, syn_ppm_get_channel(&ppm, 4));
    TEST_ASSERT_EQUAL_UINT16(1600, syn_ppm_get_channel(&ppm, 5));
}

void test_ppm_null_and_clamping(void)
{
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_ppm_init(NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_ppm_process_pulse(NULL, 1500));
    TEST_ASSERT_EQUAL_UINT16(1500, syn_ppm_get_channel(NULL, 0));
    TEST_ASSERT_EQUAL_UINT16(1500, syn_ppm_get_channel(NULL, 99));

    SYN_PPM_Decoder ppm;
    syn_ppm_init(&ppm);

    /* Out of bounds pulse clamping test */
    syn_ppm_process_pulse(&ppm, 4000);
    syn_ppm_process_pulse(&ppm, 500);  /* Under-range 500 us -> clamped 750 us */
    syn_ppm_process_pulse(&ppm, 2500); /* Over-range 2500 us -> clamped 2250 us */
    syn_ppm_process_pulse(&ppm, 4000); /* Frame sync */

    TEST_ASSERT_EQUAL_UINT16(750, syn_ppm_get_channel(&ppm, 0));
    TEST_ASSERT_EQUAL_UINT16(2250, syn_ppm_get_channel(&ppm, 1));
}
