/**
 * @file test_dshot.c
 * @brief Unit tests for Zero-Heap DShot Digital ESC Driver.
 */

#include "syntropic/output/syn_dshot.h"
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
