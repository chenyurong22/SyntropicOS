/**
 * @file test_dhcp.c
 * @brief Unit tests for Zero-Heap Native DHCP Client Protocol Engine.
 */

#include "syntropic/net/syn_dhcp.h"
#include "unity/unity.h"

#include <string.h>

static const uint8_t MAC[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
static const uint32_t XID = 0x87654321UL;

void test_dhcp_init(void)
{
    SYN_DHCP dhcp;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_dhcp_init(&dhcp, XID));
    TEST_ASSERT_EQUAL_INT(SYN_DHCP_STATE_INIT, dhcp.state);
    TEST_ASSERT_EQUAL_UINT32(XID, dhcp.xid);
}

void test_dhcp_build_discover(void)
{
    SYN_DHCP dhcp;
    syn_dhcp_init(&dhcp, XID);

    uint8_t buf[300];
    size_t len = 0;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_dhcp_build_discover(&dhcp, MAC, buf, sizeof(buf), &len));
    TEST_ASSERT_EQUAL_INT(SYN_DHCP_STATE_DISCOVER, dhcp.state);
    TEST_ASSERT_EQUAL_UINT32(1, dhcp.discovers_sent);
    TEST_ASSERT_GREATER_OR_EQUAL(244, len);

    /* Verify BOOTP Header */
    TEST_ASSERT_EQUAL_UINT8(1, buf[0]); /* op = Boot Request */
    TEST_ASSERT_EQUAL_UINT8(1, buf[1]); /* htype = Ethernet */
    TEST_ASSERT_EQUAL_UINT8(6, buf[2]); /* hlen = 6 */

    /* Verify Magic Cookie (0x63, 0x82, 0x53, 0x63) */
    TEST_ASSERT_EQUAL_UINT8(0x63, buf[236]);
    TEST_ASSERT_EQUAL_UINT8(0x82, buf[237]);
    TEST_ASSERT_EQUAL_UINT8(0x53, buf[238]);
    TEST_ASSERT_EQUAL_UINT8(0x63, buf[239]);

    /* Verify Option 53 = DHCPDISCOVER (1) */
    TEST_ASSERT_EQUAL_UINT8(53, buf[240]);
    TEST_ASSERT_EQUAL_UINT8(1, buf[241]);
    TEST_ASSERT_EQUAL_UINT8(1, buf[242]);
}

void test_dhcp_process_offer_and_ack(void)
{
    SYN_DHCP dhcp;
    SYN_ETH eth;
    syn_eth_init(&eth, MAC, 0);
    syn_dhcp_init(&dhcp, XID);

    /* Construct DHCPOFFER payload */
    uint8_t offer_pkt[300] = {0};
    offer_pkt[0] = 2; /* Boot Reply */
    offer_pkt[1] = 1;
    offer_pkt[2] = 6;
    /* XID */
    offer_pkt[4] = (uint8_t)(XID >> 24);
    offer_pkt[5] = (uint8_t)(XID >> 16);
    offer_pkt[6] = (uint8_t)(XID >> 8);
    offer_pkt[7] = (uint8_t)(XID);
    /* yiaddr = 192.168.1.150 (0xC0A80196) */
    offer_pkt[16] = 192;
    offer_pkt[17] = 168;
    offer_pkt[18] = 1;
    offer_pkt[19] = 150;
    /* Magic Cookie */
    offer_pkt[236] = 0x63;
    offer_pkt[237] = 0x82;
    offer_pkt[238] = 0x53;
    offer_pkt[239] = 0x63;

    /* Options: Option 53 = DHCPOFFER (2), Option 1 = Netmask (255.255.255.0), Option 3 = Gateway
     * (192.168.1.1), Option 255 = END */
    size_t idx = 240;
    offer_pkt[idx++] = 53;
    offer_pkt[idx++] = 1;
    offer_pkt[idx++] = SYN_DHCP_OFFER;
    offer_pkt[idx++] = 1;
    offer_pkt[idx++] = 4;
    offer_pkt[idx++] = 255;
    offer_pkt[idx++] = 255;
    offer_pkt[idx++] = 255;
    offer_pkt[idx++] = 0;
    offer_pkt[idx++] = 3;
    offer_pkt[idx++] = 4;
    offer_pkt[idx++] = 192;
    offer_pkt[idx++] = 168;
    offer_pkt[idx++] = 1;
    offer_pkt[idx++] = 1;
    offer_pkt[idx++] = 255;

    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_dhcp_process_packet(&dhcp, &eth, offer_pkt, idx));
    TEST_ASSERT_EQUAL_INT(SYN_DHCP_STATE_OFFER, dhcp.state);
    TEST_ASSERT_EQUAL_UINT32(0xC0A80196, dhcp.offered_ip);

    /* Build DHCPREQUEST */
    uint8_t req_buf[300];
    size_t req_len = 0;
    SYN_Status req_st = syn_dhcp_build_request(&dhcp, MAC, req_buf, sizeof(req_buf), &req_len);
    TEST_ASSERT_EQUAL_INT(SYN_OK, req_st);
    TEST_ASSERT_EQUAL_INT(SYN_DHCP_STATE_REQUEST, dhcp.state);

    /* Construct DHCPACK payload */
    uint8_t ack_pkt[300] = {0};
    memcpy(ack_pkt, offer_pkt, idx);
    ack_pkt[242] = SYN_DHCP_ACK; /* Option 53 = DHCPACK (5) */

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_dhcp_process_packet(&dhcp, &eth, ack_pkt, idx));
    TEST_ASSERT_EQUAL_INT(SYN_DHCP_STATE_BOUND, dhcp.state);
    TEST_ASSERT_EQUAL_UINT32(1, dhcp.acks_received);

    /* Verify Ethernet interface IP address updated automatically */
    TEST_ASSERT_EQUAL_UINT32(0xC0A80196, eth.ip_addr);
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFF00, eth.netmask);
    TEST_ASSERT_EQUAL_UINT32(0xC0A80101, eth.gateway);
}

