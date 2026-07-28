/**
 * @file syn_icmp.c
 * @brief Zero-Heap Native ICMP Protocol Engine Implementation.
 */

#include "syntropic/net/syn_icmp.h"

#include <string.h>

SYN_Status syn_icmp_init(SYN_ICMP *icmp)
{
    if (!icmp) {
        return SYN_INVALID_PARAM;
    }
    memset(icmp, 0, sizeof(*icmp));
    return SYN_OK;
}

uint16_t syn_icmp_checksum(const void *buf, size_t len)
{
    const uint8_t *ptr = (const uint8_t *)buf;
    uint32_t sum = 0;

    while (len > 1) {
        sum += ((uint16_t)ptr[0] << 8) | ptr[1];
        ptr += 2;
        len -= 2;
    }

    if (len == 1) {
        sum += (uint16_t)ptr[0] << 8;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);
}

SYN_Status syn_icmp_process_packet(SYN_ICMP *icmp, const uint8_t *ip_pkt, size_t len,
                                   uint8_t *frame_tx, size_t *tx_len)
{
    if (!icmp || !ip_pkt || len < 34) {
        return SYN_INVALID_PARAM;
    }

    if (tx_len) {
        *tx_len = 0;
    }

    /* Verify Ethernet EtherType == IPv4 (0x0800) */
    uint16_t ethertype = ((uint16_t)ip_pkt[12] << 8) | ip_pkt[13];
    if (ethertype != SYN_ETHTYPE_IPV4) {
        return SYN_OK;
    }

    /* IP Header begins at offset 14 */
    const uint8_t *ip_hdr = &ip_pkt[14];
    uint8_t ip_hl = (ip_hdr[0] & 0x0F) * 4;
    if (ip_hl < 20 || (size_t)(14 + ip_hl) > len) {
        return SYN_INVALID_PARAM;
    }

    uint8_t proto = ip_hdr[9];
    if (proto != 1) { /* Protocol 1 = ICMP */
        return SYN_OK;
    }

    /* ICMP Header begins after IP Header */
    size_t icmp_offset = 14 + ip_hl;
    if (icmp_offset + SYN_ICMP_HEADER_LEN > len) {
        return SYN_INVALID_PARAM;
    }

    const uint8_t *icmp_hdr = &ip_pkt[icmp_offset];
    uint8_t type = icmp_hdr[0];

    if (type == SYN_ICMP_TYPE_ECHO_REQUEST) {
        icmp->echo_requests_rx++;

        if (frame_tx && tx_len) {
            size_t pkt_len = len;
            if (pkt_len > 1514) {
                pkt_len = 1514;
            }

            memcpy(frame_tx, ip_pkt, pkt_len);

            /* Swap MAC addresses (offset 0 and 6) */
            memcpy(&frame_tx[0], &ip_pkt[6], 6);
            memcpy(&frame_tx[6], &ip_pkt[0], 6);

            /* Swap IP addresses (offset 26 and 30) */
            memcpy(&frame_tx[26], &ip_pkt[30], 4);
            memcpy(&frame_tx[30], &ip_pkt[26], 4);

            /* Modify ICMP Type: 8 (Request) -> 0 (Reply) */
            size_t tx_icmp_off = icmp_offset;
            frame_tx[tx_icmp_off] = SYN_ICMP_TYPE_ECHO_REPLY;
            frame_tx[tx_icmp_off + 2] = 0; /* Reset checksum bytes */
            frame_tx[tx_icmp_off + 3] = 0;

            size_t icmp_payload_len = pkt_len - tx_icmp_off;
            uint16_t csum = syn_icmp_checksum(&frame_tx[tx_icmp_off], icmp_payload_len);
            frame_tx[tx_icmp_off + 2] = (uint8_t)(csum >> 8);
            frame_tx[tx_icmp_off + 3] = (uint8_t)(csum & 0xFF);

            /* Zero pad frame to minimum Ethernet length (60 bytes) */
            if (pkt_len < SYN_ETH_MIN_FRAME_LEN) {
                memset(&frame_tx[pkt_len], 0, SYN_ETH_MIN_FRAME_LEN - pkt_len);
                pkt_len = SYN_ETH_MIN_FRAME_LEN;
            }

            icmp->echo_replies_tx++;
            *tx_len = pkt_len;
        }
    } else if (type == SYN_ICMP_TYPE_ECHO_REPLY) {
        icmp->echo_replies_rx++;
    }

    return SYN_OK;
}

