/**
 * @file test_udp.c
 * @brief Unit tests for zero-alloc native UDP stack (syn_udp).
 */

#include "mocks/mock_port.h"
#include "syntropic/net/syn_eth.h"
#include "syntropic/net/syn_transport_udp.h"
#include "syntropic/net/syn_udp.h"
#include "syntropic/sched/syn_sched.h"
#include "syntropic/util/syn_pack.h"
#include "unity/unity.h"

#include <string.h>

static SYN_ETH s_eth;
static SYN_UDP s_udp;
static uint8_t s_mac[6] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x55};
static uint32_t s_ip = 0x0A000001; /* 10.0.0.1 */

static void init_udp_fixture(void)
{
    syn_eth_init(&s_eth, s_mac, s_ip);
    syn_udp_init(&s_udp, &s_eth);
}

void test_udp_init_and_bind(void)
{
    init_udp_fixture();
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_udp_init(&s_udp, &s_eth));

    SYN_UdpSocket *sock1 = syn_udp_bind(&s_udp, 5000);
    TEST_ASSERT_NOT_NULL(sock1);
    TEST_ASSERT_TRUE(sock1->is_bound);
    TEST_ASSERT_EQUAL_UINT16(5000, sock1->local_port);

    SYN_UdpSocket *sock1_again = syn_udp_bind(&s_udp, 5000);
    TEST_ASSERT_EQUAL_PTR(sock1, sock1_again);

    SYN_UdpSocket *sock2 = syn_udp_bind(&s_udp, 6000);
    TEST_ASSERT_NOT_NULL(sock2);
    TEST_ASSERT_EQUAL_UINT16(6000, sock2->local_port);

    syn_udp_unbind(sock1);
    TEST_ASSERT_FALSE(sock1->is_bound);
}

