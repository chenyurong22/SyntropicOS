/**
 * @file syn_bacnet.h
 * @brief Zero-malloc, cleanroom BACnet MS/TP & APDU protocol stack (ISO 16484-5).
 * @ingroup syn_protocol
 *
 * Provides a lightweight, non-blocking BACnet MS/TP (Master-Slave/Token-Passing)
 * frame encoder/decoder, CRC-8 / CRC-16 verifiers, APDU parser (Who-Is, I-Am,
 * ReadProperty, WriteProperty), and static Object Database engine for embedded targets.
 */

#ifndef SYN_BACNET_H
#define SYN_BACNET_H

#include "../common/syn_defs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── BACnet MS/TP Constants ─────────────────────────────────────────────── */

#define SYN_BACNET_MSTP_PREAMBLE_1 0x55U /**< First MS/TP preamble byte (0x55) */
#define SYN_BACNET_MSTP_PREAMBLE_2 0xFFU /**< Second MS/TP preamble byte (0xFF) */
#define SYN_BACNET_BROADCAST_MAC 0xFFU   /**< Broadcast MAC address (0xFF) */

/** @brief BACnet MS/TP Frame Types (ISO 16484-5 Clause 9.2) */
typedef enum {
    SYN_BACNET_MSTP_FRAME_TOKEN = 0x00U,
    SYN_BACNET_MSTP_FRAME_POLL_FOR_MASTER = 0x01U,
    SYN_BACNET_MSTP_FRAME_REPLY_TO_POLL_FOR_MASTER = 0x02U,
    SYN_BACNET_MSTP_FRAME_TEST_REQUEST = 0x03U,
    SYN_BACNET_MSTP_FRAME_TEST_RESPONSE = 0x04U,
    SYN_BACNET_MSTP_FRAME_DATA_NOT_EXPECTING_REPLY = 0x05U,
    SYN_BACNET_MSTP_FRAME_DATA_EXPECTING_REPLY = 0x06U,
    SYN_BACNET_MSTP_FRAME_REPLY_POSTPONED = 0x07U,
} SYN_BACnet_MSTP_FrameType;

/** @brief BACnet Service Choices (Clause 21) */
typedef enum {
    SYN_BACNET_SERVICE_UNCONFIRMED_I_AM = 0U,
    SYN_BACNET_SERVICE_UNCONFIRMED_WHO_IS = 8U,
    SYN_BACNET_SERVICE_CONFIRMED_READ_PROPERTY = 12U,
    SYN_BACNET_SERVICE_CONFIRMED_WRITE_PROPERTY = 15U,
} SYN_BACnet_ServiceChoice;

/** @brief BACnet Object Types (Clause 12) */
typedef enum {
    SYN_BACNET_OBJ_ANALOG_INPUT = 0U,
    SYN_BACNET_OBJ_ANALOG_OUTPUT = 1U,
    SYN_BACNET_OBJ_BINARY_INPUT = 3U,
    SYN_BACNET_OBJ_BINARY_OUTPUT = 4U,
    SYN_BACNET_OBJ_DEVICE = 8U,
} SYN_BACnet_ObjectType;

/** @brief BACnet Property Identifiers */
typedef enum {
    SYN_BACNET_PROP_PRESENT_VALUE = 85U,
    SYN_BACNET_PROP_STATUS_FLAGS = 111U,
    SYN_BACNET_PROP_OBJECT_IDENTIFIER = 75U,
    SYN_BACNET_PROP_OBJECT_NAME = 77U,
    SYN_BACNET_PROP_OBJECT_TYPE = 79U,
    SYN_BACNET_PROP_VENDOR_ID = 121U,
} SYN_BACnet_PropertyID;

/* ── Data Structures ────────────────────────────────────────────────────── */

/** @brief Decoded BACnet MS/TP Frame Structure */
typedef struct {
    uint8_t frame_type;      /**< MS/TP Frame Type (0..7) */
    uint8_t destination_mac; /**< Destination MAC address (0..254, 255=Broadcast) */
    uint8_t source_mac;      /**< Source MAC address (0..254) */
    uint16_t data_len;       /**< Payload length in bytes */
    uint8_t payload[501];    /**< Payload buffer */
} SYN_BACnet_MSTP_Frame;

