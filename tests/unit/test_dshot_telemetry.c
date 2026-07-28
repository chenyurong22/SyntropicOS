/**
 * @file test_dshot_telemetry.c
 * @brief Unit tests for Zero-Heap Bidirectional DShot (BDShot) Telemetry Decoder.
 */

#include "syntropic/output/syn_dshot.h"
#include "syntropic/output/syn_dshot_telemetry.h"
#include "unity/unity.h"

void test_dshot_gcr_decode(void)
{
    uint16_t payload = 0;
    /* GCR representation for nibbles 0x1, 0x2, 0x3, 0x4 -> binary GCR */
    /* 0x1=1B(11011), 0x2=12(10010), 0x3=13(10011), 0x4=1D(11101) */
    uint32_t gcr = (0x1BU << 15U) | (0x12U << 10U) | (0x13U << 5U) | 0x1DU;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_dshot_decode_gcr_20bit(gcr, &payload));
    TEST_ASSERT_EQUAL_HEX16(0x1234, payload);
}

void test_dshot_telemetry_erpm_parsing(void)
{
    SYN_DShot_Telemetry telem;

    /* Test parser error handling */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_dshot_parse_telemetry(0, 7, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_dshot_decode_gcr_20bit(0, NULL));

    /* Test invalid GCR symbol */
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_dshot_parse_telemetry(0xFFFFFFFF, 7, &telem));
}
