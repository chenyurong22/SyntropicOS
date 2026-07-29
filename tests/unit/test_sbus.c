/**
 * @file test_sbus.c
 * @brief Unit tests for Zero-Heap SBUS Receiver Decoder.
 */

#include "syntropic/proto/syn_sbus.h"
#include "unity/unity.h"

void test_sbus_init(void)
{
    SYN_SBUS_Parser parser;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_sbus_init(&parser));
    TEST_ASSERT_EQUAL_UINT8(0, parser.idx);
    TEST_ASSERT_EQUAL_UINT32(0, parser.frames_received);
}

void test_sbus_decode_buffer(void)
{
    uint8_t buf[25] = {0};
    buf[0] = 0x0F;  /* Header */
    buf[23] = 0x0C; /* Frame loss (bit 2) + Failsafe (bit 3) */
    buf[24] = 0x00; /* Footer */

    /* Pack channel 0 = 992 (0x03E0) into byte 1 & byte 2 */
    buf[1] = 0xE0;
    buf[2] = 0x03;

    SYN_SBUS_Frame frame;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_sbus_decode_buffer(buf, &frame));
    TEST_ASSERT_EQUAL_UINT16(992, frame.channels[0]);
    TEST_ASSERT_TRUE(frame.frame_loss);
    TEST_ASSERT_TRUE(frame.failsafe);
    TEST_ASSERT_FALSE(frame.ch17);
    TEST_ASSERT_FALSE(frame.ch18);
}

void test_sbus_streaming_parser(void)
{
    SYN_SBUS_Parser parser;
    syn_sbus_init(&parser);

    uint8_t buf[25] = {0};
    buf[0] = 0x0F;
    buf[1] = 0xAC;
    buf[2] = 0x06; /* Raw 1708 */

    SYN_SBUS_Frame frame;
    SYN_Status status = SYN_BUSY;

    for (int i = 0; i < 24; i++) {
        status = syn_sbus_parse_byte(&parser, buf[i], &frame);
        TEST_ASSERT_EQUAL_INT(SYN_BUSY, status);
    }

    /* Final byte triggers frame completion */
    status = syn_sbus_parse_byte(&parser, buf[24], &frame);
    TEST_ASSERT_EQUAL_INT(SYN_OK, status);
    TEST_ASSERT_EQUAL_UINT32(1, parser.frames_received);
    TEST_ASSERT_EQUAL_UINT16(1708, frame.channels[0]);

    /* Test parsing frame with failsafe flag and NULL frame output pointer */
    buf[23] = 0x0C; /* Frame loss + Failsafe */
    for (int i = 0; i < 24; i++) {
        syn_sbus_parse_byte(&parser, buf[i], NULL);
    }
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_sbus_parse_byte(&parser, buf[24], NULL));
    TEST_ASSERT_EQUAL_UINT32(2, parser.frames_received);
    TEST_ASSERT_EQUAL_UINT32(1, parser.frame_loss_count);
    TEST_ASSERT_EQUAL_UINT32(1, parser.failsafe_count);
}

void test_sbus_raw_to_us_scaling(void)
{
    TEST_ASSERT_EQUAL_UINT16(1000, syn_sbus_raw_to_us(100));  /* Under-range clamping */
    TEST_ASSERT_EQUAL_UINT16(1000, syn_sbus_raw_to_us(172));  /* Min scale */
    TEST_ASSERT_EQUAL_UINT16(1500, syn_sbus_raw_to_us(992));  /* Mid scale */
    TEST_ASSERT_EQUAL_UINT16(2000, syn_sbus_raw_to_us(1811)); /* Max scale */
    TEST_ASSERT_EQUAL_UINT16(2000, syn_sbus_raw_to_us(2000)); /* Over-range clamping */
}

void test_sbus_null_and_error_handling(void)
{
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_sbus_init(NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_sbus_decode_buffer(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_sbus_parse_byte(NULL, 0x0F, NULL));

    uint8_t bad_buf[25] = {0x00}; /* Bad header 0x00 (expected 0x0F) */
    SYN_SBUS_Frame frame;
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_sbus_decode_buffer(bad_buf, &frame));

    SYN_SBUS_Parser parser;
    syn_sbus_init(&parser);
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_sbus_parse_byte(&parser, 0xAA, &frame));
}
