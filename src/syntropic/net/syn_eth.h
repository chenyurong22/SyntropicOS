/**
 * @file syn_eth.h
 * @brief Zero-Heap Native Ethernet II & ARP Protocol Engine.
 *
 * Specifications:
 * - Ethernet II Header: [Dst MAC 6B] + [Src MAC 6B] + [EtherType 2B]
 * - Minimum Frame Size: 60 bytes (padded with zeros if payload < 46B)
 * - Maximum Frame Size: 1514 bytes (excluding 4B FCS)
 * - EtherTypes:
 *   - 0x0806: ARP (Address Resolution Protocol)
 *   - 0x0800: IPv4 (Internet Protocol v4)
 *   - 0x86DD: IPv6 (Internet Protocol v6)
 *   - 0x88A4: EtherCAT Industrial Ethernet
 */

#ifndef SYN_ETH_H
#define SYN_ETH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "syntropic/common/syn_defs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SYN_ETH_MAC_LEN 6U
#define SYN_ETH_HEADER_LEN 14U
#define SYN_ETH_MIN_FRAME_LEN 60U
#define SYN_ETH_MAX_FRAME_LEN 1514U
#ifndef SYN_ETH_ARP_CACHE_SIZE
#define SYN_ETH_ARP_CACHE_SIZE 8U
#endif

#define SYN_ETHTYPE_IPV4 0x0800U
#define SYN_ETHTYPE_ARP 0x0806U
#define SYN_ETHTYPE_IPV6 0x86DDU
#define SYN_ETHTYPE_ETHERCAT 0x88A4U

#define SYN_ARP_OP_REQUEST 1U
#define SYN_ARP_OP_REPLY 2U

/** Parsed Ethernet II Header. */
typedef struct {
    uint8_t dst_mac[SYN_ETH_MAC_LEN];
    uint8_t src_mac[SYN_ETH_MAC_LEN];
    uint16_t ethertype;
} SYN_ETH_Header;

/** ARP Table Cache Entry. */
typedef struct {
    uint32_t ip;
    uint8_t mac[SYN_ETH_MAC_LEN];
    uint32_t last_seen_ms;
    bool valid;
} SYN_ETH_ArpEntry;

/** Native Ethernet Interface Engine. */
typedef struct {
    uint8_t mac_addr[SYN_ETH_MAC_LEN];
    uint32_t ip_addr;
    uint32_t netmask;
    uint32_t gateway;
    SYN_ETH_ArpEntry arp_cache[SYN_ETH_ARP_CACHE_SIZE];
    uint32_t frames_tx;
    uint32_t frames_rx;
    uint32_t arp_requests;
    uint32_t arp_replies;
} SYN_ETH;

/**
 * @brief Generate a deterministic Locally Administered MAC address from MCU Unique ID.
 *
 * @param uid_bytes Pointer to hardware unique ID bytes (e.g. STM32 96-bit UID).
 * @param uid_len   Length of unique ID in bytes.
 * @param mac_out   Output buffer to receive generated 6-byte MAC address.
 * @return SYN_OK on success.
 */
SYN_Status syn_eth_generate_mac(const void *uid_bytes, size_t uid_len, uint8_t mac_out[6]);

/**
 * @brief Initialize Native Ethernet Engine.
 *
 * @param eth     Pointer to Ethernet instance.
 * @param mac_addr 6-byte hardware MAC address.
 * @param ip_addr  32-bit IPv4 address (host byte order).
 * @return SYN_OK on success.
 */
SYN_Status syn_eth_init(SYN_ETH *eth, const uint8_t mac_addr[6], uint32_t ip_addr);

/**
 * @brief Process incoming Ethernet II frame.
 *
 * @param eth      Pointer to Ethernet instance.
 * @param frame    Pointer to raw frame bytes.
 * @param len      Frame byte length (60..1514 bytes).
 * @param tx_buf   Output buffer to receive immediate ARP reply if generated.
 * @param tx_len   Pointer to receive ARP reply byte length (0 if no reply needed).
 * @return SYN_OK on success.
 */
