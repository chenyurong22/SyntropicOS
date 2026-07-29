/**
 * @file test_smbus.c
 * @brief Unit tests for SMBus protocol engine and PEC CRC-8 calculation.
 */

#include "syntropic/syntropic.h"
#include "unity/unity.h"

void test_smbus_pec_calculation(void)
{
    /* Test PEC CRC-8 with standard vectors */
    uint8_t data1[] = {0x5A};
    uint8_t pec1 = syn_smbus_calc_pec(0, data1, sizeof(data1));
    TEST_ASSERT_NOT_EQUAL(0, pec1);

    /* Incremental PEC calculation */
    uint8_t data2_part1[] = {0x5A, 0x01};
    uint8_t data2_part2[] = {0x02, 0x03};
    uint8_t pec_full = syn_smbus_calc_pec(0, data2_part1, 2);
    pec_full = syn_smbus_calc_pec(pec_full, data2_part2, 2);

    uint8_t data2_all[] = {0x5A, 0x01, 0x02, 0x03};
    uint8_t pec_direct = syn_smbus_calc_pec(0, data2_all, 4);
    TEST_ASSERT_EQUAL_HEX8(pec_direct, pec_full);

    TEST_ASSERT_EQUAL_HEX8(0x00, syn_smbus_calc_pec(0x00, NULL, 5));
}

void test_smbus_encode_decode_write_word(void)
{
    SYN_SMBUS_Packet tx_pkt;
    tx_pkt.slave_addr = 0x4B;
    tx_pkt.command = 0x21;
    tx_pkt.proto = SYN_SMBUS_PROTO_WRITE_WORD;
    tx_pkt.pec_enabled = true;
    tx_pkt.length = 2;
    tx_pkt.data[0] = 0xAB;
    tx_pkt.data[1] = 0xCD;

    uint8_t buf[64];
    size_t out_len = 0;
    SYN_Status status = syn_smbus_encode_packet(&tx_pkt, buf, sizeof(buf), &out_len);

    TEST_ASSERT_EQUAL(SYN_OK, status);
    TEST_ASSERT_EQUAL(5, out_len);        /* Addr(W) + Cmd + Data[0] + Data[1] + PEC */
    TEST_ASSERT_EQUAL_HEX8(0x96, buf[0]); /* (0x4B << 1) | 0 */
    TEST_ASSERT_EQUAL_HEX8(0x21, buf[1]);
    TEST_ASSERT_EQUAL_HEX8(0xAB, buf[2]);
    TEST_ASSERT_EQUAL_HEX8(0xCD, buf[3]);

    /* Test decoding */
    SYN_SMBUS_Packet rx_pkt;
    status = syn_smbus_decode_packet(&rx_pkt, buf, out_len, SYN_SMBUS_PROTO_WRITE_WORD, true);
    TEST_ASSERT_EQUAL(SYN_OK, status);
    TEST_ASSERT_TRUE(rx_pkt.pec_valid);
    TEST_ASSERT_EQUAL(2, rx_pkt.length);
    TEST_ASSERT_EQUAL_HEX8(0xAB, rx_pkt.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xCD, rx_pkt.data[1]);
}

void test_smbus_decode_invalid_pec(void)
{
    uint8_t bad_buf[] = {0x96, 0x21, 0xAB, 0xCD, 0x00}; /* Bad PEC byte */
    SYN_SMBUS_Packet rx_pkt;
    SYN_Status status = syn_smbus_decode_packet(&rx_pkt, bad_buf, sizeof(bad_buf),
                                                SYN_SMBUS_PROTO_WRITE_WORD, true);
    TEST_ASSERT_EQUAL(SYN_ERROR, status);
    TEST_ASSERT_FALSE(rx_pkt.pec_valid);
}

