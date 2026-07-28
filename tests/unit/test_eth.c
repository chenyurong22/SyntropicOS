/**
 * @file test_eth.c
 * @brief Unit tests for Zero-Heap Native Ethernet II & ARP Protocol Engine.
 */

#include "syntropic/net/syn_eth.h"
#include "unity/unity.h"

#include <string.h>

static const uint8_t MY_MAC[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
static const uint8_t PEER_MAC[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x02};
static const uint32_t MY_IP = 0xC0A80164;   /* 192.168.1.100 */
static const uint32_t PEER_IP = 0xC0A801C8; /* 192.168.1.200 */

void test_eth_generate_mac(void)
{
    uint8_t uid[12] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C};
    uint8_t mac1[6];
    uint8_t mac2[6];

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_generate_mac(uid, 12, mac1));
    TEST_ASSERT_EQUAL_UINT8(0x02, mac1[0]); /* Bit 1 set -> Locally Administered MAC */

    /* Deterministic: Same UID produces exact same MAC */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_generate_mac(uid, 12, mac2));
    TEST_ASSERT_EQUAL_INT(0, memcmp(mac1, mac2, 6));

    /* Different UID produces different MAC */
    uid[0] = 0xFF;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_generate_mac(uid, 12, mac2));
    TEST_ASSERT_NOT_EQUAL(0, memcmp(mac1, mac2, 6));
}

void test_eth_init(void)
{
    SYN_ETH eth;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_init(&eth, MY_MAC, MY_IP));
    TEST_ASSERT_EQUAL_UINT32(MY_IP, eth.ip_addr);
    TEST_ASSERT_EQUAL_INT(0, memcmp(MY_MAC, eth.mac_addr, 6));
}

void test_eth_arp_cache(void)
{
    SYN_ETH eth;
    syn_eth_init(&eth, MY_MAC, MY_IP);

    uint8_t mac_out[6];
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_eth_arp_lookup(&eth, PEER_IP, mac_out));

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_arp_update(&eth, PEER_IP, PEER_MAC));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_arp_lookup(&eth, PEER_IP, mac_out));
    TEST_ASSERT_EQUAL_INT(0, memcmp(PEER_MAC, mac_out, 6));
}

void test_eth_build_frame(void)
{
    SYN_ETH eth;
    syn_eth_init(&eth, MY_MAC, MY_IP);

    uint8_t payload[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint8_t frame[128];
    size_t frame_len = 0;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_build_frame(&eth, PEER_MAC, SYN_ETHTYPE_IPV4, payload, 10,
                                                      frame, &frame_len));
    TEST_ASSERT_EQUAL_INT(60, frame_len); /* Padded to 60 bytes minimum */

    TEST_ASSERT_EQUAL_INT(0, memcmp(PEER_MAC, &frame[0], 6));
    TEST_ASSERT_EQUAL_INT(0, memcmp(MY_MAC, &frame[6], 6));
    TEST_ASSERT_EQUAL_UINT8(0x08, frame[12]);
    TEST_ASSERT_EQUAL_UINT8(0x00, frame[13]);
    TEST_ASSERT_EQUAL_UINT8(1, frame[14]);
    TEST_ASSERT_EQUAL_UINT8(10, frame[23]);
}

void test_eth_process_arp_request(void)
{
    SYN_ETH eth;
    syn_eth_init(&eth, MY_MAC, MY_IP);

    /* Construct ARP Request frame asking for MY_IP */
    uint8_t arp_req_frame[60] = {0};

    /* Dst MAC: Broadcast */
    memset(&arp_req_frame[0], 0xFF, 6);
    /* Src MAC */
    memcpy(&arp_req_frame[6], PEER_MAC, 6);
    /* EtherType = ARP (0x0806) */
    arp_req_frame[12] = 0x08;
    arp_req_frame[13] = 0x06;

    /* ARP Payload */
    arp_req_frame[14] = 0;
    arp_req_frame[15] = 1; /* Ethernet */
    arp_req_frame[16] = 0x08;
    arp_req_frame[17] = 0x00; /* IPv4 */
    arp_req_frame[18] = 6;
    arp_req_frame[19] = 4;
    arp_req_frame[20] = 0;
    arp_req_frame[21] = 1; /* Request */

    memcpy(&arp_req_frame[22], PEER_MAC, 6);
    arp_req_frame[28] = (uint8_t)(PEER_IP >> 24);
    arp_req_frame[29] = (uint8_t)(PEER_IP >> 16);
    arp_req_frame[30] = (uint8_t)(PEER_IP >> 8);
    arp_req_frame[31] = (uint8_t)(PEER_IP);

    memset(&arp_req_frame[32], 0, 6);
    arp_req_frame[38] = (uint8_t)(MY_IP >> 24);
    arp_req_frame[39] = (uint8_t)(MY_IP >> 16);
    arp_req_frame[40] = (uint8_t)(MY_IP >> 8);
    arp_req_frame[41] = (uint8_t)(MY_IP);

    uint8_t tx_reply[128];
    size_t tx_len = 0;

    TEST_ASSERT_EQUAL_INT(SYN_OK,
                          syn_eth_process_frame(&eth, arp_req_frame, 60, tx_reply, &tx_len));
    TEST_ASSERT_EQUAL_INT(60, tx_len);
    TEST_ASSERT_EQUAL_UINT32(1, eth.arp_requests);

    /* Verify ARP Reply Dst MAC is PEER_MAC */
    TEST_ASSERT_EQUAL_INT(0, memcmp(PEER_MAC, &tx_reply[0], 6));

    /* Verify ARP cache auto-learned PEER_IP -> PEER_MAC */
    uint8_t mac_learned[6];
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_arp_lookup(&eth, PEER_IP, mac_learned));
    TEST_ASSERT_EQUAL_INT(0, memcmp(PEER_MAC, mac_learned, 6));
}

