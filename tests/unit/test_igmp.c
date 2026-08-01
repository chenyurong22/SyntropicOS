/**
 * @file test_igmp.c
 * @brief Unit tests for Zero-Heap Native IGMPv2 Protocol Engine.
 */

#include "syntropic/net/syn_igmp.h"
#include "unity/unity.h"

#include <string.h>

static const uint8_t MAC[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
static const uint32_t MY_IP = 0xC0A80164;      /* 192.168.1.100 */
static const uint32_t MDNS_GROUP = 0xE00000FB; /* 224.0.0.251 */

void test_igmp_init(void)
{
    SYN_IGMP igmp;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_igmp_init(&igmp));
    TEST_ASSERT_EQUAL_UINT32(0, igmp.reports_sent);
    TEST_ASSERT_EQUAL_UINT32(0, igmp.queries_received);
}

void test_igmp_join_and_leave(void)
{
    SYN_IGMP igmp;
    SYN_ETH eth;
    syn_igmp_init(&igmp);
    syn_eth_init(&eth, MAC, MY_IP);

    uint8_t join_frame[60];
    size_t join_len = 0;

    /* Join mDNS multicast group 224.0.0.251 */
    TEST_ASSERT_EQUAL_INT(SYN_OK,
                          syn_igmp_join_group(&igmp, &eth, MDNS_GROUP, join_frame, &join_len));
    TEST_ASSERT_EQUAL_INT(60, join_len);
    TEST_ASSERT_EQUAL_UINT32(1, igmp.reports_sent);
    TEST_ASSERT_TRUE(igmp.groups[0].joined);
    TEST_ASSERT_EQUAL_UINT32(MDNS_GROUP, igmp.groups[0].group_ip);

    /* Verify Destination Multicast MAC: 01:00:5E:00:00:FB */
    TEST_ASSERT_EQUAL_UINT8(0x01, join_frame[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, join_frame[1]);
    TEST_ASSERT_EQUAL_UINT8(0x5E, join_frame[2]);
    TEST_ASSERT_EQUAL_UINT8(0x00, join_frame[3]);
    TEST_ASSERT_EQUAL_UINT8(0x00, join_frame[4]);
    TEST_ASSERT_EQUAL_UINT8(0xFB, join_frame[5]);

    /* Verify IGMP Type == 0x16 (v2 Report) */
    TEST_ASSERT_EQUAL_UINT8(SYN_IGMP_TYPE_V2_REPORT, join_frame[34]);

    /* Leave Group */
    uint8_t leave_frame[60];
    size_t leave_len = 0;
    TEST_ASSERT_EQUAL_INT(SYN_OK,
                          syn_igmp_leave_group(&igmp, &eth, MDNS_GROUP, leave_frame, &leave_len));
    TEST_ASSERT_EQUAL_INT(60, leave_len);
    TEST_ASSERT_EQUAL_UINT32(1, igmp.leaves_sent);
    TEST_ASSERT_FALSE(igmp.groups[0].joined);
    TEST_ASSERT_EQUAL_UINT8(SYN_IGMP_TYPE_V2_LEAVE, leave_frame[34]);
}

void test_igmp_process_query(void)
{
    SYN_IGMP igmp;
    SYN_ETH eth;
    syn_igmp_init(&igmp);
    syn_eth_init(&eth, MAC, MY_IP);

    uint8_t dummy_frame[60];
    size_t dummy_len = 0;
    syn_igmp_join_group(&igmp, &eth, MDNS_GROUP, dummy_frame, &dummy_len);

    /* Construct incoming IGMP Membership Query (Type 0x11) frame */
    uint8_t query_pkt[60] = {0};
    query_pkt[12] = 0x08;
    query_pkt[13] = 0x00; /* IPv4 */
    query_pkt[23] = 2;    /* Protocol = IGMP */
    query_pkt[34] = SYN_IGMP_TYPE_MEMBERSHIP_QUERY;

    uint8_t reply_frame[60];
    size_t reply_len = 0;
    TEST_ASSERT_EQUAL_INT(
        SYN_OK, syn_igmp_process_packet(&igmp, &eth, query_pkt, 60, reply_frame, &reply_len));
    TEST_ASSERT_EQUAL_UINT32(1, igmp.queries_received);
    TEST_ASSERT_EQUAL_INT(60, reply_len);
    TEST_ASSERT_EQUAL_UINT8(SYN_IGMP_TYPE_V2_REPORT, reply_frame[34]);
}

void test_igmp_null_checks(void)
{
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_igmp_init(NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_igmp_join_group(NULL, NULL, 0, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_igmp_leave_group(NULL, NULL, 0, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM,
                          syn_igmp_process_packet(NULL, NULL, NULL, 0, NULL, NULL));
}

void test_igmp_group_overflow_and_leaving_unjoined(void)
{
    SYN_IGMP igmp;
    SYN_ETH eth;
    syn_igmp_init(&igmp);
    syn_eth_init(&eth, MAC, MY_IP);

    uint8_t frame[60];
    size_t len = 0;

    /* Fill all 4 group slots */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_igmp_join_group(&igmp, &eth, 0xE0000001, frame, &len));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_igmp_join_group(&igmp, &eth, 0xE0000002, frame, &len));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_igmp_join_group(&igmp, &eth, 0xE0000003, frame, &len));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_igmp_join_group(&igmp, &eth, 0xE0000004, frame, &len));

    /* Join 5th group (causes replacement of slot 0) */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_igmp_join_group(&igmp, &eth, 0xE0000005, frame, &len));
    TEST_ASSERT_EQUAL_UINT32(0xE0000005, igmp.groups[0].group_ip);

    /* Leave a group that was never joined */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_igmp_leave_group(&igmp, &eth, 0xE0000099, frame, &len));
}

