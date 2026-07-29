/**
 * @file test_dshot.c
 * @brief Unit tests for Zero-Heap DShot Digital ESC Driver.
 */

#include "syntropic/output/syn_dshot.h"
#include "syntropic/output/syn_dshot_telemetry.h"
#include "unity/unity.h"

void test_dshot_crc_calculation(void)
{
    /* Test CRC calculation for payload 0x000 (0 throttle, no telemetry) */
    uint8_t crc0 = syn_dshot_calc_crc(0x000);
    TEST_ASSERT_EQUAL_UINT8(0x00, crc0);

    /* Test CRC calculation for throttle 1048, no telemetry: payload = 1048 << 1 = 2096 (0x0830) */
    /* crc = 0x0830 ^ 0x0083 ^ 0x0008 = 0x08BBU -> & 0x0F = 0x0B */
    uint8_t crc_test = syn_dshot_calc_crc(0x0830);
    TEST_ASSERT_EQUAL_UINT8(0x0B, crc_test);
}

void test_dshot_encode(void)
{
    SYN_DShot_Packet packet;

    /* Disarmed throttle 0 */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_dshot_encode(0, false, &packet));
    TEST_ASSERT_EQUAL_UINT16(0, packet.throttle);
    TEST_ASSERT_FALSE(packet.telemetry);
    TEST_ASSERT_EQUAL_UINT16(0x0000, packet.raw_frame);

    /* Throttle 1000 with telemetry requested */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_dshot_encode(1000, true, &packet));
    TEST_ASSERT_EQUAL_UINT16(1000, packet.throttle);
    TEST_ASSERT_TRUE(packet.telemetry);
}

void test_dshot_us_to_throttle(void)
{
    TEST_ASSERT_EQUAL_UINT16(0, syn_dshot_us_to_throttle(1000));    /* Below disarm threshold */
    TEST_ASSERT_EQUAL_UINT16(0, syn_dshot_us_to_throttle(1048));    /* Disarm boundary */
    TEST_ASSERT_EQUAL_UINT16(2047, syn_dshot_us_to_throttle(2000)); /* Max throttle boundary */
    TEST_ASSERT_EQUAL_UINT16(2047, syn_dshot_us_to_throttle(2200)); /* Over-range clamping */
}

void test_dshot_null_and_clamping(void)
{
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_dshot_encode(1000, false, NULL));

    SYN_DShot_Packet packet;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_dshot_encode(3000, false, &packet));
    TEST_ASSERT_EQUAL_UINT16(2047, packet.throttle);
}

void test_dshot_telemetry_extended_edge_cases(void)
{
    /* Null argument tests */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_dshot_decode_gcr_20bit(0, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_dshot_parse_telemetry(0, 7, NULL));

    /* Invalid GCR nibble test: 0x00000 has 0x00 GCR codes which map to -1 */
    uint16_t payload = 0;
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_dshot_decode_gcr_20bit(0x00000, &payload));
    SYN_DShot_Telemetry telem;
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_dshot_parse_telemetry(0x00000, 7, &telem));

    /* Construct a valid 20-bit GCR payload with data_12bit = 0x000 (period_us = 0) */
    /* payload_16 = (0x000 << 4) | CRC(0x000) = 0x0000. GCR for nibble 0 is index 25 (0x19) */
    /* 4 nibbles of 0 -> GCR 0x19, 0x19, 0x19, 0x19 -> (0x19<<15) | (0x19<<10) | (0x19<<5) | 0x19 */
    uint32_t valid_gcr_zero_period = (0x19U << 15U) | (0x19U << 10U) | (0x19U << 5U) | 0x19U;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_dshot_decode_gcr_20bit(valid_gcr_zero_period, &payload));
    TEST_ASSERT_EQUAL_UINT16(0x0000, payload);

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_dshot_parse_telemetry(valid_gcr_zero_period, 0, &telem));
    TEST_ASSERT_TRUE(telem.valid);
    TEST_ASSERT_EQUAL_UINT32(0, telem.period_us);
    TEST_ASSERT_EQUAL_UINT32(0, telem.erpm);
    TEST_ASSERT_EQUAL_UINT32(0, telem.rpm);

    /* Construct GCR frame with valid GCR decoding but invalid CRC */
    /* Nibbles: 0, 0, 0, 1 -> 0x0001. data_12bit=0x000, rx_crc=1. CRC(0)=0. Mismatch! */
    /* Nibble 1 (0x01) -> GCR index 27 (0x1B) */
    uint32_t gcr_crc_error = (0x19U << 15U) | (0x19U << 10U) | (0x19U << 5U) | 0x1BU;
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_dshot_parse_telemetry(gcr_crc_error, 7, &telem));
    TEST_ASSERT_FALSE(telem.valid);
}
