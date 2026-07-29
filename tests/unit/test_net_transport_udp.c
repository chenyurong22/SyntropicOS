/**
 * @file test_net_transport_udp.c
 * @brief Unit tests for syn_transport_udp bridge interface.
 */

#include "mocks/mock_port.h"
#include "syntropic/net/syn_eth.h"
#include "syntropic/net/syn_transport_udp.h"
#include "syntropic/net/syn_udp.h"
#include "syntropic/util/syn_pack.h"
#include "unity/unity.h"

#include <string.h>

static SYN_ETH s_eth;
static SYN_UDP s_udp;
static uint8_t s_mac[6] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x55};
static uint32_t s_ip = 0x0A000001; /* 10.0.0.1 */

static void init_transport_udp_fixture(void)
{
    syn_eth_init(&s_eth, s_mac, s_ip);
    syn_udp_init(&s_udp, &s_eth);
    syn_transport_udp_set_instance(&s_udp);
}

void test_net_transport_udp_instance(void)
{
    syn_transport_udp_set_instance(NULL);
    TEST_ASSERT_NULL(syn_transport_udp_get_instance());

    init_transport_udp_fixture();
    TEST_ASSERT_EQUAL_PTR(&s_udp, syn_transport_udp_get_instance());

    syn_transport_udp_set_instance(NULL);
    TEST_ASSERT_NULL(syn_transport_udp_get_instance());
}

void run_net_transport_udp_tests(void)
{
    RUN_TEST(test_net_transport_udp_instance);
}
