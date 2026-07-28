/**
 * @file syn_udp.h
 * @brief Zero-Alloc Native UDP Demuxing Engine & Task-Blocking Layer.
 * @ingroup syn_net
 */

#ifndef SYN_UDP_H
#define SYN_UDP_H

#include "syntropic/common/syn_defs.h"
#include "syntropic/net/syn_eth.h"
#include "syntropic/pt/syn_pt.h"
#include "syntropic/sched/syn_sched.h"
#include "syntropic/sched/syn_task.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Constants ─────────────────────────────────────────────────────────── */

#ifndef SYN_UDP_MAX_SOCKETS
#define SYN_UDP_MAX_SOCKETS 4 /**< Maximum concurrent UDP bound sockets */
#endif

#ifndef SYN_UDP_BUF_SIZE
#define SYN_UDP_BUF_SIZE 256 /**< UDP socket internal RX buffer size in bytes */
#endif

/** @brief UDP packet header structure (8 bytes). */
typedef struct {
    uint16_t src_port; /**< Source port number. */
    uint16_t dst_port; /**< Destination port number. */
    uint16_t length;   /**< Length of UDP header + payload in bytes. */
    uint16_t checksum; /**< Checksum over pseudo-header and payload. */
} SYN_UdpHeader;

/* ── UDP Socket Slot ────────────────────────────────────────────────────── */

/** @brief UDP socket slot control block. */
typedef struct {
    bool is_bound;        /**< True if socket is bound to a local port. */
    uint16_t local_port;  /**< Bound local port number. */
    uint32_t remote_ip;   /**< Connected remote IPv4 address. */
    uint16_t remote_port; /**< Connected remote port number. */

    uint8_t rx_buf[SYN_UDP_BUF_SIZE]; /**< Receive payload buffer. */
    uint16_t rx_len;                  /**< Received payload length. */

    SYN_Task *blocked_task; /**< Protothread task sleeping on this UDP socket. */
} SYN_UdpSocket;

/* ── UDP Stack Container ───────────────────────────────────────────────── */

/** @brief UDP protocol stack container instance. */
typedef struct {
    SYN_ETH *eth;                               /**< Associated Ethernet interface instance. */
    SYN_UdpSocket sockets[SYN_UDP_MAX_SOCKETS]; /**< Array of managed UDP sockets. */
} SYN_UDP;

/* ── Functions ─────────────────────────────────────────────────────────── */

/**
 * @brief Initialize UDP Stack.
 * @param udp Pointer to UDP stack instance.
 * @param eth Pointer to Ethernet interface instance.
 * @return SYN_OK on success.
 */
SYN_Status syn_udp_init(SYN_UDP *udp, SYN_ETH *eth);

/**
 * @brief Bind UDP socket to local port.
 * @param udp        Pointer to UDP stack instance.
 * @param local_port Port number to bind.
 * @return Pointer to allocated UDP socket slot, or NULL if capacity exceeded.
 */
SYN_UdpSocket *syn_udp_bind(SYN_UDP *udp, uint16_t local_port);

/**
 * @brief Unbind and release a UDP socket slot.
 * @param sock Pointer to UDP socket slot.
 */
void syn_udp_unbind(SYN_UdpSocket *sock);

/**
 * @brief Process incoming UDP IPv4 packet.
 * @param udp       Pointer to UDP stack instance.
 * @param ip_packet Pointer to raw IPv4 packet.
 * @param len       IPv4 packet byte length.
 * @return SYN_OK on success.
 */
SYN_Status syn_udp_process_packet(SYN_UDP *udp, const uint8_t *ip_packet, size_t len);

/**
 * @brief Construct outbound UDP packet into frame output buffer.
 * @param udp      Pointer to UDP stack.
 * @param src_port Source UDP port.
 * @param dst_ip   Destination IPv4 address.
 * @param dst_port Destination UDP port.
 * @param data     Pointer to payload data buffer.
 * @param data_len Payload byte length.
 * @param tx_out   Output Ethernet frame buffer.
 * @param tx_len   Pointer to receive final Ethernet frame length.
 * @return SYN_OK on success.
 */
int syn_udp_sendto(SYN_UDP *udp, uint16_t src_port, uint32_t dst_ip, uint16_t dst_port,
                   const uint8_t *data, size_t data_len, uint8_t *tx_out, size_t *tx_len);

/**
 * @brief Compute UDP 16-bit 1's complement checksum over pseudo-header and segment.
 * @param src_ip  Source 32-bit IPv4 address.
 * @param dst_ip  Destination 32-bit IPv4 address.
 * @param udp_seg Pointer to raw UDP segment bytes.
 * @param len     UDP segment length in bytes.
 * @return Computed 16-bit UDP checksum.
 */
uint16_t syn_udp_checksum(uint32_t src_ip, uint32_t dst_ip, const uint8_t *udp_seg, size_t len);

/* ── Task Blocking Macros ───────────────────────────────────────────────── */

/**
 * @brief Protothread helper macro to block task until UDP packet arrives.
 * @param pt         Protothread state.
 * @param task       Task instance.
 * @param sock       UDP socket pointer.
 * @param buf        Destination read buffer.
 * @param max_len    Maximum bytes to read.
 * @param bytes_read Pointer to output size_t receiving count of read bytes.
 * @param src_ip     Pointer to uint32_t receiving sender IP address.
 * @param src_port   Pointer to uint16_t receiving sender port.
 */
#define PT_UDP_BLOCK_READ(pt, task, sock, buf, max_len, bytes_read, src_ip, src_port) \
    do {                                                                              \
        (sock)->blocked_task = (task);                                                \
        PT_BLOCK_CONDITION(pt, task, (sock)->rx_len > 0 || !(sock)->is_bound);        \
        if ((sock)->rx_len > 0) {                                                     \
            size_t _n = ((sock)->rx_len < (max_len)) ? (sock)->rx_len : (max_len);    \
            memcpy((buf), (sock)->rx_buf, _n);                                        \
            memmove((sock)->rx_buf, (sock)->rx_buf + _n, (sock)->rx_len - _n);        \
            (sock)->rx_len -= _n;                                                     \
            *(bytes_read) = _n;                                                       \
            if (src_ip)                                                               \
                *(src_ip) = (sock)->remote_ip;                                        \
            if (src_port)                                                             \
                *(src_port) = (sock)->remote_port;                                    \
        } else {                                                                      \
            *(bytes_read) = 0;                                                        \
        }                                                                             \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* SYN_UDP_H */
