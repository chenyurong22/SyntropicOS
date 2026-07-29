/**
 * @file test_eth.c
 * @brief Unit tests for Zero-Heap Native Ethernet II & ARP Protocol Engine.
 */

#include "mocks/mock_port.h"
#include "syntropic/net/syn_eth.h"
#include "syntropic/net/syn_icmp.h"
#include "syntropic/net/syn_tcp.h"
#include "syntropic/net/syn_transport_udp.h"
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

    /* Null parameter error test */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_eth_generate_mac(NULL, 0, NULL));

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
    /* Miss must return SYN_NOT_FOUND, not SYN_ERROR */
    TEST_ASSERT_EQUAL_INT(SYN_NOT_FOUND, syn_eth_arp_lookup(&eth, PEER_IP, mac_out));

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_arp_update(&eth, PEER_IP, PEER_MAC));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_arp_lookup(&eth, PEER_IP, mac_out));
    TEST_ASSERT_EQUAL_INT(0, memcmp(PEER_MAC, mac_out, 6));
}

/* Regression: arp_lookup must return SYN_NOT_FOUND, not the generic SYN_ERROR. */
void test_eth_arp_lookup_returns_not_found(void)
{
    SYN_ETH eth;
    syn_eth_init(&eth, MY_MAC, MY_IP);

    uint8_t mac_out[6];
    SYN_Status st = syn_eth_arp_lookup(&eth, 0xDEADBEEF, mac_out);
    TEST_ASSERT_EQUAL_INT(SYN_NOT_FOUND, st);
    /* Confirm it's specifically NOT SYN_ERROR */
    TEST_ASSERT_NOT_EQUAL((int)SYN_ERROR, (int)st);
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

    /* ARP update existing entry (lines 87-89) */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_arp_update(&eth, 0x0A000001, PEER_MAC));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_arp_update(&eth, 0x0A000001, PEER_MAC));

    /* Process ARP Reply packet (lines 241-242) */
    uint8_t arp_reply_pkt[60] = {0};
    memcpy(&arp_reply_pkt[0], MY_MAC, 6);
    memcpy(&arp_reply_pkt[6], PEER_MAC, 6);
    arp_reply_pkt[12] = 0x08;
    arp_reply_pkt[13] = 0x06; /* ARP */
    arp_reply_pkt[14] = 0x00;
    arp_reply_pkt[15] = 0x01; /* htype = 1 */
    arp_reply_pkt[16] = 0x08;
    arp_reply_pkt[17] = 0x00; /* ptype = 0x0800 */
    arp_reply_pkt[18] = 6;    /* hlen = 6 */
    arp_reply_pkt[19] = 4;    /* plen = 4 */
    arp_reply_pkt[20] = 0x00;
    arp_reply_pkt[21] = 0x02; /* ARP Reply */
    TEST_ASSERT_EQUAL_INT(
        SYN_OK, syn_eth_process_frame(&eth, arp_reply_pkt, sizeof(arp_reply_pkt), tx, &tx_len));
    TEST_ASSERT_EQUAL_UINT32(1, eth.arp_replies);

    /* Process UDP packet (lines 249-251) */
    uint8_t udp_eth_pkt[64] = {0};
    memcpy(&udp_eth_pkt[0], MY_MAC, 6);
    memcpy(&udp_eth_pkt[6], PEER_MAC, 6);
    udp_eth_pkt[12] = 0x08;
    udp_eth_pkt[13] = 0x00; /* IPv4 */
    udp_eth_pkt[14] = 0x45;
    udp_eth_pkt[23] = 17; /* UDP */
    TEST_ASSERT_EQUAL_INT(
        SYN_OK, syn_eth_process_frame(&eth, udp_eth_pkt, sizeof(udp_eth_pkt), tx, &tx_len));
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

/**
 * Regression: ARP cache full eviction must pick the LRU entry (lowest
 * last_seen_ms), NOT always slot 0.  Fill 8 entries with staggered
 * timestamps so that slot 3 is oldest, then insert a 9th entry and
 * verify that slot 3 is overwritten, not slot 0.
 */
