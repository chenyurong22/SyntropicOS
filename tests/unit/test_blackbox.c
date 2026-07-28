/**
 * @file test_blackbox.c
 * @brief Unit tests for Zero-Heap Flight Telemetry Blackbox Binary Recorder.
 */

#include "syntropic/log/syn_blackbox.h"
#include "unity/unity.h"

void test_blackbox_init(void)
{
    SYN_Blackbox bb;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_blackbox_init(&bb));
    TEST_ASSERT_EQUAL_UINT32(0, bb.frame_count);
}

void test_blackbox_varint(void)
{
    uint8_t buf[8];
    size_t len = syn_blackbox_encode_varint(0, buf);
    TEST_ASSERT_EQUAL_INT(1, len);
    TEST_ASSERT_EQUAL_UINT8(0, buf[0]);

    len = syn_blackbox_encode_varint(-1, buf);
    TEST_ASSERT_EQUAL_INT(1, len);
    TEST_ASSERT_EQUAL_UINT8(1, buf[0]); /* ZigZag(-1) = 1 */
}

void test_blackbox_encode_intra_and_delta(void)
{
    SYN_Blackbox bb;
    syn_blackbox_init(&bb);

    SYN_Blackbox_Record rec1 = {.iteration = 100,
                                .time_us = 10000,
                                .gyro = {10, -5, 2},
                                .accel = {0, 0, 1000},
                                .setpoint = {0, 0, 0, 1500},
                                .motor = {1500, 1500, 1500, 1500}};

    uint8_t buf[64];
    size_t out_len = 0;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_blackbox_encode_intra(&bb, &rec1, buf, &out_len));
    TEST_ASSERT_EQUAL_UINT8('I', buf[0]);
    TEST_ASSERT_TRUE(out_len > 15);
    TEST_ASSERT_EQUAL_UINT32(1, bb.frame_count);

    SYN_Blackbox_Record rec2 = rec1;
    rec2.iteration++;
    rec2.time_us += 1000;
    rec2.gyro[0] += 2;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_blackbox_encode_delta(&bb, &rec2, buf, &out_len));
    TEST_ASSERT_EQUAL_UINT8('P', buf[0]);
    TEST_ASSERT_TRUE(out_len > 10);
    TEST_ASSERT_EQUAL_UINT32(2, bb.frame_count);
}

void test_blackbox_null_checks(void)
{
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_blackbox_init(NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_blackbox_encode_intra(NULL, NULL, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_blackbox_encode_delta(NULL, NULL, NULL, NULL));
}