SYN_Status syn_eth_process_frame(SYN_ETH *eth, const uint8_t *frame, size_t len, uint8_t *tx_buf,
                                 size_t *tx_len);

/**
 * @brief Build and transmit an outbound Ethernet II frame.
 *
 * @param eth       Pointer to Ethernet instance.
 * @param dst_mac   Destination MAC address (6 bytes).
 * @param ethertype EtherType value (e.g. SYN_ETHTYPE_IPV4).
 * @param payload   Pointer to payload buffer.
 * @param payload_len Payload byte length.
 * @param frame_out Output frame buffer (must hold at least 1514 bytes).
 * @param frame_len Pointer to receive final raw Ethernet frame length.
 * @return SYN_OK on success.
 */
SYN_Status syn_eth_build_frame(SYN_ETH *eth, const uint8_t dst_mac[6], uint16_t ethertype,
                               const uint8_t *payload, size_t payload_len, uint8_t *frame_out,
                               size_t *frame_len);

/**
 * @brief Lookup MAC address in local ARP cache table.
 *
 * @param eth     Pointer to Ethernet instance.
 * @param ip      32-bit IPv4 address to lookup.
 * @param mac_out Output buffer (6 bytes) to receive resolved MAC.
 * @return SYN_OK if IP found in cache, SYN_NOT_FOUND if absent.
 */
SYN_Status syn_eth_arp_lookup(SYN_ETH *eth, uint32_t ip, uint8_t mac_out[6]);

SYN_Status syn_eth_arp_update(SYN_ETH *eth, uint32_t ip, const uint8_t mac[6]);

/* ── Protocol engine injection ──────────────────────────────────────────── */

/**
 * @brief Weak hook — override to inject an ICMP engine into the Ethernet dispatcher.
 *
 * When syn_eth_process_frame receives an IPv4/ICMP packet (protocol 1),
 * it calls this to obtain the ICMP instance. Return NULL to silently drop.
 *
 * @return Pointer to SYN_ICMP instance, or NULL.
 */
struct SYN_ICMP *syn_eth_get_icmp_instance(void);

/**
 * @brief Weak hook — override to inject a TCP engine into the Ethernet dispatcher.
 *
 * When syn_eth_process_frame receives an IPv4/TCP packet (protocol 6),
 * it calls this to obtain the TCP instance. Return NULL to silently drop.
 *
 * @return Pointer to SYN_TCP instance, or NULL.
 */
struct SYN_TCP *syn_eth_get_tcp_instance(void);

/* ── Protothread Coroutine Integration ──────────────────────────────────── */

#include "port/syn_port_eth.h"
#include "syntropic/pt/syn_pt.h"

/**
 * @brief Block a protothread coroutine until a raw Ethernet frame is received from PHY.
 *
 * @param pt      Protothread context.
 * @param rx_buf  Buffer to receive frame.
 * @param max_len Capacity.
 * @param rx_len  Pointer to receive length.
 */
#define PT_ETH_WAIT_FRAME(pt, rx_buf, max_len, rx_len) \
    PT_WAIT_UNTIL(pt, syn_port_eth_rx((rx_buf), (max_len), (rx_len)) == SYN_OK)

/**
 * @brief Block a protothread coroutine until an IP is resolved in the local ARP cache.
 *
 * @param pt      Protothread context.
 * @param eth     Pointer to Ethernet instance.
 * @param target_ip Target IPv4 address.
 * @param mac_out Output buffer (6 bytes).
 */
#define PT_ETH_WAIT_ARP(pt, eth, target_ip, mac_out) \
    PT_WAIT_UNTIL(pt, syn_eth_arp_lookup((eth), (target_ip), (mac_out)) == SYN_OK)

#ifdef __cplusplus
}
#endif

#endif /* SYN_ETH_H */
