/**
 * @file syn_netcfg.c
 * @brief Unified Zero-Heap Network IP Address Manager Implementation.
 */

#include "syntropic/net/syn_netcfg.h"

#include <string.h>

SYN_Status syn_netcfg_init(SYN_NETCFG *netcfg, SYN_NETCFG_Mode mode, const uint8_t mac[6])
{
    if (!netcfg || !mac) {
        return SYN_INVALID_PARAM;
    }

    memset(netcfg, 0, sizeof(*netcfg));
    netcfg->mode = mode;

    syn_dhcp_init(&netcfg->dhcp, 0x12345678UL);
    syn_autoip_init(&netcfg->autoip, mac);

    if (mode == SYN_NETCFG_MODE_STATIC) {
        netcfg->is_bound = true;
    }

    return SYN_OK;
}

SYN_Status syn_netcfg_set_static(SYN_NETCFG *netcfg, SYN_ETH *eth, uint32_t ip, uint32_t netmask,
                                 uint32_t gateway)
{
    if (!netcfg || !eth || ip == 0) {
        return SYN_INVALID_PARAM;
    }

    netcfg->mode = SYN_NETCFG_MODE_STATIC;
    netcfg->assigned_ip = ip;
    netcfg->assigned_netmask = netmask;
    netcfg->assigned_gateway = gateway;
    netcfg->is_bound = true;

    eth->ip_addr = ip;
    eth->netmask = netmask;
    eth->gateway = gateway;

    return SYN_OK;
}

SYN_Status syn_netcfg_trigger_autoip_fallback(SYN_NETCFG *netcfg, SYN_ETH *eth,
                                              const uint8_t mac[6])
{
    if (!netcfg || !eth || !mac) {
        return SYN_INVALID_PARAM;
    }

    syn_autoip_init(&netcfg->autoip, mac);
    netcfg->autoip.state = SYN_AUTOIP_STATE_BOUND;

    netcfg->assigned_ip = netcfg->autoip.ip_addr;
    netcfg->assigned_netmask = SYN_AUTOIP_NETMASK;
    netcfg->assigned_gateway = 0;
    netcfg->is_bound = true;

    eth->ip_addr = netcfg->assigned_ip;
    eth->netmask = netcfg->assigned_netmask;
    eth->gateway = 0;

    return SYN_OK;
}