void test_smbus_block_process_call(void)
{
    SYN_SMBUS_Packet pkt;
    pkt.slave_addr = 0x16;
    pkt.command = 0x08;
    pkt.proto = SYN_SMBUS_PROTO_BLOCK_PROCESS_CALL;
    pkt.pec_enabled = false;
    pkt.length = 4;
    pkt.data[0] = 1;
    pkt.data[1] = 2;
    pkt.data[2] = 3;
    pkt.data[3] = 4;

    uint8_t buf[64];
    size_t out_len = 0;
    SYN_Status status = syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len);

    TEST_ASSERT_EQUAL(SYN_OK, status);
    TEST_ASSERT_EQUAL(7, out_len); /* Addr(W) + Cmd + Len + 4 Data bytes */
    TEST_ASSERT_EQUAL_HEX8(0x2C, buf[0]);
    TEST_ASSERT_EQUAL_HEX8(0x08, buf[1]);
    TEST_ASSERT_EQUAL_HEX8(4, buf[2]);

    /* Process call with length 0 and 1 */
    pkt.proto = SYN_SMBUS_PROTO_PROCESS_CALL;
    pkt.length = 0;
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len));
    pkt.length = 1;
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len));
}

void test_smbus_quick_and_byte_protocols(void)
{
    SYN_SMBUS_Packet pkt;
    uint8_t buf[64];
    size_t out_len = 0;

    /* Quick Read */
    pkt.slave_addr = 0x27;
    pkt.proto = SYN_SMBUS_PROTO_QUICK_READ;
    pkt.pec_enabled = false;
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len));
    TEST_ASSERT_EQUAL(1, out_len);
    TEST_ASSERT_EQUAL_HEX8(0x4F, buf[0]);

    /* Quick Write */
    pkt.proto = SYN_SMBUS_PROTO_QUICK_WRITE;
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len));
    TEST_ASSERT_EQUAL(1, out_len);
    TEST_ASSERT_EQUAL_HEX8(0x4E, buf[0]);

    /* Send Byte */
    pkt.proto = SYN_SMBUS_PROTO_SEND_BYTE;
    pkt.command = 0x55;
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len));
    TEST_ASSERT_EQUAL(2, out_len);
    TEST_ASSERT_EQUAL_HEX8(0x55, buf[1]);

    /* Receive Byte */
    pkt.proto = SYN_SMBUS_PROTO_RECEIVE_BYTE;
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len));
    TEST_ASSERT_EQUAL(1, out_len);
    TEST_ASSERT_EQUAL_HEX8(0x4F, buf[0]);

    /* Write Byte with 0 length */
    pkt.proto = SYN_SMBUS_PROTO_WRITE_BYTE;
    pkt.command = 0x10;
    pkt.length = 0;
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len));
    TEST_ASSERT_EQUAL(3, out_len);
    TEST_ASSERT_EQUAL_HEX8(0x00, buf[2]);

    /* Write Byte with length 1 */
    pkt.length = 1;
    pkt.data[0] = 0x88;
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len));
    TEST_ASSERT_EQUAL(3, out_len);
    TEST_ASSERT_EQUAL_HEX8(0x88, buf[2]);

    /* Read Byte */
    pkt.proto = SYN_SMBUS_PROTO_READ_BYTE;
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len));
    TEST_ASSERT_EQUAL(2, out_len);

    /* Write Word with length 0 */
    pkt.proto = SYN_SMBUS_PROTO_WRITE_WORD;
    pkt.length = 0;
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len));
    TEST_ASSERT_EQUAL(4, out_len);

    /* Write Word with length 1 */
    pkt.length = 1;
    pkt.data[0] = 0x11;
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len));
    TEST_ASSERT_EQUAL(4, out_len);

    /* Read Word */
    pkt.proto = SYN_SMBUS_PROTO_READ_WORD;
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len));
    TEST_ASSERT_EQUAL(2, out_len);

    /* Process Call */
    pkt.proto = SYN_SMBUS_PROTO_PROCESS_CALL;
    pkt.length = 2;
    pkt.data[0] = 0x12;
    pkt.data[1] = 0x34;
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len));
    TEST_ASSERT_EQUAL(4, out_len);

    /* Write Block with length > max block len (truncation) */
    pkt.proto = SYN_SMBUS_PROTO_WRITE_BLOCK;
    pkt.length = 50; /* > 32 */
    for (int i = 0; i < 32; i++)
        pkt.data[i] = (uint8_t)i;
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len));

    /* Null data check for PEC calculation */
    TEST_ASSERT_EQUAL_UINT8(0x12, syn_smbus_calc_pec(0x12, NULL, 0));
    TEST_ASSERT_EQUAL(35, out_len);
    TEST_ASSERT_EQUAL_HEX8(32, buf[2]);

    /* Write Block with zero length */
    pkt.length = 0;
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len));
    TEST_ASSERT_EQUAL(3, out_len);

    /* Block Process Call with zero length */
    pkt.proto = SYN_SMBUS_PROTO_BLOCK_PROCESS_CALL;
    pkt.length = 0;
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len));

    /* Read Block */
    pkt.proto = SYN_SMBUS_PROTO_READ_BLOCK;
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len));
    TEST_ASSERT_EQUAL(2, out_len);
}