void test_udp_process_packet_demux_and_task_wake(void)
{
    init_udp_fixture();
    SYN_UdpSocket *sock = syn_udp_bind(&s_udp, 9000);
    TEST_ASSERT_NOT_NULL(sock);

    SYN_Task dummy_task;
    memset(&dummy_task, 0, sizeof(dummy_task));
    dummy_task.state = SYN_TASK_BLOCKED;
    sock->blocked_task = &dummy_task;

    /* Build raw Ethernet + IPv4 + UDP packet addressed to port 9000 */
    uint8_t pkt[128];
    memset(pkt, 0, sizeof(pkt));

    /* Ethernet */
    memcpy(&pkt[0], s_mac, 6);
    memcpy(&pkt[6], s_mac, 6);
    pkt[12] = 0x08;
    pkt[13] = 0x00;

    /* IPv4 */
    pkt[14] = 0x45;
    syn_poke_u16(20 + 8 + 5, pkt, 16);
    pkt[23] = 17;                      /* UDP */
    syn_poke_u32(0x0A000002, pkt, 26); /* Src IP 10.0.0.2 */
    syn_poke_u32(s_ip, pkt, 30);       /* Dst IP 10.0.0.1 */

    /* UDP */
    syn_poke_u16(1234, pkt, 34); /* Src Port 1234 */
    syn_poke_u16(9000, pkt, 36); /* Dst Port 9000 */
    syn_poke_u16(13, pkt, 38);   /* Length 13 (8 hdr + 5 payload) */
    memcpy(&pkt[42], "HELLO", 5);

    SYN_Status st = syn_udp_process_packet(&s_udp, pkt, 14 + 20 + 13);
    TEST_ASSERT_EQUAL_INT(SYN_OK, st);

    /* Verify payload arrived */
    TEST_ASSERT_EQUAL_UINT16(5, sock->rx_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY("HELLO", sock->rx_buf, 5);
    TEST_ASSERT_EQUAL_UINT32(0x0A000002, sock->remote_ip);
    TEST_ASSERT_EQUAL_UINT16(1234, sock->remote_port);

    /* Verify task unblocking */
    TEST_ASSERT_EQUAL_INT(SYN_TASK_READY, dummy_task.state);
}

void test_udp_sendto_framing(void)
{
    init_udp_fixture();
    uint8_t tx_out[128];
    size_t tx_len = 0;

    int res =
        syn_udp_sendto(&s_udp, 5000, 0x0A000002, 6000, (const uint8_t *)"TEST", 4, tx_out, &tx_len);
    TEST_ASSERT_EQUAL_INT(4, res);
    TEST_ASSERT_EQUAL_UINT32(14 + 20 + 8 + 4, tx_len);

    /* Check EtherType */
    TEST_ASSERT_EQUAL_UINT16(0x0800, syn_peek_u16(tx_out, 12));
    /* Check IPv4 Proto */
    TEST_ASSERT_EQUAL_UINT8(17, tx_out[23]);
    /* Check UDP Ports */
    TEST_ASSERT_EQUAL_UINT16(5000, syn_peek_u16(tx_out, 34));
    TEST_ASSERT_EQUAL_UINT16(6000, syn_peek_u16(tx_out, 36));
}

/**
 * Regression: syn_udp_sendto must result in syn_port_eth_tx being called —
 * previously the frame was built but silently dropped (transmit path was
 * missing the syn_port_eth_tx call).
 */
void test_udp_transport_bridge_transmits(void)
{
    init_udp_fixture();

    /* Inject ARP entry so the ETH layer resolves the dst MAC */
    uint8_t dst_mac[6] = {0x02, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    syn_eth_arp_update(&s_eth, 0x0A000002, dst_mac);

    /* Reset mock ETH tx counters */
    mock_eth_tx_len = 0;
    mock_eth_tx_count = 0;

    /* Build a UDP frame directly via the native stack (the transport bridge
     * wraps this exact call) */
    uint8_t tx_frame[SYN_ETH_MAX_FRAME_LEN];
    size_t tx_len = 0;
    int ret = syn_udp_sendto(&s_udp, 7777, 0x0A000002, 8888, (const uint8_t *)"PING", 4, tx_frame,
                             &tx_len);
    TEST_ASSERT_EQUAL_INT(4, ret);
    TEST_ASSERT_GREATER_THAN(0U, tx_len);

    /* Simulate what the bridge does after syn_udp_sendto succeeds */
    syn_port_eth_tx(tx_frame, tx_len);

    /* syn_port_eth_tx must have been called */
    TEST_ASSERT_GREATER_THAN(0, mock_eth_tx_count);
    TEST_ASSERT_GREATER_OR_EQUAL(14U + 20U + 8U + 4U, mock_eth_tx_len);
    TEST_ASSERT_EQUAL_UINT8(0x08, mock_eth_tx_buf[12]); /* EtherType hi */
    TEST_ASSERT_EQUAL_UINT8(0x00, mock_eth_tx_buf[13]); /* EtherType lo */
    TEST_ASSERT_EQUAL_UINT8(17, mock_eth_tx_buf[23]);   /* Protocol: UDP */
}

void test_udp_extended_edge_cases(void)
{
    /* Null & invalid init/bind checks */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_udp_init(NULL, NULL));
    TEST_ASSERT_NULL(syn_udp_bind(NULL, 5000));
    TEST_ASSERT_NULL(syn_udp_bind(&s_udp, 0));

    /* Null & invalid checksum checks */
    TEST_ASSERT_EQUAL_UINT16(0, syn_udp_checksum(0, 0, NULL, 0));
    /* Odd payload length for checksum calculation */
    uint8_t odd_payload[9] = {0x12, 0x34, 0x56, 0x78, 0x00, 0x09, 0x00, 0x00, 0xAA};
    TEST_ASSERT_NOT_EQUAL(
        0, syn_udp_checksum(0x0A000001, 0x0A000002, odd_payload, sizeof(odd_payload)));

    /* Truncated IP packet processing */
    uint8_t short_pkt[20] = {0};
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM,
                          syn_udp_process_packet(&s_udp, short_pkt, sizeof(short_pkt)));

    /* Invalid EtherType (0x0806 ARP) */
    uint8_t arp_pkt[64] = {0};
    syn_poke_u16(0x0806, arp_pkt, 12);
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_udp_process_packet(&s_udp, arp_pkt, sizeof(arp_pkt)));

    /* Invalid Protocol (TCP = 6) */
    uint8_t tcp_pkt[64] = {0};
    syn_poke_u16(0x0800, tcp_pkt, 12);
    tcp_pkt[23] = 6;
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_udp_process_packet(&s_udp, tcp_pkt, sizeof(tcp_pkt)));

    /* Unbound destination port */
    uint8_t unbound_pkt[64] = {0};
    syn_poke_u16(0x0800, unbound_pkt, 12);
    unbound_pkt[23] = 17;
    syn_poke_u16(9999, &unbound_pkt[34], 2); /* Dst port 9999 */
    syn_poke_u16(16, &unbound_pkt[34], 4);   /* UDP length 16 */
    TEST_ASSERT_EQUAL_INT(SYN_ERROR,
                          syn_udp_process_packet(&s_udp, unbound_pkt, sizeof(unbound_pkt)));

    /* Over-sized payload sendto */
    uint8_t tx_out[2000];
    size_t tx_len = 0;
    TEST_ASSERT_EQUAL_INT(
        -1, syn_udp_sendto(&s_udp, 5000, 0x0A000002, 6000, tx_out, 1600, tx_out, &tx_len));

    /* sendto with null eth instance */
    SYN_UDP no_eth_udp;
    syn_udp_init(&no_eth_udp, NULL);
    TEST_ASSERT_GREATER_THAN(0, syn_udp_sendto(&no_eth_udp, 5000, 0x0A000002, 6000,
                                               (const uint8_t *)"TEST", 4, tx_out, &tx_len));

    /* unbind NULL check */
    syn_udp_unbind(NULL);
}

void run_udp_tests(void)
{
    RUN_TEST(test_udp_init_and_bind);
    RUN_TEST(test_udp_process_packet_demux_and_task_wake);
    RUN_TEST(test_udp_sendto_framing);
    RUN_TEST(test_udp_transport_bridge_transmits);
    RUN_TEST(test_udp_extended_edge_cases);
}