/** @brief BACnet Object Instance Definition */
typedef struct {
    uint8_t object_type;  /**< SYN_BACnet_ObjectType (AI, AO, BI, BO, Device) */
    uint32_t instance_id; /**< Object Instance Number (0..4194303) */
    float present_value;  /**< Current Numeric/Boolean Present Value */
    const char *name;     /**< Object Name string */
} SYN_BACnet_Object;

#ifndef SYN_BACNET_MAX_OBJECTS
#define SYN_BACNET_MAX_OBJECTS 16 /**< Maximum supported BACnet objects per node instance */
#endif

/** @brief BACnet MS/TP Node Context */
typedef struct {
    uint8_t mac_address;                               /**< Local MS/TP MAC (0..127) */
    uint32_t device_id;                                /**< BACnet Device Object Instance */
    uint8_t max_master;                                /**< Max Master MAC (default 127) */
    SYN_BACnet_Object objects[SYN_BACNET_MAX_OBJECTS]; /**< Static Object Table */
    size_t object_count;                               /**< Active Object count */
} SYN_BACnet_Node;

/* ── API Functions ──────────────────────────────────────────────────────── */

/**
 * @brief Calculate BACnet MS/TP Header CRC-8.
 * @param data Data buffer.
 * @param len Byte count.
 * @return CRC-8 checksum value.
 */
uint8_t syn_bacnet_crc8(const uint8_t *data, size_t len);

/**
 * @brief Calculate BACnet MS/TP Data CRC-16.
 * @param data Data buffer.
 * @param len Byte count.
 * @return CRC-16 checksum value.
 */
uint16_t syn_bacnet_crc16(const uint8_t *data, size_t len);

/**
 * @brief Encode a complete BACnet MS/TP Frame.
 *
 * @param frame_type  Frame Type enum.
 * @param dest_mac    Destination MAC address.
 * @param src_mac     Source MAC address.
 * @param payload     Payload buffer (can be NULL if payload_len == 0).
 * @param payload_len Payload byte count.
 * @param out_buf     Destination output buffer (must be >= payload_len + 10 bytes).
 * @return Total encoded frame byte count.
 */
size_t syn_bacnet_mstp_encode_frame(uint8_t frame_type, uint8_t dest_mac, uint8_t src_mac,
                                    const uint8_t *payload, uint16_t payload_len, uint8_t *out_buf);

/**
 * @brief Decode a raw byte buffer into a BACnet MS/TP Frame structure.
 *
 * @param buf   Input raw frame byte array.
 * @param len   Input byte count.
 * @param frame [out] Decoded MS/TP Frame structure.
 * @return true if valid MS/TP frame header and CRC-8/CRC-16 match.
 */
bool syn_bacnet_mstp_decode_frame(const uint8_t *buf, size_t len, SYN_BACnet_MSTP_Frame *frame);

/**
 * @brief Initialize a BACnet MS/TP Node context.
 * @param node        Node handle.
 * @param mac_address Local MAC address (0..127).
 * @param device_id   BACnet Device Instance ID (0..4194302).
 * @return SYN_OK on success.
 */
SYN_Status syn_bacnet_node_init(SYN_BACnet_Node *node, uint8_t mac_address, uint32_t device_id);

/**
 * @brief Register an Object in the BACnet Node database.
 * @param node         Node handle.
 * @param object_type  SYN_BACnet_ObjectType (AI, AO, BI, BO).
 * @param instance_id  Instance ID.
 * @param init_value   Initial Present Value.
 * @param name         Object Name string.
 * @return SYN_OK on success, SYN_ERROR if database full.
 */
SYN_Status syn_bacnet_add_object(SYN_BACnet_Node *node, uint8_t object_type, uint32_t instance_id,
                                 float init_value, const char *name);

/**
 * @brief Process an incoming BACnet MS/TP frame and generate a response if required.
 *
 * @param node      Node context.
 * @param rx_frame  Incoming received frame.
 * @param tx_frame  [out] Outgoing response frame structure.
 * @param has_tx    [out] Set to true if a response frame must be transmitted.
 * @return SYN_OK on success.
 */
SYN_Status syn_bacnet_node_process(SYN_BACnet_Node *node, const SYN_BACnet_MSTP_Frame *rx_frame,
                                   SYN_BACnet_MSTP_Frame *tx_frame, bool *has_tx);

#ifdef __cplusplus
}
#endif

#endif /* SYN_BACNET_H */