void test_smbus_decoding_variations(void)
{
    SYN_SMBUS_Packet rx_pkt;

    /* Receive Byte response */
    uint8_t byte_resp[] = {0x42};
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_decode_packet(&rx_pkt, byte_resp, 1,
                                                      SYN_SMBUS_PROTO_RECEIVE_BYTE, false));
    TEST_ASSERT_EQUAL(1, rx_pkt.length);
    TEST_ASSERT_EQUAL_HEX8(0x42, rx_pkt.data[0]);

    /* Receive/Read Byte full frame decoding (data_len >= 3) */
    uint8_t byte_full_frame[] = {0x50, 0x10, 0x77};
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_decode_packet(&rx_pkt, byte_full_frame, 3,
                                                      SYN_SMBUS_PROTO_READ_BYTE, false));
    TEST_ASSERT_EQUAL(1, rx_pkt.length);
    TEST_ASSERT_EQUAL_HEX8(0x77, rx_pkt.data[0]);

    /* Write Byte full frame decoding (data_len >= 3) */
    uint8_t write_byte_raw[] = {0x4E, 0x10, 0x99};
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_decode_packet(&rx_pkt, write_byte_raw, 3,
                                                      SYN_SMBUS_PROTO_WRITE_BYTE, false));
    TEST_ASSERT_EQUAL(1, rx_pkt.length);
    TEST_ASSERT_EQUAL_HEX8(0x99, rx_pkt.data[0]);

    /* Write Byte response only decoding (data_len == 1) */
    uint8_t write_byte_resp[] = {0x88};
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_decode_packet(&rx_pkt, write_byte_resp, 1,
                                                      SYN_SMBUS_PROTO_WRITE_BYTE, false));
    TEST_ASSERT_EQUAL(1, rx_pkt.length);
    TEST_ASSERT_EQUAL_HEX8(0x88, rx_pkt.data[0]);

    /* Write Word response only decoding (data_len == 2) */
    uint8_t write_word_resp[] = {0x11, 0x22};
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_decode_packet(&rx_pkt, write_word_resp, 2,
                                                      SYN_SMBUS_PROTO_WRITE_WORD, false));
    TEST_ASSERT_EQUAL(2, rx_pkt.length);
    TEST_ASSERT_EQUAL_HEX8(0x11, rx_pkt.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x22, rx_pkt.data[1]);

    /* Read Word response only decoding (data_len == 2) */
    uint8_t read_word_resp[] = {0x33, 0x44};
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_decode_packet(&rx_pkt, read_word_resp, 2,
                                                      SYN_SMBUS_PROTO_READ_WORD, false));
    TEST_ASSERT_EQUAL(2, rx_pkt.length);
    TEST_ASSERT_EQUAL_HEX8(0x33, rx_pkt.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x44, rx_pkt.data[1]);

    /* Read Block response decoding */
    uint8_t block_raw[] = {0x03, 0xDE, 0xAD, 0xBE};
    TEST_ASSERT_EQUAL(
        SYN_OK, syn_smbus_decode_packet(&rx_pkt, block_raw, 4, SYN_SMBUS_PROTO_READ_BLOCK, false));
    TEST_ASSERT_EQUAL(3, rx_pkt.length);
    TEST_ASSERT_EQUAL_HEX8(0xDE, rx_pkt.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xAD, rx_pkt.data[1]);
    TEST_ASSERT_EQUAL_HEX8(0xBE, rx_pkt.data[2]);

    /* Full Block frame decoding (header_len = 2) */
    uint8_t block_full_frame[] = {0x50, 0x20, 0x02, 0x55, 0xAA};
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_decode_packet(&rx_pkt, block_full_frame, 5,
                                                      SYN_SMBUS_PROTO_WRITE_BLOCK, false));
    TEST_ASSERT_EQUAL(2, rx_pkt.length);
    TEST_ASSERT_EQUAL_HEX8(0x55, rx_pkt.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xAA, rx_pkt.data[1]);

    /* Default case decoding */
    uint8_t custom_raw[] = {0xAA, 0xBB};
    TEST_ASSERT_EQUAL(
        SYN_OK, syn_smbus_decode_packet(&rx_pkt, custom_raw, 2, (SYN_SMBUS_Protocol)99, false));
    TEST_ASSERT_EQUAL(2, rx_pkt.length);
}

