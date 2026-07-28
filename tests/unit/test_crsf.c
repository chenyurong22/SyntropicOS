/**
 * @file test_crsf.c
 * @brief Unit tests for Zero-Heap CRSF (TBS Crossfire / ExpressLRS) Protocol Parser.
 */

#include "syntropic/proto/syn_crsf.h"
#include "unity/unity.h"

void test_crsf_init(void)
{
    SYN_CRSF_Parser parser;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_crsf_init(&parser));
    TEST_ASSERT_EQUAL_UINT8(0, parser.idx);
    TEST_ASSERT_EQUAL_UINT32(0, parser.packets_received);
}

void test_crsf_crc8_calculation(void)
{
    /* Test CRC8 DVB-S2 (poly 0xD5) */
    uint8_t data[] = {0x16, 0x00, 0x00};
    uint8_t crc = syn_crsf_calc_crc(data, sizeof(data));
    TEST_ASSERT_NOT_EQUAL(0, crc);
}

void test_crsf_parse_rc_channels(void)
{
    SYN_CRSF_Parser parser;
    syn_crsf_init(&parser);

    /* Construct 26-byte CRSF RC channels packet */
    uint8_t pkt[26] = {0};
    pkt[0] = 0xC8; /* Addr */
    pkt[1] = 24;   /* Payload length (1 type + 22 channels + 1 crc) */
    pkt[2] = 0x16; /* RC Channels frame type */

    /* Pack channel 0 = 992 (0x03E0) into payload[0..1] */
    pkt[3] = 0xE0;
    pkt[4] = 0x03;

    /* Compute expected CRC over bytes 2..24 */
    pkt[25] = syn_crsf_calc_crc(&pkt[2], 23);

    SYN_CRSF_FrameType type;
    SYN_Status status = SYN_BUSY;

    for (int i = 0; i < 25; i++) {
        status = syn_crsf_parse_byte(&parser, pkt[i], &type);
        TEST_ASSERT_EQUAL_INT(SYN_BUSY, status);
    }

    status = syn_crsf_parse_byte(&parser, pkt[25], &type);
    TEST_ASSERT_EQUAL_INT(SYN_OK, status);
    TEST_ASSERT_EQUAL_INT(SYN_CRSF_TYPE_RC_CHANNELS, type);
    TEST_ASSERT_EQUAL_UINT16(992, parser.last_channels.channels[0]);
}

void test_crsf_raw_to_us_scaling(void)
{
    TEST_ASSERT_EQUAL_UINT16(988, syn_crsf_raw_to_us(100));   /* Under-range */
    TEST_ASSERT_EQUAL_UINT16(988, syn_crsf_raw_to_us(170));   /* Min */
    TEST_ASSERT_EQUAL_UINT16(1472, syn_crsf_raw_to_us(992));  /* Mid */
    TEST_ASSERT_EQUAL_UINT16(2012, syn_crsf_raw_to_us(1908)); /* Max */
    TEST_ASSERT_EQUAL_UINT16(2012, syn_crsf_raw_to_us(2500)); /* Over-range */
}

void test_crsf_null_and_error_handling(void)
{
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_crsf_init(NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_crsf_parse_byte(NULL, 0xC8, NULL));
    TEST_ASSERT_EQUAL_UINT8(0, syn_crsf_calc_crc(NULL, 10));

    SYN_CRSF_Parser parser;
    syn_crsf_init(&parser);
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_crsf_parse_byte(&parser, 0x00, NULL)); /* Bad addr */
}
