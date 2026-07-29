/**
 * @file test_mavlink.c
 * @brief Unit tests for Zero-Heap MAVLink v2 Protocol Parser & Encoder.
 */

#include "syntropic/proto/syn_mavlink.h"
#include "unity/unity.h"

void test_mavlink_init(void)
{
    SYN_MAVLINK_Parser parser;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_mavlink_init(&parser));
    TEST_ASSERT_EQUAL_UINT32(0, parser.packets_received);
}

void test_mavlink_encode_and_parse_attitude(void)
{
    /* Construct MAVLink v2 Attitude frame payload (28 bytes) */
    uint8_t payload[28] = {0};
    uint8_t buf[64];
    size_t out_len = 0;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_mavlink_encode_msg(1, 1, 42, SYN_MAVLINK_MSG_ATTITUDE, 39,
                                                         payload, 28, buf, &out_len));
    TEST_ASSERT_EQUAL_INT(40, out_len); /* 10 header + 28 payload + 2 CRC */

    SYN_MAVLINK_Parser parser;
    syn_mavlink_init(&parser);

    SYN_MAVLINK_Frame frame;
    SYN_Status status = SYN_BUSY;

    for (size_t i = 0; i < out_len - 1; i++) {
        status = syn_mavlink_parse_byte(&parser, buf[i], &frame);
        TEST_ASSERT_EQUAL_INT(SYN_BUSY, status);
    }

    status = syn_mavlink_parse_byte(&parser, buf[out_len - 1], &frame);
    TEST_ASSERT_EQUAL_INT(SYN_OK, status);
    TEST_ASSERT_EQUAL_UINT8(1, frame.sys_id);
    TEST_ASSERT_EQUAL_UINT8(1, frame.comp_id);
    TEST_ASSERT_EQUAL_UINT8(42, frame.seq);
    TEST_ASSERT_EQUAL_UINT32(SYN_MAVLINK_MSG_ATTITUDE, frame.msg_id);
    TEST_ASSERT_EQUAL_UINT8(28, frame.payload_len);
}

void test_mavlink_null_and_crc_error(void)
{
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_mavlink_init(NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_mavlink_parse_byte(NULL, 0xFD, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM,
                          syn_mavlink_encode_msg(1, 1, 0, 0, 0, NULL, 0, NULL, NULL));

    SYN_MAVLINK_Parser parser;
    syn_mavlink_init(&parser);

    /* Corrupt packet (12 bytes: 10 header + 2 bad CRC bytes) */
    uint8_t buf[12] = {0xFD, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0xFF, 0xFF};
    SYN_Status status = SYN_BUSY;
    for (int i = 0; i < 11; i++) {
        status = syn_mavlink_parse_byte(&parser, buf[i], NULL);
        TEST_ASSERT_EQUAL_INT(SYN_BUSY, status);
    }
    status = syn_mavlink_parse_byte(&parser, buf[11], NULL);
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, status);
    TEST_ASSERT_EQUAL_UINT32(1, parser.crc_errors);
}

void test_mavlink_msg_ids_and_invalid_state_fallback(void)
{
    /* Test encode_msg with sys_status, global_pos_int, vfr_hud, and unknown msg_id */
    uint8_t payload[8] = {0};
    uint8_t buf[32];
    size_t out_len = 0;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_mavlink_encode_msg(1, 1, 1, SYN_MAVLINK_MSG_SYS_STATUS, 124,
                                                         payload, 8, buf, &out_len));
    TEST_ASSERT_EQUAL_INT(SYN_OK,
                          syn_mavlink_encode_msg(1, 1, 2, SYN_MAVLINK_MSG_GLOBAL_POSITION_INT, 96,
                                                 payload, 8, buf, &out_len));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_mavlink_encode_msg(1, 1, 3, SYN_MAVLINK_MSG_VFR_HUD, 20,
                                                         payload, 8, buf, &out_len));
    TEST_ASSERT_EQUAL_INT(SYN_OK,
                          syn_mavlink_encode_msg(1, 1, 4, 99999, 0, payload, 8, buf, &out_len));

    /* Test parser fallback when state is invalid */
    SYN_MAVLINK_Parser parser;
    syn_mavlink_init(&parser);
    parser.state = 255; /* Out of range state */
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_mavlink_parse_byte(&parser, 0, NULL));
    TEST_ASSERT_EQUAL_INT(0, parser.state); /* Resets to STATE_STX */
}
