/**
 * @file syn_dhcp.h
 * @brief Zero-Heap Native DHCP Client Protocol Engine.
 *
 * Specifications:
 * - RFC 2131 Dynamic Host Configuration Protocol
 * - UDP Port 68 (Client) <-> Port 67 (Server)
 * - Zero dynamic memory allocation (0 bytes heap)
 * - DHCP Option parsing: Subnet Mask (1), Gateway (3), Lease Time (51), Msg Type (53)
 */

#ifndef SYN_DHCP_H
#define SYN_DHCP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "syntropic/common/syn_defs.h"
#include "syntropic/net/syn_eth.h"
#include "syntropic/pt/syn_pt.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SYN_DHCP_CLIENT_PORT 68U
#define SYN_DHCP_SERVER_PORT 67U

#define SYN_DHCP_MAGIC_COOKIE 0x63825363UL

#define SYN_DHCP_DISCOVER 1U
#define SYN_DHCP_OFFER 2U
#define SYN_DHCP_REQUEST 3U
#define SYN_DHCP_DECLINE 4U
#define SYN_DHCP_ACK 5U
#define SYN_DHCP_NAK 6U

#define SYN_DHCP_OPT_SUBNET_MASK 1U
#define SYN_DHCP_OPT_ROUTER 3U
#define SYN_DHCP_OPT_LEASE_TIME 51U
#define SYN_DHCP_OPT_MSG_TYPE 53U
#define SYN_DHCP_OPT_END 255U

/** DHCP Client State Machine Enum. */
typedef enum {
    SYN_DHCP_STATE_INIT = 0,
    SYN_DHCP_STATE_DISCOVER,
    SYN_DHCP_STATE_OFFER,
    SYN_DHCP_STATE_REQUEST,
    SYN_DHCP_STATE_BOUND
} SYN_DHCP_State;

/** DHCP Client Context Descriptor. */
typedef struct {
    SYN_DHCP_State state;
    uint32_t xid;            /**< Transaction ID */
    uint32_t offered_ip;     /**< IP offered by server */
    uint32_t subnet_mask;    /**< Subnet mask */
    uint32_t gateway;        /**< Router gateway IP */
    uint32_t lease_time_sec; /**< Lease duration */
    uint32_t server_ip;      /**< DHCP Server IP */
    uint32_t discovers_sent; /**< Telemetry counter */
    uint32_t requests_sent;  /**< Telemetry counter */
    uint32_t acks_received;  /**< Telemetry counter */
} SYN_DHCP;

/**
 * @brief Initialize DHCP Client Context.
 *
 * @param dhcp Pointer to DHCP context.
 * @param xid  32-bit transaction ID (e.g. random value or tick count).
 * @return SYN_OK on success.
 */
SYN_Status syn_dhcp_init(SYN_DHCP *dhcp, uint32_t xid);

/**
 * @brief Build a DHCPDISCOVER UDP packet payload.
 *
 * @param dhcp     Pointer to DHCP context.
 * @param mac_addr 6-byte client MAC address.
 * @param buf_out  Output buffer (must hold at least 250 bytes).
 * @param len_out  Pointer to receive byte length of generated payload.
 * @return SYN_OK on success.
 */
SYN_Status syn_dhcp_build_discover(SYN_DHCP *dhcp, const uint8_t mac_addr[6], uint8_t *buf_out,
                                   size_t *len_out);

/**
 * @brief Build a DHCPREQUEST UDP packet payload.
 *
 * @param dhcp     Pointer to DHCP context.
 * @param mac_addr 6-byte client MAC address.
 * @param buf_out  Output buffer (must hold at least 250 bytes).
 * @param len_out  Pointer to receive byte length of generated payload.
 * @return SYN_OK on success.
 */
SYN_Status syn_dhcp_build_request(SYN_DHCP *dhcp, const uint8_t mac_addr[6], uint8_t *buf_out,
                                  size_t *len_out);

/**
 * @brief Process incoming DHCP response packet payload (DHCPOFFER or DHCPACK).
 *
 * @param dhcp       Pointer to DHCP context.
 * @param eth        Pointer to Ethernet context (updated automatically when BOUND).
 * @param dhcp_pkt   Pointer to incoming DHCP UDP payload.
 * @param len        Payload byte length.
 * @return SYN_OK on success, SYN_BUSY if still waiting/in-progress.
 */
SYN_Status syn_dhcp_process_packet(SYN_DHCP *dhcp, SYN_ETH *eth, const uint8_t *dhcp_pkt,
                                   size_t len);

/* ── Protothread Coroutine Integration ──────────────────────────────────── */

/**
 * @brief Block a protothread coroutine until DHCP IP lease binding completes.
 *
 * @param pt   Protothread context.
 * @param dhcp Pointer to DHCP context.
 */
#define PT_DHCP_WAIT_BOUND(pt, dhcp) PT_WAIT_UNTIL(pt, (dhcp)->state == SYN_DHCP_STATE_BOUND)

/**
 * @brief Block task execution (SYN_TASK_BLOCKED) until DHCP IP lease binding completes.
 *
 * @param pt   Protothread context.
 * @param task Pointer to SYN_Task.
 * @param dhcp Pointer to DHCP context.
 */
#define PT_DHCP_BLOCK_BOUND(pt, task, dhcp) \
    PT_BLOCK_CONDITION(pt, task, (dhcp)->state == SYN_DHCP_STATE_BOUND)

#ifdef __cplusplus
}
#endif

#endif /* SYN_DHCP_H */
