/**
 * @file syn_ir.h
 * @brief Infrared (IR) Remote Control Protocol Engine (NEC, Sony, RC5, RC6, Samsung, Panasonic,
 * Denon, Apple).
 *
 * Implements non-blocking, zero-malloc IR pulse decoding and encoding for common
 * consumer remote control protocols using microsecond pulse timing inputs.
 *
 * @par Decoded Protocols
 * - NEC (Standard & Extended 32-bit)
 * - Sony SIRCS (12-bit, 15-bit, 20-bit)
 * - Philips RC5 (14-bit Manchester)
 * - Philips RC6 Mode 0 (21-bit Manchester)
 * - Samsung (32-bit PDM)
 * - Kaseikyo / Panasonic (48-bit PDM)
 * - Denon / Sharp (15-bit PDM)
 * - Apple (32-bit NEC variant)
 *
 * @par Usage (Decoder)
 * @code
 *   static SYN_IR_Decoder decoder;
 *   syn_ir_decoder_init(&decoder);
 *
 *   // In Timer Input Capture / EXTI ISR:
 *   SYN_IR_Frame frame;
 *   if (syn_ir_decode_pulse(&decoder, duration_us, is_mark, &frame)) {
 *       // Process decoded frame: frame.protocol, frame.address, frame.command
 *   }
 * @endcode
 * @ingroup syn_proto
 */

#ifndef SYN_IR_H
#define SYN_IR_H

#include "../common/syn_defs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(SYN_USE_IR) && SYN_USE_IR