static SYN_PT_Status dhcp_coroutine_task(SYN_PT *pt, SYN_DHCP *dhcp)
{
    PT_BEGIN(pt);
    PT_DHCP_WAIT_BOUND(pt, dhcp);
    PT_END(pt);
}

void test_dhcp_coroutine_pt(void)
{
    SYN_DHCP dhcp;
    syn_dhcp_init(&dhcp, XID);

    SYN_PT pt;
    PT_INIT(&pt);

    /* First step: DHCP state != BOUND -> coroutine yields (PT_WAITING) */
    TEST_ASSERT_EQUAL_INT(PT_WAITING, dhcp_coroutine_task(&pt, &dhcp));

    /* Simulate binding completion */
    dhcp.state = SYN_DHCP_STATE_BOUND;

    /* Second step: DHCP state == BOUND -> coroutine completes (PT_EXITED) */
    TEST_ASSERT_EQUAL_INT(PT_EXITED, dhcp_coroutine_task(&pt, &dhcp));
}

void test_dhcp_extended_option_parsing(void)
{
    SYN_DHCP dhcp;
    syn_dhcp_init(&dhcp, XID);

    /* Construct DHCPOFFER with pad option (0), Option 51 (lease time = 3600), Option 54 (server IP
     * = 192.168.1.254) */
    uint8_t offer_pkt[300] = {0};
    offer_pkt[0] = 2;
    offer_pkt[4] = (uint8_t)(XID >> 24);
    offer_pkt[5] = (uint8_t)(XID >> 16);
    offer_pkt[6] = (uint8_t)(XID >> 8);
    offer_pkt[7] = (uint8_t)(XID);
    offer_pkt[16] = 10;
    offer_pkt[17] = 0;
    offer_pkt[18] = 0;
    offer_pkt[19] = 50;

    /* Magic Cookie */
    offer_pkt[236] = 0x63;
    offer_pkt[237] = 0x82;
    offer_pkt[238] = 0x53;
    offer_pkt[239] = 0x63;

    size_t idx = 240;
    offer_pkt[idx++] = 0; /* Option 0: Pad */
    offer_pkt[idx++] = 53;
    offer_pkt[idx++] = 1;
    offer_pkt[idx++] = SYN_DHCP_OFFER;
    offer_pkt[idx++] = 51; /* Option 51: Lease Time */
    offer_pkt[idx++] = 4;
    offer_pkt[idx++] = 0;
    offer_pkt[idx++] = 0;
    offer_pkt[idx++] = 0x0E;
    offer_pkt[idx++] = 0x10; /* 3600 */
    offer_pkt[idx++] = 54;   /* Option 54: Server Identifier */
    offer_pkt[idx++] = 4;
    offer_pkt[idx++] = 192;
    offer_pkt[idx++] = 168;
    offer_pkt[idx++] = 1;
    offer_pkt[idx++] = 254;
    offer_pkt[idx++] = 255; /* END */

    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_dhcp_process_packet(&dhcp, NULL, offer_pkt, idx));
    TEST_ASSERT_EQUAL_UINT32(3600, dhcp.lease_time_sec);
    TEST_ASSERT_EQUAL_UINT32(0xC0A801FE, dhcp.server_ip);

    /* Test XID mismatch */
    offer_pkt[4] = 0xFF;
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_dhcp_process_packet(&dhcp, NULL, offer_pkt, idx));

    /* Test invalid magic cookie (line 135) */
    offer_pkt[4] = (uint8_t)(XID >> 24); /* Restore XID */
    offer_pkt[236] = 0x00;               /* Corrupt magic cookie */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_dhcp_process_packet(&dhcp, NULL, offer_pkt, idx));

    /* Test build request with non-zero server_ip (lines 111-114) */
    dhcp.state = SYN_DHCP_STATE_DISCOVER;
    dhcp.server_ip = 0xC0A80101;
    uint8_t mac[6] = {1, 2, 3, 4, 5, 6};
    uint8_t req_b[300];
    size_t req_l = 0;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_dhcp_build_request(&dhcp, mac, req_b, sizeof(req_b), &req_l));

    /* Truncated options tests */
    uint8_t trunc_pkt[250] = {0};
    trunc_pkt[4] = (uint8_t)(XID >> 24);
    trunc_pkt[5] = (uint8_t)(XID >> 16);
    trunc_pkt[6] = (uint8_t)(XID >> 8);
    trunc_pkt[7] = (uint8_t)(XID);
    trunc_pkt[236] = 0x63;
    trunc_pkt[237] = 0x82;
    trunc_pkt[238] = 0x53;
    trunc_pkt[239] = 0x63;
    trunc_pkt[240] = 53; /* MSG_TYPE option */
    trunc_pkt[241] = 1;  /* len 1 */
    trunc_pkt[242] = 2;  /* OFFER */
    trunc_pkt[243] = 50; /* missing length byte at end of 244 bytes */
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_dhcp_process_packet(&dhcp, NULL, trunc_pkt, 244));

    trunc_pkt[244] = 10; /* opt_len 10 > 0 remaining bytes */
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_dhcp_process_packet(&dhcp, NULL, trunc_pkt, 245));

    /* Unknown message type (99) with valid length >= 244 */
    trunc_pkt[242] = 99;
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_dhcp_process_packet(&dhcp, NULL, trunc_pkt, 244));

    /* Unknown option (e.g. Option 99) with length 2 */
    uint8_t unk_opt_pkt[300] = {0};
    unk_opt_pkt[4] = (uint8_t)(XID >> 24);
    unk_opt_pkt[5] = (uint8_t)(XID >> 16);
    unk_opt_pkt[6] = (uint8_t)(XID >> 8);
    unk_opt_pkt[7] = (uint8_t)(XID);
    unk_opt_pkt[236] = 0x63;
    unk_opt_pkt[237] = 0x82;
    unk_opt_pkt[238] = 0x53;
    unk_opt_pkt[239] = 0x63;
    size_t uidx = 240;
    unk_opt_pkt[uidx++] = 99;
    unk_opt_pkt[uidx++] = 2;
    unk_opt_pkt[uidx++] = 0xAA;
    unk_opt_pkt[uidx++] = 0xBB;
    /* Subnet mask option with invalid len < 4 */
    unk_opt_pkt[uidx++] = 1;
    unk_opt_pkt[uidx++] = 2;
    unk_opt_pkt[uidx++] = 0xFF;
    unk_opt_pkt[uidx++] = 0xFF;
    /* Router option with invalid len < 4 */
    unk_opt_pkt[uidx++] = 3;
    unk_opt_pkt[uidx++] = 1;
    unk_opt_pkt[uidx++] = 0x01;
    /* Lease time option with invalid len < 4 */
    unk_opt_pkt[uidx++] = 51;
    unk_opt_pkt[uidx++] = 3;
    unk_opt_pkt[uidx++] = 0x00;
    unk_opt_pkt[uidx++] = 0x01;
    unk_opt_pkt[uidx++] = 0x00;
    /* Server IP option with invalid len < 4 */
    unk_opt_pkt[uidx++] = 54;
    unk_opt_pkt[uidx++] = 2;
    unk_opt_pkt[uidx++] = 0xC0;
    unk_opt_pkt[uidx++] = 0xA8;
    unk_opt_pkt[uidx++] = 255;
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_dhcp_process_packet(&dhcp, NULL, unk_opt_pkt, uidx));

    /* Build request with server_ip == 0 */
    dhcp.server_ip = 0;
    uint8_t req_buf_nosrv[300];
    size_t req_len_nosrv = 0;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_dhcp_build_request(&dhcp, mac, req_buf_nosrv,
                                                         sizeof(req_buf_nosrv), &req_len_nosrv));
}

void test_dhcp_null_checks(void)
{
    SYN_DHCP dhcp;
    uint8_t mac[6] = {1, 2, 3, 4, 5, 6};
    uint8_t small_buf[100];
    size_t len = 0;

    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_dhcp_init(NULL, 0));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_dhcp_build_discover(NULL, NULL, NULL, 0, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_dhcp_build_request(NULL, NULL, NULL, 0, NULL));
    SYN_Status disc_st = syn_dhcp_build_discover(&dhcp, mac, small_buf, sizeof(small_buf), &len);
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, disc_st);

    SYN_Status req_err_st = syn_dhcp_build_request(&dhcp, mac, small_buf, sizeof(small_buf), &len);
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, req_err_st);
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_dhcp_process_packet(NULL, NULL, NULL, 0));
}
