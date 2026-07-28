/**
 * @file test_icmp.c
 * @brief Unit tests for Zero-Heap Native ICMP Protocol Engine.
 */

#include "syntropic/net/syn_icmp.h"
#include "unity/unity.h"

#include <string.h>

static const uint8_t MY_MAC[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
static const uint8_t PEER_MAC[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x02};
static const uint32_t MY_IP = 0xC0A80164;   /* 192.168.1.100 */
static const uint32_t PEER_IP = 0xC0A801C8; /* 192.168.1.200 */

void test_icmp_init(void)
{
    SYN_ICMP icmp;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_icmp_init(&icmp));
    TEST_ASSERT_EQUAL_UINT32(0, icmp.echo_requests_rx);
    TEST_ASSERT_EQUAL_UINT32(0, icmp.echo_replies_tx);
}

void test_icmp_checksum(void)
{
    uint8_t data[8] = {0x08, 0x00, 0x00, 0x00, 0x12, 0x34, 0x00, 0x01};
    uint16_t csum = syn_icmp_checksum(data, 8);
    TEST_ASSERT_NOT_EQUAL(0, csum);

    /* Verify checksum validation over whole block yields 0 */
    data[2] = (uint8_t)(csum >> 8);
    data[3] = (uint8_t)(csum & 0xFF);
    TEST_ASSERT_EQUAL_UINT16(0, syn_icmp_checksum(data, 8));
}

void test_icmp_build_echo_request(void)
{
    SYN_ICMP icmp;
    SYN_ETH eth;
    syn_icmp_init(&icmp);
    syn_eth_init(&eth, MY_MAC, MY_IP);

    uint8_t payload[8] = "PINGTEST";
    uint8_t frame[128];
    size_t frame_len = 0;

    TEST_ASSERT_EQUAL_INT(SYN_OK,
                          syn_icmp_build_echo_request(&icmp, &eth, PEER_IP, PEER_MAC, 0x1234,
                                                      0x0001, payload, 8, frame, &frame_len));
    TEST_ASSERT_EQUAL_INT(60, frame_len); /* 14 + 20 + 8 + 8 = 50 padded to 60 */
    TEST_ASSERT_EQUAL_UINT32(1, icmp.echo_requests_tx);

    /* Verify Ethernet Header */
    TEST_ASSERT_EQUAL_INT(0, memcmp(PEER_MAC, &frame[0], 6));
    TEST_ASSERT_EQUAL_INT(0, memcmp(MY_MAC, &frame[6], 6));
    TEST_ASSERT_EQUAL_UINT8(0x08, frame[12]);
    TEST_ASSERT_EQUAL_UINT8(0x00, frame[13]);

    /* Verify IP Header */
    TEST_ASSERT_EQUAL_UINT8(0x45, frame[14]);
    TEST_ASSERT_EQUAL_UINT8(1, frame[23]); /* Protocol 1 = ICMP */

    /* Verify ICMP Header */
    TEST_ASSERT_EQUAL_UINT8(SYN_ICMP_TYPE_ECHO_REQUEST, frame[34]);
    TEST_ASSERT_EQUAL_UINT8(0x12, frame[38]);
    TEST_ASSERT_EQUAL_UINT8(0x34, frame[39]);
}

void test_icmp_process_echo_request(void)
{
    SYN_ICMP icmp;
    SYN_ETH eth;
    syn_icmp_init(&icmp);
    syn_eth_init(&eth, MY_MAC, MY_IP);

    /* Build an inbound Echo Request frame */
    uint8_t in_req[60];
    size_t in_len = 0;
    syn_icmp_build_echo_request(&icmp, &eth, MY_IP, MY_MAC, 0x5555, 0x0002,
                                (const uint8_t *)"HELLO", 5, in_req, &in_len);

    /* Process inbound request */
    uint8_t out_reply[128];
    size_t out_len = 0;
    TEST_ASSERT_EQUAL_INT(SYN_OK,
                          syn_icmp_process_packet(&icmp, in_req, in_len, out_reply, &out_len));
    TEST_ASSERT_EQUAL_INT(60, out_len);
    TEST_ASSERT_EQUAL_UINT32(1, icmp.echo_requests_rx);
    TEST_ASSERT_EQUAL_UINT32(1, icmp.echo_replies_tx);

    /* Verify reply ICMP Type is 0 (Echo Reply) */
    TEST_ASSERT_EQUAL_UINT8(SYN_ICMP_TYPE_ECHO_REPLY, out_reply[34]);
}

void test_icmp_null_checks(void)
{
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_icmp_init(NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_icmp_process_packet(NULL, NULL, 0, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_icmp_build_echo_request(NULL, NULL, 0, NULL, 0, 0,
                                                                         NULL, 0, NULL, NULL));
}
