/**
 * @file test_cjt188.c
 * @brief Unit tests for CJ/T 188 smart metering protocol driver.
 */

#include "unity/unity.h"
#include "syntropic/proto/syn_cjt188.h"

#include <string.h>

void test_cjt188_checksum_calculation(void)
{
    /* Example CJ/T 188 frame from standard:
     * 68 10 44 33 22 11 00 33 78 01 03 1F 90 00 -> CS = 80
     */
    uint8_t frame[] = {
        0x68, 0x10, 0x44, 0x33, 0x22, 0x11, 0x00, 0x33, 0x78, 0x01, 0x03, 0x1F, 0x90, 0x00
    };
    uint8_t cs = syn_cjt188_checksum(frame, sizeof(frame));
    TEST_ASSERT_EQUAL_HEX8(0x80, cs);
}

void test_cjt188_encode_read_req(void)
{
    uint8_t buf[32];
    uint8_t meter_id[5] = {0x44, 0x33, 0x22, 0x11, 0x00};
    uint8_t vendor_id[2] = {0x33, 0x78};

    size_t len = syn_cjt188_encode_read_req(buf, sizeof(buf), SYN_CJT188_METER_COLD_WATER,
                                            meter_id, vendor_id, SYN_CJT188_DI_READ_METER_DATA, 0x00);
    TEST_ASSERT_EQUAL(20, len);

    /* Verify preamble */
    TEST_ASSERT_EQUAL_HEX8(0xFE, buf[0]);
    TEST_ASSERT_EQUAL_HEX8(0xFE, buf[1]);
    TEST_ASSERT_EQUAL_HEX8(0xFE, buf[2]);
    TEST_ASSERT_EQUAL_HEX8(0xFE, buf[3]);

    /* Verify Start, Type, Control, Length, End */
    TEST_ASSERT_EQUAL_HEX8(0x68, buf[4]);
    TEST_ASSERT_EQUAL_HEX8(SYN_CJT188_METER_COLD_WATER, buf[5]);
    TEST_ASSERT_EQUAL_HEX8(SYN_CJT188_CTRL_READ_DATA, buf[13]);
    TEST_ASSERT_EQUAL_HEX8(0x03, buf[14]);
    TEST_ASSERT_EQUAL_HEX8(0x80, buf[18]); /* Checksum */
    TEST_ASSERT_EQUAL_HEX8(0x16, buf[19]); /* End byte */
}

void test_cjt188_encode_valve_ctrl(void)
{
    uint8_t buf[32];
    uint8_t meter_id[5] = {0x69, 0x05, 0x90, 0x05, 0x15};
    uint8_t vendor_id[2] = {0x33, 0x78};

    /* Encode Open Valve (0x55) */
    size_t len = syn_cjt188_encode_valve_ctrl(buf, sizeof(buf), SYN_CJT188_METER_COLD_WATER,
                                              meter_id, vendor_id, true, 0x01);
    TEST_ASSERT_EQUAL(21, len);
    TEST_ASSERT_EQUAL_HEX8(SYN_CJT188_CTRL_WRITE_DATA, buf[13]);
    TEST_ASSERT_EQUAL_HEX8(0x04, buf[14]); /* Data len */
    TEST_ASSERT_EQUAL_HEX8(0x17, buf[15]); /* DI LSB */
    TEST_ASSERT_EQUAL_HEX8(0xA0, buf[16]); /* DI MSB */
    TEST_ASSERT_EQUAL_HEX8(0x01, buf[17]); /* SER */
    TEST_ASSERT_EQUAL_HEX8(SYN_CJT188_VALVE_OPEN, buf[18]); /* 0x55 */
    TEST_ASSERT_EQUAL_HEX8(0x16, buf[20]); /* End byte */
}

void test_cjt188_parse_valid_frame(void)
{
    /* Sample CJ/T 188 response frame:
     * FE FE FE FE 68 10 44 33 22 11 00 33 78 81 16 1F 90 00 ... CS 16
     */
    uint8_t raw[] = {
        0xFE, 0xFE, 0x68, 0x10, 0x44, 0x33, 0x22, 0x11, 0x00, 0x33, 0x78,
        0x81, 0x05, 0x1F, 0x90, 0x00, 0x77, 0x66, 0x2E, 0x16
    };
    /* Compute checksum over 68 .. 66 (16 bytes) */
    raw[18] = syn_cjt188_checksum(&raw[2], 16);

    SYN_CJT188_Frame frame;
    bool ok = syn_cjt188_parse_frame(raw, sizeof(raw), &frame);
    TEST_ASSERT_TRUE(ok);

    TEST_ASSERT_EQUAL_HEX8(SYN_CJT188_METER_COLD_WATER, frame.meter_type);
    TEST_ASSERT_EQUAL_HEX8(SYN_CJT188_CTRL_READ_DATA_RESP, frame.ctrl);
    TEST_ASSERT_EQUAL(0x5, frame.len);
    TEST_ASSERT_EQUAL_HEX16(SYN_CJT188_DI_READ_METER_DATA, frame.data_id);
    TEST_ASSERT_EQUAL(0x00, frame.seq);
    TEST_ASSERT_NOT_NULL(frame.payload);
    TEST_ASSERT_EQUAL(2, frame.payload_len);
    TEST_ASSERT_EQUAL_HEX8(0x77, frame.payload[0]);
    TEST_ASSERT_EQUAL_HEX8(0x66, frame.payload[1]);
}

void test_cjt188_streaming_decoder(void)
{
    SYN_CJT188_Decoder decoder;
    syn_cjt188_decoder_init(&decoder);

    uint8_t raw[] = {
        0xFE, 0xFE, 0xFE, 0xFE, 0x68, 0x10, 0x44, 0x33, 0x22, 0x11, 0x00,
        0x33, 0x78, 0x01, 0x03, 0x1F, 0x90, 0x00, 0x80, 0x16
    };

    SYN_CJT188_Frame frame;
    bool got_frame = false;

    for (size_t i = 0; i < sizeof(raw); i++) {
        if (syn_cjt188_decoder_feed(&decoder, raw[i], &frame)) {
            got_frame = true;
        }
    }

    TEST_ASSERT_TRUE(got_frame);
    TEST_ASSERT_EQUAL_HEX8(SYN_CJT188_METER_COLD_WATER, frame.meter_type);
    TEST_ASSERT_EQUAL_HEX8(SYN_CJT188_CTRL_READ_DATA, frame.ctrl);
    TEST_ASSERT_EQUAL_HEX16(SYN_CJT188_DI_READ_METER_DATA, frame.data_id);
}

void run_cjt188_tests(void)
{
    RUN_TEST(test_cjt188_checksum_calculation);
    RUN_TEST(test_cjt188_encode_read_req);
    RUN_TEST(test_cjt188_encode_valve_ctrl);
    RUN_TEST(test_cjt188_parse_valid_frame);
    RUN_TEST(test_cjt188_streaming_decoder);
}