void test_igmp_non_igmp_packets(void)
{
    SYN_IGMP igmp;
    SYN_ETH eth;
    syn_igmp_init(&igmp);
    syn_eth_init(&eth, MAC, MY_IP);

    uint8_t short_pkt[30] = {0};
    size_t tx_len = 0;
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM,
                          syn_igmp_process_packet(&igmp, &eth, short_pkt, 30, NULL, &tx_len));

    /* ARP packet (ethertype != 0x0800) */
    uint8_t arp_pkt[60] = {0};
    arp_pkt[12] = 0x08;
    arp_pkt[13] = 0x06;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_igmp_process_packet(&igmp, &eth, arp_pkt, 60, NULL, &tx_len));

    /* UDP packet (proto = 17 != 2) */
    uint8_t udp_pkt[60] = {0};
    udp_pkt[12] = 0x08;
    udp_pkt[13] = 0x00;
    udp_pkt[23] = 17;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_igmp_process_packet(&igmp, &eth, udp_pkt, 60, NULL, &tx_len));

    /* Join group with group_ip = 0 -> SYN_INVALID_PARAM */
    uint8_t frame[60];
    size_t len = 0;
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_igmp_join_group(&igmp, &eth, 0, frame, &len));

    /* Re-join an existing group */
    SYN_IGMP rejoin_igmp;
    syn_igmp_init(&rejoin_igmp);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_igmp_join_group(&rejoin_igmp, &eth, MDNS_GROUP, frame, &len));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_igmp_join_group(&rejoin_igmp, &eth, MDNS_GROUP, frame, &len));

    /* Process IGMP query with NULL frame_tx / tx_len */
    uint8_t query_pkt_null[60] = {0};
    query_pkt_null[12] = 0x08;
    query_pkt_null[13] = 0x00;
    query_pkt_null[23] = 2;
    query_pkt_null[34] = SYN_IGMP_TYPE_MEMBERSHIP_QUERY;
    TEST_ASSERT_EQUAL_INT(
        SYN_OK, syn_igmp_process_packet(&rejoin_igmp, &eth, query_pkt_null, 60, NULL, NULL));

    /* Process IGMP V2 Report type packet (non-query) */
    uint8_t report_pkt[60] = {0};
    report_pkt[12] = 0x08;
    report_pkt[13] = 0x00;
    report_pkt[23] = 2;
    report_pkt[34] = SYN_IGMP_TYPE_V2_REPORT;
    TEST_ASSERT_EQUAL_INT(SYN_OK,
                          syn_igmp_process_packet(&rejoin_igmp, &eth, report_pkt, 60, NULL, NULL));
}