void test_smbus_invalid_params_and_overflow(void)
{
    SYN_SMBUS_Packet pkt;
    uint8_t buf[2];
    size_t out_len = 0;

    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_smbus_encode_packet(NULL, buf, sizeof(buf), &out_len));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM,
                      syn_smbus_decode_packet(NULL, buf, 1, SYN_SMBUS_PROTO_READ_BYTE, false));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM,
                      syn_smbus_decode_packet(&pkt, buf, 0, SYN_SMBUS_PROTO_READ_BYTE, true));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM,
                      syn_smbus_decode_packet(&pkt, buf, 0, SYN_SMBUS_PROTO_WRITE_BYTE, false));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM,
                      syn_smbus_decode_packet(&pkt, buf, 1, SYN_SMBUS_PROTO_WRITE_WORD, false));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM,
                      syn_smbus_decode_packet(&pkt, buf, 0, SYN_SMBUS_PROTO_READ_BYTE, false));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM,
                      syn_smbus_decode_packet(&pkt, buf, 1, SYN_SMBUS_PROTO_READ_WORD, false));

    /* Block invalid length check */
    uint8_t invalid_block[] = {0x05, 0x01}; /* Count is 5, but len is 2 */
    TEST_ASSERT_EQUAL(
        SYN_INVALID_PARAM,
        syn_smbus_decode_packet(&pkt, invalid_block, 2, SYN_SMBUS_PROTO_READ_BLOCK, false));

    /* Buffer overflow checks for all protocol variants */
    pkt.slave_addr = 0x10;
    pkt.pec_enabled = true;

    pkt.proto = SYN_SMBUS_PROTO_QUICK_READ;
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_smbus_encode_packet(&pkt, buf, 0, &out_len));

    pkt.proto = SYN_SMBUS_PROTO_SEND_BYTE;
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_smbus_encode_packet(&pkt, buf, 2, &out_len));

    pkt.proto = SYN_SMBUS_PROTO_RECEIVE_BYTE;
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_smbus_encode_packet(&pkt, buf, 1, &out_len));

    pkt.proto = SYN_SMBUS_PROTO_WRITE_BYTE;
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_smbus_encode_packet(&pkt, buf, 3, &out_len));

    pkt.proto = SYN_SMBUS_PROTO_READ_BYTE;
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_smbus_encode_packet(&pkt, buf, 2, &out_len));

    pkt.proto = SYN_SMBUS_PROTO_WRITE_WORD;
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_smbus_encode_packet(&pkt, buf, 2, &out_len));

    pkt.proto = SYN_SMBUS_PROTO_READ_WORD;
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_smbus_encode_packet(&pkt, buf, 2, &out_len));

    pkt.proto = SYN_SMBUS_PROTO_WRITE_BLOCK;
    pkt.length = 5;
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_smbus_encode_packet(&pkt, buf, 4, &out_len));

    pkt.proto = SYN_SMBUS_PROTO_READ_BLOCK;
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_smbus_encode_packet(&pkt, buf, 2, &out_len));

    pkt.proto = SYN_SMBUS_PROTO_PROCESS_CALL;
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_smbus_encode_packet(&pkt, buf, 3, &out_len));

    pkt.proto = SYN_SMBUS_PROTO_BLOCK_PROCESS_CALL;
    pkt.length = 5;
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_smbus_encode_packet(&pkt, buf, 4, &out_len));

    pkt.proto = (SYN_SMBUS_Protocol)99;
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_smbus_encode_packet(&pkt, buf, sizeof(buf), &out_len));
}