void test_eth_arp_cache_lru_eviction(void)
{
    SYN_ETH eth;
    syn_eth_init(&eth, MY_MAC, MY_IP);

    /* Insert 8 entries. Slot 3 gets timestamp=1 (oldest). All others get >10. */
    for (uint32_t i = 0; i < SYN_ETH_ARP_CACHE_SIZE; i++) {
        mock_tick_ms = (i == 3) ? 1 : ((i + 2) * 10);
        uint32_t ip = 0x0A000001 + i;
        uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, (uint8_t)(i + 1)};
        TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_arp_update(&eth, ip, mac));
    }

    /* Record the IP that occupies slot 3 (will be the LRU victim) */
    uint32_t slot3_ip = eth.arp_cache[3].ip;
    /* Record slot 0's IP to verify it survives */
    uint32_t slot0_ip = eth.arp_cache[0].ip;

    /* Insert a 9th entry — should evict LRU (slot 3), not slot 0 */
    mock_tick_ms = 1000;
    uint32_t new_ip = 0x0B000001;
    uint8_t new_mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0xFF};
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_arp_update(&eth, new_ip, new_mac));

    /* New entry must be present */
    uint8_t mac_out[6];
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_arp_lookup(&eth, new_ip, mac_out));
    TEST_ASSERT_EQUAL_INT(0, memcmp(new_mac, mac_out, 6));

    /* LRU victim (slot 3) must be gone */
    TEST_ASSERT_EQUAL_INT(SYN_NOT_FOUND, syn_eth_arp_lookup(&eth, slot3_ip, mac_out));

    /* Slot 0 must still be present (it had a newer timestamp) */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_arp_lookup(&eth, slot0_ip, mac_out));
}

void test_eth_arp_cache_eviction_overflow(void)
{
    SYN_ETH eth;
    syn_eth_init(&eth, MY_MAC, MY_IP);

    /* Fill 10 entries with monotonically increasing timestamps */
    for (uint32_t i = 1; i <= 10; i++) {
        mock_tick_ms = i * 10;
        uint32_t test_ip = 0x0A000000 | i;
        uint8_t test_mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, (uint8_t)i};
        TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_arp_update(&eth, test_ip, test_mac));
    }

    /* Most recent entries (9, 10) must survive */
    uint8_t mac_out[6];
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_arp_lookup(&eth, 0x0A000009, mac_out));
    TEST_ASSERT_EQUAL_UINT8(9, mac_out[5]);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_arp_lookup(&eth, 0x0A00000A, mac_out));
    TEST_ASSERT_EQUAL_UINT8(10, mac_out[5]);
}

/* ── ETH dispatcher routing ──────────────────────────────────────────────── */

/* ICMP injection hook for dispatcher routing test */
static SYN_ICMP g_test_icmp;
static bool g_icmp_injected = false;

struct SYN_ICMP *syn_eth_get_icmp_instance(void)
{
    return (struct SYN_ICMP *)(g_icmp_injected ? &g_test_icmp : NULL);
}

/** Regression: ETH dispatcher must forward ICMP (proto 1) to the ICMP engine. */
void test_eth_dispatch_icmp(void)
{
    SYN_ETH eth;
    syn_eth_init(&eth, MY_MAC, MY_IP);
    syn_icmp_init(&g_test_icmp);
    g_icmp_injected = true;

    /* Minimal ICMP Echo Request frame (type=8, directed at our IP) */
    uint8_t frame[64];
    memset(frame, 0, sizeof(frame));
    memcpy(&frame[0], MY_MAC, 6);
    memcpy(&frame[6], PEER_MAC, 6);
    frame[12] = 0x08;
    frame[13] = 0x00; /* IPv4 */
    frame[14] = 0x45; /* IHL=5 */
    frame[23] = 1;    /* Protocol = ICMP */
    frame[26] = (uint8_t)(PEER_IP >> 24);
    frame[27] = (uint8_t)(PEER_IP >> 16);
    frame[28] = (uint8_t)(PEER_IP >> 8);
    frame[29] = (uint8_t)PEER_IP;
    frame[30] = (uint8_t)(MY_IP >> 24);
    frame[31] = (uint8_t)(MY_IP >> 16);
    frame[32] = (uint8_t)(MY_IP >> 8);
    frame[33] = (uint8_t)MY_IP;
    frame[34] = 8; /* ICMP type echo request */
    frame[35] = 0; /* code */

    uint8_t tx[128];
    size_t tx_len = 0;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_process_frame(&eth, frame, sizeof(frame), tx, &tx_len));

    /* ICMP engine must have counted the echo request */
    TEST_ASSERT_EQUAL_UINT32(1, g_test_icmp.echo_requests_rx);

    g_icmp_injected = false;
}

