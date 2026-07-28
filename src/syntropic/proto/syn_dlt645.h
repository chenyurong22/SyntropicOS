/**
 * @file syn_dlt645.h
 * @ingroup syn_protocol
 * @brief DL/T 645 Electricity Meter Communication Protocol (1997 & 2007).
 *
 * Implements DL/T 645-1997 and DL/T 645-2007 master-slave request/response
 * encoding, decoding, checksum validation, and streaming reception.
 */

#ifndef SYN_DLT645_H
#define SYN_DLT645_H

#include "../common/syn_defs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Protocol Constants ─────────────────────────────────────────────────── */

#define SYN_DLT645_SOF 0x68      /**< Start of frame delimiter byte (0x68) */
#define SYN_DLT645_EOF 0x16      /**< End of frame delimiter byte (0x16) */
#define SYN_DLT645_OFFSET 0x33   /**< Data byte encoding offset (+0x33) */
#define SYN_DLT645_PREAMBLE 0xFE /**< Preamble wake-up byte (0xFE) */

#define SYN_DLT645_ADDR_LEN 6 /**< Meter BCD address length in bytes (6) */

/* ── Protocol Versions ──────────────────────────────────────────────────── */

/** DL/T 645 Protocol Version Standard Enum. */
typedef enum {
    SYN_DLT645_VER_1997 = 0, /**< DL/T 645-1997 (2-byte Data Identifier) */
    SYN_DLT645_VER_2007 = 1  /**< DL/T 645-2007 (4-byte Data Identifier) */
} SYN_DLT645_Ver;

/* ── Control Codes ──────────────────────────────────────────────────────── */

/** DL/T 645 Control Code Commands Enum. */
typedef enum {
    SYN_DLT645_CMD_READ_DATA = 0x11,        /**< Master read data request            */
    SYN_DLT645_CMD_READ_DATA_RESP = 0x91,   /**< Slave read data response            */
    SYN_DLT645_CMD_WRITE_DATA = 0x14,       /**< Master write data request           */
    SYN_DLT645_CMD_WRITE_DATA_RESP = 0x94,  /**< Slave write data response           */
    SYN_DLT645_CMD_READ_ADDR = 0x13,        /**< Read meter address request          */
    SYN_DLT645_CMD_READ_ADDR_RESP = 0x93,   /**< Read meter address response         */
    SYN_DLT645_CMD_WRITE_ADDR = 0x15,       /**< Write meter address request         */
    SYN_DLT645_CMD_WRITE_ADDR_RESP = 0x95,  /**< Write meter address response        */
    SYN_DLT645_CMD_CHANGE_BAUD = 0x17,      /**< Change baud rate request            */
    SYN_DLT645_CMD_CHANGE_BAUD_RESP = 0x97, /**< Change baud rate response           */
    SYN_DLT645_CMD_ERROR_RESP = 0xD1        /**< Slave error response                */
} SYN_DLT645_Cmd;

/* ── Frame Structure ────────────────────────────────────────────────────── */

/** @brief Decoded DL/T 645 frame representation. */
typedef struct {
    uint8_t address[SYN_DLT645_ADDR_LEN]; /**< 6-byte BCD meter address              */
    uint8_t control;                      /**< Control code byte                     */
    uint32_t data_id;                     /**< Data Identifier (DI)                  */
    uint8_t payload[64];                  /**< Raw un-offset data payload bytes      */
    uint8_t payload_len;                  /**< Payload length (excluding DI)         */
    SYN_DLT645_Ver version;               /**< Protocol version (1997 or 2007)       */
} SYN_DLT645_Frame;

/* ── Streaming Decoder ──────────────────────────────────────────────────── */

/** @brief Frame received callback. */
typedef void (*SYN_DLT645_FrameCallback)(const SYN_DLT645_Frame *frame, void *ctx);

/** @brief Streaming byte-at-a-time decoder state. */
typedef struct {
    uint8_t rx_buf[128];         /**< Internal reception buffer             */
    size_t rx_len;               /**< Received byte count                   */
    SYN_DLT645_FrameCallback cb; /**< Completion callback                   */
    void *cb_ctx;                /**< Callback context                      */
    SYN_DLT645_Ver version;      /**< Expected version                      */
} SYN_DLT645_Decoder;

/* ── API ────────────────────────────────────────────────────────────────── */

/**
 * @brief Compute DL/T 645 arithmetic modulo-256 checksum over buffer.
 * @param buf  Buffer starting from SOF (0x68).
 * @param len  Length of buffer from SOF to end of data field.
 * @return Modulo-256 checksum byte.
 */
uint8_t syn_dlt645_calc_checksum(const uint8_t *buf, size_t len);

/**
 * @brief Encode a DL/T 645 frame into a binary transmission buffer.
 * @param frame         Input frame data.
 * @param out_buf       Destination output buffer.
 * @param out_capacity  Output buffer capacity (min 20 bytes).
 * @return Number of bytes written to out_buf (0 on error).
 */
size_t syn_dlt645_encode(const SYN_DLT645_Frame *frame, uint8_t *out_buf, size_t out_capacity);

/**
 * @brief Parse a raw byte buffer into a DL/T 645 frame.
 * @param in_buf     Input buffer containing the complete frame.
 * @param len        Length of input buffer.
 * @param version    Protocol version expected (1997 or 2007).
 * @param out_frame  Destination frame struct.
 * @return SYN_OK on success, SYN_INVALID_PARAM or SYN_ERROR on validation failure.
 */
SYN_Status syn_dlt645_parse(const uint8_t *in_buf, size_t len, SYN_DLT645_Ver version,
                            SYN_DLT645_Frame *out_frame);

/**
 * @brief Initialize a streaming DL/T 645 decoder.
 * @param dec      Decoder instance.
 * @param version  Expected protocol version.
 * @param cb       Frame received callback.
 * @param ctx      User callback context.
 */
void syn_dlt645_decoder_init(SYN_DLT645_Decoder *dec, SYN_DLT645_Ver version,
                             SYN_DLT645_FrameCallback cb, void *ctx);

/**
 * @brief Feed a single byte into the streaming decoder.
 * @param dec      Decoder instance.
 * @param rx_byte  Single byte received from UART ISR.
 */
void syn_dlt645_decoder_feed(SYN_DLT645_Decoder *dec, uint8_t rx_byte);

#ifdef __cplusplus
}
#endif

#endif /* SYN_DLT645_H */