static SYN_PT_Status eth_coroutine_task(SYN_PT *pt, SYN_ETH *eth, uint32_t ip, uint8_t *mac_out)
{
    PT_BEGIN(pt);
    PT_ETH_WAIT_ARP(pt, eth, ip, mac_out);
    PT_END(pt);
}

void test_eth_coroutine_pt(void)
{
    SYN_ETH eth;
    syn_eth_init(&eth, MY_MAC, MY_IP);

    SYN_PT pt;
    PT_INIT(&pt);

    uint8_t mac[6];
    /* First step: ARP lookup fails (not found) -> coroutine yields (PT_WAITING) */
    TEST_ASSERT_EQUAL_INT(PT_WAITING, eth_coroutine_task(&pt, &eth, PEER_IP, mac));

    /* Insert entry into ARP cache */
    syn_eth_arp_update(&eth, PEER_IP, PEER_MAC);

    /* Second step: ARP lookup succeeds -> coroutine completes (PT_EXITED) */
    TEST_ASSERT_EQUAL_INT(PT_EXITED, eth_coroutine_task(&pt, &eth, PEER_IP, mac));
    TEST_ASSERT_EQUAL_INT(0, memcmp(PEER_MAC, mac, 6));
}

void test_eth_null_checks(void)
{
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_eth_init(NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_eth_process_frame(NULL, NULL, 0, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM,
                          syn_eth_build_frame(NULL, NULL, 0, NULL, 0, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_eth_arp_lookup(NULL, 0, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_eth_arp_update(NULL, 0, NULL));
}

void test_eth_runt_and_oversized_frames(void)
{
    SYN_ETH eth;
    syn_eth_init(&eth, MY_MAC, MY_IP);

    uint8_t runt[13] = {0};
    uint8_t tx[128];
    size_t tx_len = 0;

    /* Runt frame (< 14 bytes) must return SYN_INVALID_PARAM */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_eth_process_frame(&eth, runt, 13, tx, &tx_len));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_eth_process_frame(&eth, runt, 5, tx, &tx_len));

    /* Building frame exceeding 1514 bytes maximum length must fail */
    uint8_t large_payload[1510];
    uint8_t large_out[1600];
    size_t out_len = 0;
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM,
                          syn_eth_build_frame(&eth, PEER_MAC, SYN_ETHTYPE_IPV4, large_payload, 1510,
                                              large_out, &out_len));
}

void test_eth_mac_filtering(void)
{
    SYN_ETH eth;
    syn_eth_init(&eth, MY_MAC, MY_IP);

    /* Construct frame destined for alien MAC 02:99:88:77:66:55 */
    uint8_t alien_frame[60] = {0x02, 0x99, 0x88, 0x77, 0x66, 0x55};
    memcpy(&alien_frame[6], PEER_MAC, 6);
    alien_frame[12] = 0x08;
    alien_frame[13] = 0x06;

    uint8_t tx[128];
    size_t tx_len = 999;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_process_frame(&eth, alien_frame, 60, tx, &tx_len));
    TEST_ASSERT_EQUAL_INT(0, tx_len); /* Dropped by MAC filter, no reply generated */
}

void test_eth_arp_cache_eviction_overflow(void)
{
    SYN_ETH eth;
    syn_eth_init(&eth, MY_MAC, MY_IP);

    /* Fill all 8 ARP entries + 2 overflow entries */
    for (uint32_t i = 1; i <= 10; i++) {
        uint32_t test_ip = 0x0A000000 | i; /* 10.0.0.i */
        uint8_t test_mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, (uint8_t)i};
        TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_arp_update(&eth, test_ip, test_mac));
    }

    /* Entry 10 should have overwritten Entry 0 (10.0.0.10) */
    uint8_t mac_out[6];
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_arp_lookup(&eth, 0x0A00000A, mac_out));
    TEST_ASSERT_EQUAL_UINT8(10, mac_out[5]);
}

void test_eth_multiprotocol_interleaving(void)
{
    SYN_ETH eth;
    syn_eth_init(&eth, MY_MAC, MY_IP);

    uint8_t frame_arp[60] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    memcpy(&frame_arp[6], PEER_MAC, 6);
    frame_arp[12] = 0x08;
    frame_arp[13] = 0x06;

    uint8_t frame_ecat[60] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    memcpy(&frame_ecat[6], PEER_MAC, 6);
    frame_ecat[12] = 0x88;
    frame_ecat[13] = 0xA4; /* EtherCAT */

    uint8_t tx[128];
    size_t tx_len = 0;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_process_frame(&eth, frame_arp, 60, tx, &tx_len));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_process_frame(&eth, frame_ecat, 60, tx, &tx_len));
    TEST_ASSERT_EQUAL_UINT32(2, eth.frames_rx);
}