SYN_Status syn_icmp_build_echo_request(SYN_ICMP *icmp, SYN_ETH *eth, uint32_t dst_ip,
                                       const uint8_t dst_mac[6], uint16_t id, uint16_t seq,
                                       const uint8_t *payload, size_t payload_len,
                                       uint8_t *frame_out, size_t *frame_len)
{
    if (!icmp || !eth || !dst_mac || !frame_out || !frame_len) {
        return SYN_INVALID_PARAM;
    }

    size_t ip_len = 20 + SYN_ICMP_HEADER_LEN + payload_len;
    size_t total_len = 14 + ip_len;
    if (total_len < SYN_ETH_MIN_FRAME_LEN) {
        total_len = SYN_ETH_MIN_FRAME_LEN;
    }
    if (total_len > SYN_ETH_MAX_FRAME_LEN) {
        return SYN_INVALID_PARAM;
    }

    memset(frame_out, 0, total_len);

    /* Ethernet II Header */
    memcpy(&frame_out[0], dst_mac, 6);
    memcpy(&frame_out[6], eth->mac_addr, 6);
    frame_out[12] = 0x08;
    frame_out[13] = 0x00; /* IPv4 */

    /* IP Header */
    frame_out[14] = 0x45; /* Version 4, IHL 5 */
    frame_out[15] = 0x00; /* TOS */
    frame_out[16] = (uint8_t)(ip_len >> 8);
    frame_out[17] = (uint8_t)(ip_len & 0xFF);
    frame_out[18] = (uint8_t)(id >> 8);
    frame_out[19] = (uint8_t)(id & 0xFF);
    frame_out[20] = 0x40;
    frame_out[21] = 0x00; /* Don't Fragment */
    frame_out[22] = 64;   /* TTL = 64 */
    frame_out[23] = 1;    /* Protocol = ICMP */
    frame_out[24] = 0;
    frame_out[25] = 0; /* IP Checksum placeholder */

    frame_out[26] = (uint8_t)(eth->ip_addr >> 24);
    frame_out[27] = (uint8_t)(eth->ip_addr >> 16);
    frame_out[28] = (uint8_t)(eth->ip_addr >> 8);
    frame_out[29] = (uint8_t)(eth->ip_addr);

    frame_out[30] = (uint8_t)(dst_ip >> 24);
    frame_out[31] = (uint8_t)(dst_ip >> 16);
    frame_out[32] = (uint8_t)(dst_ip >> 8);
    frame_out[33] = (uint8_t)(dst_ip);

    uint16_t ip_csum = syn_icmp_checksum(&frame_out[14], 20);
    frame_out[24] = (uint8_t)(ip_csum >> 8);
    frame_out[25] = (uint8_t)(ip_csum & 0xFF);

    /* ICMP Header */
    size_t icmp_off = 34;
    frame_out[icmp_off] = SYN_ICMP_TYPE_ECHO_REQUEST;
    frame_out[icmp_off + 1] = 0; /* Code */
    frame_out[icmp_off + 2] = 0;
    frame_out[icmp_off + 3] = 0; /* Checksum placeholder */
    frame_out[icmp_off + 4] = (uint8_t)(id >> 8);
    frame_out[icmp_off + 5] = (uint8_t)(id & 0xFF);
    frame_out[icmp_off + 6] = (uint8_t)(seq >> 8);
    frame_out[icmp_off + 7] = (uint8_t)(seq & 0xFF);

    if (payload && payload_len > 0) {
        memcpy(&frame_out[icmp_off + 8], payload, payload_len);
    }

    uint16_t icmp_csum = syn_icmp_checksum(&frame_out[icmp_off], SYN_ICMP_HEADER_LEN + payload_len);
    frame_out[icmp_off + 2] = (uint8_t)(icmp_csum >> 8);
    frame_out[icmp_off + 3] = (uint8_t)(icmp_csum & 0xFF);

    icmp->echo_requests_tx++;
    *frame_len = total_len;

    return SYN_OK;
}