#ifdef __cplusplus
extern "C" {
#endif

/* ── Encoding Types ────────────────────────────────────────────────────── */

/**
 * @brief IR signal encoding types.
 */
typedef enum {
    SYN_IR_ENC_PDM = 0,        /**< Pulse Distance Modulation (e.g. NEC, Samsung) */
    SYN_IR_ENC_PWM = 1,        /**< Pulse Width Modulation (e.g. Sony SIRCS) */
    SYN_IR_ENC_MANCHESTER = 2, /**< Manchester Bi-Phase (e.g. RC5, RC6) */
    SYN_IR_ENC_PPM = 3         /**< Pulse Position Modulation (e.g. RECS80, RCMM) */
} SYN_IR_EncodingType;

/* ── Protocol Identifiers ───────────────────────────────────────────────── */

/**
 * @brief Supported IR remote control protocols.
 */
typedef enum {
    SYN_IR_PROTO_UNKNOWN = 0,
    SYN_IR_PROTO_NEC,          /**< NEC standard (8-bit addr, 8-bit cmd) */
    SYN_IR_PROTO_NEC_EXTENDED, /**< NEC extended (16-bit addr, 8-bit cmd) */
    SYN_IR_PROTO_SONY_12,      /**< Sony SIRCS 12-bit (7 cmd, 5 addr) */
    SYN_IR_PROTO_SONY_15,      /**< Sony SIRCS 15-bit (7 cmd, 8 addr) */
    SYN_IR_PROTO_SONY_20,      /**< Sony SIRCS 20-bit (7 cmd, 5 addr, 8 ext) */
    SYN_IR_PROTO_RC5,          /**< Philips RC5 (5 addr, 6 cmd, toggle) */
    SYN_IR_PROTO_RC6,          /**< Philips RC6 Mode 0 (8 addr, 8 cmd, toggle) */
    SYN_IR_PROTO_SAMSUNG,      /**< Samsung 32-bit (8 addr, 8 addr, 8 cmd, 8 cmd_inv) */
    SYN_IR_PROTO_KASEIKYO,     /**< Panasonic/Kaseikyo 48-bit */
    SYN_IR_PROTO_DENON,        /**< Denon/Sharp 15-bit */
    SYN_IR_PROTO_APPLE,        /**< Apple NEC variant (8 device, 8 cmd, 8 pairID) */
    SYN_IR_PROTO_COUNT
} SYN_IR_Protocol;

/* ── Frame Flags ────────────────────────────────────────────────────────── */

#define SYN_IR_FLAG_NONE 0x0000U
#define SYN_IR_FLAG_REPEAT (1U << 0) /**< Set if this frame is a repeat code / held key */
#define SYN_IR_FLAG_TOGGLE (1U << 1) /**< Toggle bit active (RC5 / RC6) */

/* ── Decoded IR Frame ───────────────────────────────────────────────────── */

/**
 * @brief Decoded IR Remote Control Frame.
 */
typedef struct {
    SYN_IR_Protocol protocol; /**< Protocol type */
    uint32_t address;         /**< Decoded address field */
    uint32_t command;         /**< Decoded command field */
    uint16_t flags;           /**< Frame flags (SYN_IR_FLAG_REPEAT, SYN_IR_FLAG_TOGGLE) */
    uint8_t bit_count;        /**< Total bit count */
    uint8_t carrier_khz;      /**< Nominal carrier frequency in kHz (36, 38, 40) */
} SYN_IR_Frame;

/* ── Timing Pulse Representation (Encoder Output) ────────────────────────── */

/**
 * @brief Single pulse duration timing pair for IR transmitter.
 */
typedef struct {
    uint16_t duration_us; /**< Duration in microseconds */
    bool is_mark;         /**< true = IR carrier ON, false = Space (OFF) */
} SYN_IR_Pulse;

/* ── Decoder State Machine Handle ───────────────────────────────────────── */

typedef enum {
    SYN_IR_STATE_IDLE = 0,
    SYN_IR_STATE_LEADER,
    SYN_IR_STATE_DATA,
    SYN_IR_STATE_TRAILER
} SYN_IR_FsmState;

/**
 * @brief Non-blocking IR Decoder Handle.
 */
typedef struct {
    SYN_IR_FsmState state;        /**< Current FSM state */
    SYN_IR_Protocol active_proto; /**< Protocol currently being decoded */
    uint64_t bits;                /**< Bit shift register */
    uint8_t bit_idx;              /**< Number of bits accumulated */
    uint8_t expected_bits;        /**< Total expected bits for active protocol */
    uint16_t last_mark_us;        /**< Last mark pulse duration */
    uint16_t last_space_us;       /**< Last space pulse duration */
    SYN_IR_Frame last_frame;      /**< Last successfully decoded frame */
    bool have_last;               /**< True if a frame has been decoded */
    bool manchester_phase;        /**< Manchester phase tracking */
} SYN_IR_Decoder;

/* ── API Functions ──────────────────────────────────────────────────────── */

/**
 * @brief Initialize or reset an IR decoder instance.
 * @param decoder Pointer to IR decoder handle.
 * @return SYN_OK on success, SYN_INVALID_PARAM on NULL.
 */
SYN_Status syn_ir_decoder_init(SYN_IR_Decoder *decoder);

/**
 * @brief Process a single pulse duration (mark or space) in microseconds.
 * @param decoder     Pointer to IR decoder handle.
 * @param duration_us Pulse duration in microseconds.
 * @param is_mark     true if pulse is active IR carrier (mark), false if space.
 * @param frame_out   [out] Populated with decoded frame when function returns true.
 * @return true if a complete, valid frame was decoded, false otherwise.
 */
bool syn_ir_decode_pulse(SYN_IR_Decoder *decoder, uint16_t duration_us, bool is_mark,
                         SYN_IR_Frame *frame_out);

/**
 * @brief Signal a gap/timeout (>10ms silence) to finalize protocols.
 * @param decoder   Pointer to IR decoder handle.
 * @param frame_out [out] Populated with decoded frame if timeout completed frame.
 * @return true if a complete valid frame was finalized by timeout, false otherwise.
 */
bool syn_ir_decode_timeout(SYN_IR_Decoder *decoder, SYN_IR_Frame *frame_out);

/**
 * @brief Encode a frame into a sequence of pulse timing pairs.
 * @param frame      Pointer to frame to encode.
 * @param pulse_buf  Output array for pulse timings.
 * @param buf_len    Capacity of pulse_buf.
 * @param count_out  [out] Number of pulse timing pairs written.
 * @return SYN_OK on success, SYN_INVALID_PARAM or SYN_ERROR on failure.
 */
SYN_Status syn_ir_encode_frame(const SYN_IR_Frame *frame, SYN_IR_Pulse *pulse_buf, size_t buf_len,
                               size_t *count_out);

/**
 * @brief Get human-readable protocol string name.
 * @param proto Protocol enum value.
 * @return Pointer to const string.
 */
const char *syn_ir_protocol_name(SYN_IR_Protocol proto);

#ifdef __cplusplus
}
#endif

#endif /* SYN_USE_IR */

#endif /* SYN_IR_H */
