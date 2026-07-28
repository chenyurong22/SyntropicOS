/**
 * @file syn_netcfg.h
 * @brief Unified Zero-Heap Network IP Address Manager.
 *
 * Unifies Static IP, DHCP Client, and RFC 3927 AutoIP fallback.
 */

#ifndef SYN_NETCFG_H
#define SYN_NETCFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "syntropic/common/syn_defs.h"
#include "syntropic/net/syn_autoip.h"
#include "syntropic/net/syn_dhcp.h"
#include "syntropic/net/syn_eth.h"
#include "syntropic/pt/syn_pt.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** IP Manager Modes. */
typedef enum {
    SYN_NETCFG_MODE_STATIC = 0,
    SYN_NETCFG_MODE_DHCP,
    SYN_NETCFG_MODE_AUTO /* DHCP with RFC 3927 AutoIP fallback */
} SYN_NETCFG_Mode;

/** Network IP Manager Context. */
typedef struct {
    SYN_NETCFG_Mode mode;
    SYN_DHCP dhcp;
    SYN_AUTOIP autoip;
    bool is_bound;
    uint32_t assigned_ip;
    uint32_t assigned_netmask;
    uint32_t assigned_gateway;
} SYN_NETCFG;

/**
 * @brief Initialize Unified Network IP Manager.
 *
 * @param netcfg Pointer to Netcfg context.
 * @param mode   Configuration mode (STATIC, DHCP, or AUTO).
 * @param mac    6-byte client MAC address.
 * @return SYN_OK on success.
 */
SYN_Status syn_netcfg_init(SYN_NETCFG *netcfg, SYN_NETCFG_Mode mode, const uint8_t mac[6]);

/**
 * @brief Set Static IP configuration parameters.
 *
 * @param netcfg  Pointer to Netcfg context.
 * @param eth     Pointer to Ethernet context.
 * @param ip      32-bit IPv4 address.
 * @param netmask 32-bit netmask.
 * @param gateway 32-bit router gateway IP.
 * @return SYN_OK on success.
 */
SYN_Status syn_netcfg_set_static(SYN_NETCFG *netcfg, SYN_ETH *eth, uint32_t ip, uint32_t netmask,
                                 uint32_t gateway);

/**
 * @brief Trigger AutoIP fallback when DHCP times out.
 *
 * @param netcfg Pointer to Netcfg context.
 * @param eth    Pointer to Ethernet context.
 * @param mac    6-byte client MAC address.
 * @return SYN_OK on success.
 */
SYN_Status syn_netcfg_trigger_autoip_fallback(SYN_NETCFG *netcfg, SYN_ETH *eth,
                                              const uint8_t mac[6]);

/* ── Protothread Coroutine Integration ──────────────────────────────────── */

/**
 * @brief Block a protothread coroutine until IP configuration binding completes.
 *
 * @param pt     Protothread context.
 * @param netcfg Pointer to Netcfg context.
 */
#define PT_NETCFG_WAIT_BOUND(pt, netcfg) PT_WAIT_UNTIL(pt, (netcfg)->is_bound == true)

#ifdef __cplusplus
}
#endif

#endif /* SYN_NETCFG_H */
