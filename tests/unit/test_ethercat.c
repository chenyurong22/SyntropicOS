/**
 * @file test_ethercat.c
 * @brief Unity tests for EtherCAT Master/Slave Protocol Engine (syn_ethercat).
 */

#include "mocks/mock_port.h"
#include "syntropic/proto/syn_ethercat.h"
#include "syntropic/syntropic.h"
#include "unity/unity.h"

#include <string.h>

static void test_ecat_init_and_esm(void)
{
    SYN_EcatNode node;
    syn_ecat_init(&node, 0x1001, NULL);

    TEST_ASSERT_EQUAL(SYN_ECAT_STATE_INIT, node.state);
    TEST_ASSERT_EQUAL(SYN_ECAT_STATE_INIT, node.target_state);
    TEST_ASSERT_EQUAL_UINT16(0x1001, node.station_addr);

    /* Request PREOP state */
    TEST_ASSERT_EQUAL(SYN_OK, syn_ecat_set_state(&node, SYN_ECAT_STATE_PREOP));
    TEST_ASSERT_EQUAL(SYN_ECAT_STATE_PREOP, node.target_state);

    syn_ecat_update(&node);
    TEST_ASSERT_EQUAL(SYN_ECAT_STATE_PREOP, node.state);

    /* Request SAFEOP state */
    TEST_ASSERT_EQUAL(SYN_OK, syn_ecat_set_state(&node, SYN_ECAT_STATE_SAFEOP));
    syn_ecat_update(&node);
    TEST_ASSERT_EQUAL(SYN_ECAT_STATE_SAFEOP, node.state);

    /* Request OP state */
    TEST_ASSERT_EQUAL(SYN_OK, syn_ecat_set_state(&node, SYN_ECAT_STATE_OP));
    syn_ecat_update(&node);
    TEST_ASSERT_EQUAL(SYN_ECAT_STATE_OP, node.state);

    /* Invalid state request */
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_ecat_set_state(&node, (SYN_EcatState)0xFF));
}

static void test_ecat_datagram_build_and_parse(void)
{
    SYN_EcatNode node;
    syn_ecat_init(&node, 0x1001, NULL);

    uint8_t frame_buf[64];
    SYN_EcatDatagram dg = {.cmd = SYN_ECAT_CMD_FPRD,
                           .idx = 0x42,
                           .addr = 0x10010000,
                           .m = 0,
                           .circ = 0,
                           .irq = 0x0000};

    uint8_t payload[4] = {0x11, 0x22, 0x33, 0x44};
    size_t frame_len = syn_ecat_build_datagram_frame(frame_buf, sizeof(frame_buf), &dg, payload, 4);

    /* Total = 2 (header) + 10 (dg header) + 4 (data) + 2 (wkc) = 18 bytes */
    TEST_ASSERT_EQUAL_INT(18, frame_len);

    /* Simulate Master/Slave incrementing WKC on datagram read */
    frame_buf[16] = 0x01;
    frame_buf[17] = 0x00;

    uint16_t wkc = 0;
    SYN_Status st = syn_ecat_parse_frame(&node, frame_buf, frame_len, &wkc);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL_UINT16(1, wkc);
    TEST_ASSERT_EQUAL_UINT16(1, node.wkc_last);

    /* Parse corrupted/short frame */
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_ecat_parse_frame(&node, frame_buf, 5, &wkc));

    /* Parse frame with WKC mismatch */
    frame_buf[16] = 0x00;
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_ecat_parse_frame(&node, frame_buf, frame_len, &wkc));
}

static void test_ecat_coe_sdo_mailbox(void)
{
    uint8_t mbox_buf[32];

    /* SDO Download (Write) Request */
    uint32_t val = 0x12345678;
    size_t len = syn_ecat_coe_encode_sdo_download(mbox_buf, sizeof(mbox_buf), 0x6040, 0x00, &val,
                                                  sizeof(val));
    TEST_ASSERT_EQUAL_INT(10, len);

    /* Check Mailbox CoE header (Type 3 SDO Req) */
    uint16_t coe_hdr = (uint16_t)(mbox_buf[0] | (mbox_buf[1] << 8));
    TEST_ASSERT_EQUAL_UINT16((SYN_ECAT_COE_TYPE_SDO_REQ & 0x0F) << 12, coe_hdr);
    TEST_ASSERT_EQUAL_HEX8(0x40, mbox_buf[3]); /* Index LSB (0x6040 -> 0x40) */
    TEST_ASSERT_EQUAL_HEX8(0x60, mbox_buf[4]); /* Index MSB (0x6040 -> 0x60) */
    TEST_ASSERT_EQUAL_HEX8(0x00, mbox_buf[5]); /* Subindex */

    /* SDO Upload (Read) Request */
    len = syn_ecat_coe_encode_sdo_upload(mbox_buf, sizeof(mbox_buf), 0x6041, 0x00);
    TEST_ASSERT_EQUAL_INT(10, len);
    TEST_ASSERT_EQUAL_HEX8(0x40, mbox_buf[2]); /* 0x40 Upload Command */
    TEST_ASSERT_EQUAL_HEX8(0x41, mbox_buf[3]);
    TEST_ASSERT_EQUAL_HEX8(0x60, mbox_buf[4]);
}

static void test_ecat_edge_cases(void)
{
    SYN_EcatNode node;
    syn_ecat_init(&node, 0x1001, NULL);

    uint8_t buf[64];
    SYN_EcatDatagram dg = {
        .cmd = SYN_ECAT_CMD_APWR, .idx = 0x01, .addr = 0x0000, .m = 1, .circ = 1, .irq = 0x0000};

    /* Buffer capacity error */
    TEST_ASSERT_EQUAL_INT(0, syn_ecat_build_datagram_frame(buf, 10, &dg, NULL, 0));

    /* Build datagram with m=1 and circ=1 flags set */
    size_t len = syn_ecat_build_datagram_frame(buf, sizeof(buf), &dg, NULL, 0);
    TEST_ASSERT_EQUAL_INT(14, len);

    /* Invalid frame type in parser */
    buf[1] = 0x20; /* Frame type = 2 instead of 1 */
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_ecat_parse_frame(&node, buf, len, NULL));

    /* Truncated datagram length in header */
    buf[0] = 0xFF; /* dg_len = 0x07FF (2047 bytes) */
    buf[1] = 0x17;
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_ecat_parse_frame(&node, buf, len, NULL));

    /* SDO Download buffer capacity / payload size overflow */
    uint32_t val = 123;
    TEST_ASSERT_EQUAL_INT(
        0, syn_ecat_coe_encode_sdo_download(buf, sizeof(buf), 0x1000, 0x00, &val, 8));
    TEST_ASSERT_EQUAL_INT(0, syn_ecat_coe_encode_sdo_download(buf, 5, 0x1000, 0x00, &val, 4));

    /* SDO Upload buffer capacity overflow */
    TEST_ASSERT_EQUAL_INT(0, syn_ecat_coe_encode_sdo_upload(buf, 5, 0x1000, 0x00));
}

void run_ethercat_tests(void)
{
    RUN_TEST(test_ecat_init_and_esm);
    RUN_TEST(test_ecat_datagram_build_and_parse);
    RUN_TEST(test_ecat_coe_sdo_mailbox);
    RUN_TEST(test_ecat_edge_cases);
}
