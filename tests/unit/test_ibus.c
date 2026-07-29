/**
 * @file test_ibus.c
 * @brief Unit tests for Zero-Heap IBUS (FlySky) Receiver Decoder.
 */

#include "syntropic/proto/syn_ibus.h"
#include "unity/unity.h"

void test_ibus_init(void)
{
    SYN_IBUS_Parser parser;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_ibus_init(&parser));
    TEST_ASSERT_EQUAL_UINT8(0, parser.idx);
    TEST_ASSERT_EQUAL_UINT32(0, parser.frames_received);
}

void test_ibus_checksum_calculation(void)
{
    uint8_t buf[30] = {0};
    buf[0] = 0x20;
    buf[1] = 0x40;

    uint16_t checksum = syn_ibus_calc_checksum(buf);
    TEST_ASSERT_EQUAL_UINT16(0xFFFFU - 0x20U - 0x40U, checksum);
}

void test_ibus_streaming_parser(void)
{
    SYN_IBUS_Parser parser;
    syn_ibus_init(&parser);

    uint8_t buf[32] = {0};
    buf[0] = 0x20;
    buf[1] = 0x40;
    /* Channel 0 = 1500 us (0x05DC) */
    buf[2] = 0xDC;
    buf[3] = 0x05;

    uint16_t sum = syn_ibus_calc_checksum(buf);
    buf[30] = (uint8_t)(sum & 0xFF);
    buf[31] = (uint8_t)(sum >> 8);

    SYN_IBUS_Frame frame;
    SYN_Status status = SYN_BUSY;

    for (int i = 0; i < 31; i++) {
        status = syn_ibus_parse_byte(&parser, buf[i], &frame);
        TEST_ASSERT_EQUAL_INT(SYN_BUSY, status);
    }

    status = syn_ibus_parse_byte(&parser, buf[31], &frame);
    TEST_ASSERT_EQUAL_INT(SYN_OK, status);
    TEST_ASSERT_EQUAL_UINT32(1, parser.frames_received);
    TEST_ASSERT_EQUAL_UINT16(1500, frame.channels[0]);

    /* Test parsing complete 32-byte frame with NULL frame output pointer */
    for (int i = 0; i < 32; i++) {
        status = syn_ibus_parse_byte(&parser, buf[i], NULL);
    }
    TEST_ASSERT_EQUAL_INT(SYN_OK, status);
    TEST_ASSERT_EQUAL_UINT32(2, parser.frames_received);
}

void test_ibus_null_and_error_handling(void)
{
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_ibus_init(NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_ibus_parse_byte(NULL, 0x20, NULL));
    TEST_ASSERT_EQUAL_UINT16(0, syn_ibus_calc_checksum(NULL));

    SYN_IBUS_Parser parser;
    syn_ibus_init(&parser);
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_ibus_parse_byte(&parser, 0x00, NULL)); /* Bad header 1 */

    /* Header 1 correct, header 2 mismatch */
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_ibus_parse_byte(&parser, SYN_IBUS_HEADER1, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_ibus_parse_byte(&parser, 0x00, NULL)); /* Bad header 2 */
    TEST_ASSERT_EQUAL_UINT8(0, parser.idx);                                     /* Reset to 0 */
}