void test_smbus_alert_and_host_notify(void)
{
    /* Test Alert Response Address (0x0C) - Receive Byte protocol */
    SYN_SMBUS_Packet alert_pkt;
    alert_pkt.slave_addr = SYN_SMBUS_ADDR_ALERT_RESPONSE;
    alert_pkt.proto = SYN_SMBUS_PROTO_RECEIVE_BYTE;
    alert_pkt.pec_enabled = true;
    alert_pkt.length = 0;

    uint8_t buf[64];
    size_t out_len = 0;
    TEST_ASSERT_EQUAL(SYN_OK, syn_smbus_encode_packet(&alert_pkt, buf, sizeof(buf), &out_len));
    TEST_ASSERT_EQUAL_HEX8(0x19, buf[0]); /* (0x0C << 1) | 1 (Read bit) */

    /* Simulate slave response with device address 0x5A + PEC */
    uint8_t rx_alert[] = {(0x5A << 1), 0x00}; /* Addr 0x5A, PEC */
    uint8_t expected_pec = syn_smbus_calc_pec(0, rx_alert, 1);
    rx_alert[1] = expected_pec;

    SYN_SMBUS_Packet decoded;
    TEST_ASSERT_EQUAL(
        SYN_OK, syn_smbus_decode_packet(&decoded, rx_alert, 2, SYN_SMBUS_PROTO_RECEIVE_BYTE, true));
    TEST_ASSERT_EQUAL_HEX8(0x5A << 1, decoded.data[0]);
    TEST_ASSERT_TRUE(decoded.pec_valid);

    /* Test SYN_SMBUS_PROTO_READ_WORD with data_len >= 4 (lines 230-232) */
    uint8_t word_rx4[4] = {0x11, 0x22, 0x33, 0x44};
    SYN_SMBUS_Packet word_pkt;
    TEST_ASSERT_EQUAL(
        SYN_OK, syn_smbus_decode_packet(&word_pkt, word_rx4, 4, SYN_SMBUS_PROTO_READ_WORD, false));
    TEST_ASSERT_EQUAL_INT(2, word_pkt.length);
    TEST_ASSERT_EQUAL_HEX8(0x33, word_pkt.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x44, word_pkt.data[1]);
}

void run_smbus_tests(void)
{
    RUN_TEST(test_smbus_pec_calculation);
    RUN_TEST(test_smbus_encode_decode_write_word);
    RUN_TEST(test_smbus_decode_invalid_pec);
    RUN_TEST(test_smbus_block_process_call);
    RUN_TEST(test_smbus_quick_and_byte_protocols);
    RUN_TEST(test_smbus_decoding_variations);
    RUN_TEST(test_smbus_invalid_params_and_overflow);
    RUN_TEST(test_smbus_alert_and_host_notify);
}