/* TCP injection hook for dispatcher routing test */
static SYN_TCP g_test_tcp;
static bool g_tcp_injected = false;

struct SYN_TCP *syn_eth_get_tcp_instance(void)
{
    return (struct SYN_TCP *)(g_tcp_injected ? &g_test_tcp : NULL);
}

/** Regression: ETH dispatcher must forward TCP (proto 6) to the TCP engine. */
void test_eth_dispatch_tcp(void)
{
    SYN_ETH eth;
    uint8_t mac[6] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x55};
    uint32_t ip = 0xA9FE0164;
    syn_eth_init(&eth, mac, ip);

    syn_tcp_init(&g_test_tcp, &eth);
    syn_tcp_listen(&g_test_tcp, 80);
    g_tcp_injected = true;

    /* Minimal TCP SYN frame */
    uint8_t frame[54];
    memset(frame, 0, sizeof(frame));
    memcpy(&frame[0], mac, 6);
    memcpy(&frame[6], PEER_MAC, 6);
    frame[12] = 0x08;
    frame[13] = 0x00;
    frame[14] = 0x45;
    frame[16] = 0x00;
    frame[17] = 40;
    frame[23] = 6; /* TCP */
    frame[26] = 0xC0;
    frame[27] = 0xA8;
    frame[28] = 0x01;
    frame[29] = 0x01;
    frame[30] = (uint8_t)(ip >> 24);
    frame[31] = (uint8_t)(ip >> 16);
    frame[32] = (uint8_t)(ip >> 8);
    frame[33] = (uint8_t)ip;
    frame[34] = 0xC0;
    frame[35] = 0x00; /* src port */
    frame[36] = 0x00;
    frame[37] = 80; /* dst port 80 */
    frame[38] = 0;
    frame[39] = 0;
    frame[40] = 0;
    frame[41] = 1;    /* seq */
    frame[46] = 0x50; /* data offset 5 */
    frame[47] = SYN_TCP_FLAG_SYN;

    uint8_t tx[128];
    size_t tx_len = 0;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_process_frame(&eth, frame, sizeof(frame), tx, &tx_len));

    /* TCP engine must have moved to SYN_RCVD and emitted SYN+ACK */
    TEST_ASSERT_EQUAL_INT(SYN_TCP_SYN_RCVD, g_test_tcp.conns[0].state);
    TEST_ASSERT_GREATER_THAN(0, tx_len);

    g_tcp_injected = false;
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

void test_eth_arp_reply_and_oversized_payload(void)
{
    SYN_ETH eth;
    syn_eth_init(&eth, MY_MAC, MY_IP);

    /* Construct ARP Reply frame */
    uint8_t arp_reply_frame[60] = {0};
    memcpy(&arp_reply_frame[0], MY_MAC, 6);
    memcpy(&arp_reply_frame[6], PEER_MAC, 6);
    arp_reply_frame[12] = 0x08;
    arp_reply_frame[13] = 0x06; /* EtherType = ARP */
    arp_reply_frame[14] = 0x00;
    arp_reply_frame[15] = 0x01; /* HW = Ethernet */
    arp_reply_frame[16] = 0x08;
    arp_reply_frame[17] = 0x00; /* Proto = IPv4 */
    arp_reply_frame[20] = 0x00;
    arp_reply_frame[21] = 0x02; /* Oper = Reply */

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_process_frame(&eth, arp_reply_frame, 60, NULL, NULL));
    TEST_ASSERT_EQUAL_UINT32(1, eth.arp_replies);

    /* Oversized payload build frame error */
    uint8_t payload[1600];
    uint8_t frame_out[1600];
    size_t frame_len = 0;
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM,
                          syn_eth_build_frame(&eth, PEER_MAC, SYN_ETHTYPE_IPV4, payload,
                                              sizeof(payload), frame_out, &frame_len));
}

