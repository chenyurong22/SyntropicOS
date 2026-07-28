/**
 * @file syn_mavlink.h
 * @brief Zero-Heap MAVLink v2 Protocol Encoder & Streaming Decoder.
 *
 * MAVLink v2 Frame Specification:
 * Header: [0xFD] + [Payload Len 1B] + [Incompat Flags 1B] + [Compat Flags 1B] +
 *         [Seq 1B] + [Sys ID 1B] + [Comp ID 1B] + [Msg ID 3B (Little-Endian)]
 * Payload: [Payload N B]
 * Checksum: [16-bit CRC-X25 over (Header_without_STX + Payload + CRC_Extra)]
 */

#ifndef SYN_MAVLINK_H
#define SYN_MAVLINK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "syntropic/common/syn_defs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SYN_MAVLINK_STX_V2 0xFD
#define SYN_MAVLINK_MAX_PAYLOAD_LEN 255U
#define SYN_MAVLINK_HEADER_LEN 10U

/** MAVLink Message IDs. */
typedef enum {
    SYN_MAVLINK_MSG_HEARTBEAT = 0U,
    SYN_MAVLINK_MSG_SYS_STATUS = 1U,
    SYN_MAVLINK_MSG_ATTITUDE = 30U,
    SYN_MAVLINK_MSG_GLOBAL_POSITION_INT = 33U,
    SYN_MAVLINK_MSG_VFR_HUD = 74U
} SYN_MAVLINK_MsgId;

/** Parsed MAVLink v2 Frame. */
typedef struct {
    uint8_t payload_len;
    uint8_t incompat_flags;
    uint8_t compat_flags;
    uint8_t seq;
    uint8_t sys_id;
    uint8_t comp_id;
    uint32_t msg_id;
    uint8_t payload[SYN_MAVLINK_MAX_PAYLOAD_LEN];
} SYN_MAVLINK_Frame;

/** MAVLink v2 Streaming Parser Instance. */
typedef struct {
    uint8_t state;
    uint8_t payload_len;
    uint8_t incompat_flags;
    uint8_t compat_flags;
    uint8_t seq;
    uint8_t sys_id;
    uint8_t comp_id;
    uint32_t msg_id;
    uint8_t payload_idx;
    uint8_t payload[SYN_MAVLINK_MAX_PAYLOAD_LEN];
    uint16_t crc;
    SYN_MAVLINK_Frame last_frame;
    uint32_t packets_received;
    uint32_t crc_errors;
} SYN_MAVLINK_Parser;

/**
 * @brief Initialize MAVLink v2 parser instance.
 *
 * @param parser Pointer to parser struct.
 * @return SYN_OK on success.
 */
SYN_Status syn_mavlink_init(SYN_MAVLINK_Parser *parser);

/**
 * @brief Process incoming byte from serial stream.
 *
 * @param parser Pointer to parser struct.
 * @param byte   Incoming byte.
 * @param frame  Destination frame struct (populated on valid complete packet).
 * @return SYN_OK on complete valid frame, SYN_BUSY if processing, SYN_ERROR on CRC mismatch.
 */
SYN_Status syn_mavlink_parse_byte(SYN_MAVLINK_Parser *parser, uint8_t byte,
                                  SYN_MAVLINK_Frame *frame);

/**
 * @brief Encode a MAVLink v2 packet into raw byte buffer.
 *
 * @param sys_id     System ID (1..255).
 * @param comp_id    Component ID (1..255).
 * @param seq        Packet sequence number.
 * @param msg_id     Message ID.
 * @param crc_extra  CRC Extra byte for the specific message ID.
 * @param payload    Pointer to payload data buffer (or NULL if 0 len).
 * @param payload_len Payload byte length.
 * @param buf_out    Output byte buffer (must hold at least `payload_len + 12` bytes).
 * @param out_len    Pointer to receive final encoded frame length.
 * @return SYN_OK on success.
 */
SYN_Status syn_mavlink_encode_msg(uint8_t sys_id, uint8_t comp_id, uint8_t seq, uint32_t msg_id,
                                  uint8_t crc_extra, const uint8_t *payload, uint8_t payload_len,
                                  uint8_t *buf_out, size_t *out_len);

/**
 * @brief Compute MAVLink X25 CRC over buffer.
 *
 * @param data Pointer to input data.
 * @param len  Length in bytes.
 * @param crc  Initial CRC accumulator.
 * @return Updated 16-bit CRC value.
 */
uint16_t syn_mavlink_crc_accumulate(uint8_t byte, uint16_t crc);

#ifdef __cplusplus
}
#endif

#endif /* SYN_MAVLINK_H */
