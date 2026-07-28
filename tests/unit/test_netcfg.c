/**
 * @file test_netcfg.c
 * @brief Unit tests for Unified Zero-Heap Network IP Address Manager.
 */

#include "syntropic/net/syn_netcfg.h"
#include "unity/unity.h"

#include <string.h>

static const uint8_t MAC[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};

void test_netcfg_init_static(void)
{
    SYN_NETCFG netcfg;
    SYN_ETH eth;
    syn_eth_init(&eth, MAC, 0);

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_netcfg_init(&netcfg, SYN_NETCFG_MODE_STATIC, MAC));
    TEST_ASSERT_TRUE(netcfg.is_bound);

    TEST_ASSERT_EQUAL_INT(SYN_OK,
                          syn_netcfg_set_static(&netcfg, &eth, 0xC0A80164, 0xFFFFFF00, 0xC0A80101));
    TEST_ASSERT_EQUAL_UINT32(0xC0A80164, eth.ip_addr);
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFF00, eth.netmask);
    TEST_ASSERT_EQUAL_UINT32(0xC0A80101, eth.gateway);
}

void test_netcfg_autoip_fallback(void)
{
    SYN_NETCFG netcfg;
    SYN_ETH eth;
    syn_eth_init(&eth, MAC, 0);

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_netcfg_init(&netcfg, SYN_NETCFG_MODE_AUTO, MAC));
    TEST_ASSERT_FALSE(netcfg.is_bound);

    /* Trigger AutoIP fallback upon DHCP timeout */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_netcfg_trigger_autoip_fallback(&netcfg, &eth, MAC));
    TEST_ASSERT_TRUE(netcfg.is_bound);

    /* IP address should automatically fall inside 169.254.x.x */
    TEST_ASSERT_EQUAL_UINT32(0xA9FE0000UL, eth.ip_addr & 0xFFFF0000UL);
    TEST_ASSERT_EQUAL_UINT32(0xFFFF0000UL, eth.netmask);
}

static SYN_PT_Status netcfg_coroutine_task(SYN_PT *pt, SYN_NETCFG *netcfg)
{
    PT_BEGIN(pt);
    PT_NETCFG_WAIT_BOUND(pt, netcfg);
    PT_END(pt);
}

void test_netcfg_coroutine_pt(void)
{
    SYN_NETCFG netcfg;
    syn_netcfg_init(&netcfg, SYN_NETCFG_MODE_AUTO, MAC);

    SYN_PT pt;
    PT_INIT(&pt);

    /* First step: Netcfg is_bound == false -> coroutine yields (PT_WAITING) */
    TEST_ASSERT_EQUAL_INT(PT_WAITING, netcfg_coroutine_task(&pt, &netcfg));

    netcfg.is_bound = true;

    /* Second step: Netcfg is_bound == true -> coroutine completes (PT_EXITED) */
    TEST_ASSERT_EQUAL_INT(PT_EXITED, netcfg_coroutine_task(&pt, &netcfg));
}

void test_netcfg_null_checks(void)
{
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_netcfg_init(NULL, 0, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_netcfg_set_static(NULL, NULL, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_netcfg_trigger_autoip_fallback(NULL, NULL, NULL));
}