static void test_eth_weak_hooks_and_udp_dispatch(void)
{
    SYN_ETH eth;
    SYN_UDP udp;
    syn_eth_init(&eth, MY_MAC, MY_IP);
    syn_udp_init(&udp, &eth);
    syn_transport_udp_set_instance(&udp);

    /* Construct IPv4 UDP frame (proto=17) */
    uint8_t udp_frame[60] = {0};
    memcpy(&udp_frame[0], MY_MAC, 6);
    memcpy(&udp_frame[6], PEER_MAC, 6);
    udp_frame[12] = 0x08;
    udp_frame[13] = 0x00; /* IPv4 */
    udp_frame[14] = 0x45;
    udp_frame[23] = 17; /* UDP */

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_process_frame(&eth, udp_frame, 60, NULL, NULL));
    syn_transport_udp_set_instance(NULL);

    /* Construct ARP Reply frame (oper=2) */
    uint8_t arp_reply[42] = {0};
    memcpy(&arp_reply[0], MY_MAC, 6);
    memcpy(&arp_reply[6], PEER_MAC, 6);
    arp_reply[12] = 0x08;
    arp_reply[13] = 0x06; /* ARP */
    arp_reply[20] = 0x00;
    arp_reply[21] = 0x02; /* Reply */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_eth_process_frame(&eth, arp_reply, 42, NULL, NULL));
    TEST_ASSERT_EQUAL_UINT32(1, eth.arp_replies);

    /* Test ARP update existing entry */
    uint8_t mac_new[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x99};
    TEST_ASSERT_EQUAL(SYN_OK, syn_eth_arp_update(&eth, 0x0A000002, PEER_MAC));
    TEST_ASSERT_EQUAL(SYN_OK, syn_eth_arp_update(&eth, 0x0A000002, mac_new));
}

static void test_eth_generate_mac_null_checks(void)
{
    uint8_t mac[6];
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_eth_generate_mac(NULL, 4, mac));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_eth_generate_mac("uid", 0, mac));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_eth_generate_mac("uid", 3, NULL));
}

static void test_eth_weak_instance_getters(void)
{
    TEST_ASSERT_NULL(syn_eth_get_icmp_instance());
    TEST_ASSERT_NULL(syn_eth_get_tcp_instance());
}

#include "syntropic/util/syn_pack.h"

static void test_eth_runt_arp_frame(void)
{
    SYN_ETH eth;
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    syn_eth_init(&eth, mac, 0x0A000001);
    uint8_t runt[20] = {0};
    syn_poke_u16(SYN_ETHTYPE_ARP, runt, 12);
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM,
                      syn_eth_process_frame(&eth, runt, sizeof(runt), NULL, NULL));
}

void run_eth_tests(void)
{
    RUN_TEST(test_eth_generate_mac);
    RUN_TEST(test_eth_init);
    RUN_TEST(test_eth_arp_cache);
    RUN_TEST(test_eth_arp_lookup_returns_not_found);
    RUN_TEST(test_eth_build_frame);
    RUN_TEST(test_eth_process_arp_request);
    RUN_TEST(test_eth_coroutine_pt);
    RUN_TEST(test_eth_null_checks);
    RUN_TEST(test_eth_runt_and_oversized_frames);
    RUN_TEST(test_eth_mac_filtering);
    RUN_TEST(test_eth_arp_cache_lru_eviction);
    RUN_TEST(test_eth_arp_cache_eviction_overflow);
    RUN_TEST(test_eth_dispatch_icmp);
    RUN_TEST(test_eth_dispatch_tcp);
    RUN_TEST(test_eth_multiprotocol_interleaving);
    RUN_TEST(test_eth_arp_reply_and_oversized_payload);
    RUN_TEST(test_eth_weak_hooks_and_udp_dispatch);
    RUN_TEST(test_eth_generate_mac_null_checks);
    RUN_TEST(test_eth_weak_instance_getters);
    RUN_TEST(test_eth_runt_arp_frame);
}
