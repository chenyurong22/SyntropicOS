/**
 * @file test_autoip.c
 * @brief Unit tests for Zero-Heap Native RFC 3927 AutoIP Engine.
 */

#include "syntropic/net/syn_autoip.h"
#include "unity/unity.h"

#include <string.h>

static const uint8_t MAC[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};

void test_autoip_init(void)
{
    SYN_AUTOIP autoip;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_autoip_init(&autoip, MAC));
    TEST_ASSERT_EQUAL_INT(SYN_AUTOIP_STATE_INIT, autoip.state);

    /* Candidate IP should fall inside 169.254.1.1 .. 169.254.254.255 */
    TEST_ASSERT_EQUAL_UINT32(0xA9FE0000UL, autoip.ip_addr & 0xFFFF0000UL);
}

void test_autoip_probe_and_announce(void)
{
    SYN_AUTOIP autoip;
    syn_autoip_init(&autoip, MAC);

    uint8_t probe_frame[60];
    size_t probe_len = 0;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_autoip_build_probe(&autoip, MAC, probe_frame, &probe_len));
    TEST_ASSERT_EQUAL_INT(60, probe_len);
    TEST_ASSERT_EQUAL_INT(SYN_AUTOIP_STATE_PROBE, autoip.state);
    TEST_ASSERT_EQUAL_UINT8(1, autoip.probe_count);

    uint8_t announce_frame[60];
    size_t announce_len = 0;
    TEST_ASSERT_EQUAL_INT(SYN_OK,
                          syn_autoip_build_announce(&autoip, MAC, announce_frame, &announce_len));
    TEST_ASSERT_EQUAL_INT(60, announce_len);
    TEST_ASSERT_EQUAL_INT(SYN_AUTOIP_STATE_ANNOUNCE, autoip.state);
    TEST_ASSERT_EQUAL_UINT8(1, autoip.announce_count);
}

void test_autoip_process_arp_binding(void)
{
    SYN_AUTOIP autoip;
    SYN_ETH eth;
    syn_autoip_init(&autoip, MAC);
    syn_eth_init(&eth, MAC, 0);

    /* ARP frame from different sender IP (no conflict) */
    uint8_t arp_frame[60] = {0};
    arp_frame[12] = 0x08;
    arp_frame[13] = 0x06;
    arp_frame[28] = 10;
    arp_frame[29] = 0;
    arp_frame[30] = 0;
    arp_frame[31] = 1;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_autoip_process_arp(&autoip, &eth, arp_frame, 60));
    TEST_ASSERT_EQUAL_INT(SYN_AUTOIP_STATE_BOUND, autoip.state);
    TEST_ASSERT_EQUAL_UINT32(autoip.ip_addr, eth.ip_addr);
    TEST_ASSERT_EQUAL_UINT32(SYN_AUTOIP_NETMASK, eth.netmask);
}

static SYN_PT_Status autoip_coroutine_task(SYN_PT *pt, SYN_AUTOIP *autoip)
{
    PT_BEGIN(pt);
    PT_AUTOIP_WAIT_BOUND(pt, autoip);
    PT_END(pt);
}

void test_autoip_coroutine_pt(void)
{
    SYN_AUTOIP autoip;
    syn_autoip_init(&autoip, MAC);

    SYN_PT pt;
    PT_INIT(&pt);

    /* First step: AutoIP state != BOUND -> coroutine yields (PT_WAITING) */
    TEST_ASSERT_EQUAL_INT(PT_WAITING, autoip_coroutine_task(&pt, &autoip));

    autoip.state = SYN_AUTOIP_STATE_BOUND;

    /* Second step: AutoIP state == BOUND -> coroutine completes (PT_EXITED) */
    TEST_ASSERT_EQUAL_INT(PT_EXITED, autoip_coroutine_task(&pt, &autoip));
}

void test_autoip_null_checks(void)
{
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_autoip_init(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_autoip_build_probe(NULL, NULL, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_autoip_build_announce(NULL, NULL, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_autoip_process_arp(NULL, NULL, NULL, 0));

    /* Non-ARP frame -> return SYN_OK */
    SYN_AUTOIP autoip;
    syn_autoip_init(&autoip, MAC);
    uint8_t ip_frame[60] = {0};
    ip_frame[12] = 0x08;
    ip_frame[13] = 0x00; /* IPv4 */
    TEST_ASSERT_EQUAL(SYN_OK, syn_autoip_process_arp(&autoip, NULL, ip_frame, 60));

    /* ARP Conflict: sender IP matches candidate autoip->ip_addr */
    uint8_t conflict_arp[60] = {0};
    conflict_arp[12] = 0x08;
    conflict_arp[13] = 0x06;
    conflict_arp[28] = (uint8_t)(autoip.ip_addr >> 24);
    conflict_arp[29] = (uint8_t)(autoip.ip_addr >> 16);
    conflict_arp[30] = (uint8_t)(autoip.ip_addr >> 8);
    conflict_arp[31] = (uint8_t)(autoip.ip_addr);

    TEST_ASSERT_EQUAL(SYN_BUSY, syn_autoip_process_arp(&autoip, NULL, conflict_arp, 60));
    TEST_ASSERT_EQUAL_UINT8(1, autoip.collisions);
    TEST_ASSERT_EQUAL_INT(SYN_AUTOIP_STATE_INIT, autoip.state);

    /* ARP Conflict with octet4 = 254 -> rollover to 1 */
    autoip.ip_addr = 0xA9FE01FE; /* 169.254.1.254 */
    conflict_arp[28] = 0xA9;
    conflict_arp[29] = 0xFE;
    conflict_arp[30] = 0x01;
    conflict_arp[31] = 0xFE;
    TEST_ASSERT_EQUAL(SYN_BUSY, syn_autoip_process_arp(&autoip, NULL, conflict_arp, 60));
    TEST_ASSERT_EQUAL_UINT32(0xA9FE0101, autoip.ip_addr);
}
