/**
 * @file syn_ethercat.h
 * @brief EtherCAT (IEEE 802.3 EtherType 0x88A4) Bare-Metal Protocol Stack.
 *
 * Implements EtherCAT Frame packing, Datagram commands (APRD, APWR, APRW,
 * FPRD, FPWR, FPRW, BRD, BWR, LRD, LWR, LRW), Working Counter (WKC) accounting,
 * EtherCAT State Machine (ESM: INIT, PREOP, BOOT, SAFEOP, OP), and CoE
 * (CAN application protocol over EtherCAT) Mailbox SDO & PDO handling.
 *
 * Designed for zero heap allocation and direct L2 raw Ethernet frame processing.
 * @ingroup syn_protocol
 */

#ifndef SYN_ETHERCAT_H
#define SYN_ETHERCAT_H

#include "../common/syn_defs.h"
#include "syn_canopen.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief EtherCAT IEEE 802.3 Ethernet Frame Type */
#define SYN_ETHERCAT_ETHERTYPE 0x88A4

/** @brief EtherCAT Header Type (1 = EtherCAT Datagrams) */
#define SYN_ETHERCAT_TYPE_DATAGRAM 1

/* ── EtherCAT Commands ─────────────────────────────────────────────────── */

typedef enum {
    SYN_ECAT_CMD_NOP = 0,   /**< No Operation */
    SYN_ECAT_CMD_APRD = 1,  /**< Auto Increment Read */
    SYN_ECAT_CMD_APWR = 2,  /**< Auto Increment Write */
    SYN_ECAT_CMD_APRW = 3,  /**< Auto Increment Read Write */
    SYN_ECAT_CMD_FPRD = 4,  /**< Configured Address Read */
    SYN_ECAT_CMD_FPWR = 5,  /**< Configured Address Write */
    SYN_ECAT_CMD_FPRW = 6,  /**< Configured Address Read Write */
    SYN_ECAT_CMD_BRD = 7,   /**< Broadcast Read */
    SYN_ECAT_CMD_BWR = 8,   /**< Broadcast Write */
    SYN_ECAT_CMD_BRW = 9,   /**< Broadcast Read Write */
    SYN_ECAT_CMD_LRD = 10,  /**< Logical Read */
    SYN_ECAT_CMD_LWR = 11,  /**< Logical Write */
    SYN_ECAT_CMD_LRW = 12,  /**< Logical Read Write */
    SYN_ECAT_CMD_ARMW = 13, /**< Auto Increment Read Multiple Write */
    SYN_ECAT_CMD_FRMW = 14  /**< Configured Address Read Multiple Write */
} SYN_EcatCmd;

/* ── EtherCAT State Machine (ESM) States ────────────────────────────────── */

typedef enum {
    SYN_ECAT_STATE_NONE = 0x00,
    SYN_ECAT_STATE_INIT = 0x01,   /**< Init State */
    SYN_ECAT_STATE_PREOP = 0x02,  /**< Pre-Operational State */
    SYN_ECAT_STATE_BOOT = 0x03,   /**< Bootstrap State */
    SYN_ECAT_STATE_SAFEOP = 0x04, /**< Safe-Operational State */
    SYN_ECAT_STATE_OP = 0x08      /**< Operational State */
} SYN_EcatState;

/* ── CoE Mailbox Service Types ──────────────────────────────────────────── */

typedef enum {
    SYN_ECAT_COE_TYPE_EMERGENCY = 1,
    SYN_ECAT_COE_TYPE_SDO_REQ = 2,
    SYN_ECAT_COE_TYPE_SDO_RESP = 3,
    SYN_ECAT_COE_TYPE_RXPDO = 4,
    SYN_ECAT_COE_TYPE_TXPDO = 5,
    SYN_ECAT_COE_TYPE_SDO_INFO = 6
} SYN_EcatCoEType;

/* ── Struct Definitions ─────────────────────────────────────────────────── */

/** @brief Raw EtherCAT Header (2 bytes) */
typedef struct {
    uint16_t length : 11; /**< Length of datagrams in bytes */
    uint16_t reserved : 1;
    uint16_t type : 4; /**< Type (1 for EtherCAT datagrams) */
} SYN_EcatHeader;

/** @brief EtherCAT Datagram Header (10 bytes header + 2 bytes WKC = 12 bytes overhead) */
typedef struct {
    uint8_t cmd;       /**< Command code (SYN_EcatCmd) */
    uint8_t idx;       /**< Index number */
    uint32_t addr;     /**< Address (Auto-inc, Configured, or Logical) */
    uint16_t len : 11; /**< Datagram data length */
    uint16_t res : 3;  /**< Reserved */
    uint16_t circ : 1; /**< Circulating frame flag */
    uint16_t m : 1;    /**< More datagrams flag (1 = another datagram follows) */
    uint16_t irq;      /**< Interrupt request */
    uint16_t wkc;      /**< Working Counter */
} SYN_EcatDatagram;

/** @brief CoE (CANoverEtherCAT) Mailbox Header */
typedef struct {
    uint16_t number : 9;
    uint16_t res : 3;
    uint16_t service_type : 4;
} SYN_EcatCoEHeader;

/** @brief EtherCAT Node Descriptor */
typedef struct {
    SYN_EcatState state;        /**< Current ESM state */
    SYN_EcatState target_state; /**< Requested ESM state */
    uint16_t station_addr;      /**< Configured station address (FPRD/FPWR) */
    uint16_t al_status;         /**< Application Layer Status Code */
    uint16_t wkc_expected;      /**< Expected Working Counter for cyclic exchange */
    uint16_t wkc_last;          /**< Last received Working Counter */
    uint32_t rx_pdos;           /**< Processed RxPDO count */
    uint32_t tx_pdos;           /**< Processed TxPDO count */
    SYN_CANOpenNode *od;        /**< Optional binding to CANopen Object Dictionary node */
} SYN_EcatNode;

/* ── API Functions ──────────────────────────────────────────────────────── */

/**
 * @brief Initialize an EtherCAT Node instance.
 * @param node          EtherCAT node instance.
 * @param station_addr  Configured station address.
 * @param od            Optional CANopen Object Dictionary binding for CoE.
 */
void syn_ecat_init(SYN_EcatNode *node, uint16_t station_addr, SYN_CANOpenNode *od);

/**
 * @brief Build an EtherCAT Frame containing one or more datagrams.
 * @param buf       Output frame buffer (must include Ethernet header space if needed).
 * @param buf_len   Capacity of output buffer.
 * @param datagram  Datagram header descriptor.
 * @param data      Payload bytes to write into datagram.
 * @param data_len  Payload length.
 * @return Total EtherCAT frame length in bytes, or 0 on error.
 */
size_t syn_ecat_build_datagram_frame(uint8_t *buf, size_t buf_len, const SYN_EcatDatagram *datagram,
                                     const uint8_t *data, uint16_t data_len);

/**
 * @brief Parse and process a received raw EtherCAT frame.
 * @param node  EtherCAT node instance.
 * @param frame Raw received frame bytes (after Ethernet header).
 * @param len   Frame length in bytes.
 * @param wkc   [out] Extracted Working Counter sum.
 * @return SYN_OK on successful parsing and WKC validation.
 */
SYN_Status syn_ecat_parse_frame(SYN_EcatNode *node, const uint8_t *frame, size_t len,
                                uint16_t *wkc);

/**
 * @brief Request an EtherCAT State Machine (ESM) state transition.
 * @param node      EtherCAT node instance.
 * @param new_state Requested state (INIT, PREOP, BOOT, SAFEOP, OP).
 * @return SYN_OK if state transition request is valid.
 */
SYN_Status syn_ecat_set_state(SYN_EcatNode *node, SYN_EcatState new_state);

/**
 * @brief Run ESM state machine step.
 * @param node EtherCAT node instance.
 */
void syn_ecat_update(SYN_EcatNode *node);

/**
 * @brief Encode a CoE SDO Download (Write) request into a mailbox buffer.
 * @param buf       Output buffer.
 * @param buf_len   Capacity of output buffer.
 * @param index     Object Dictionary Index (0x0000 - 0xFFFF).
 * @param subindex  Subindex (0x00 - 0xFF).
 * @param data      Data bytes to write.
 * @param data_len  Data length (1-4 bytes for expedited SDO).
 * @return Encoded CoE Mailbox packet length in bytes, or 0 on error.
 */
size_t syn_ecat_coe_encode_sdo_download(uint8_t *buf, size_t buf_len, uint16_t index,
                                        uint8_t subindex, const void *data, size_t data_len);

/**
 * @brief Encode a CoE SDO Upload (Read) request into a mailbox buffer.
 * @param buf       Output buffer.
 * @param buf_len   Capacity of output buffer.
 * @param index     Object Dictionary Index (0x0000 - 0xFFFF).
 * @param subindex  Subindex (0x00 - 0xFF).
 * @return Encoded CoE Mailbox packet length in bytes, or 0 on error.
 */
size_t syn_ecat_coe_encode_sdo_upload(uint8_t *buf, size_t buf_len, uint16_t index,
                                      uint8_t subindex);

#ifdef __cplusplus
}
#endif

#endif /* SYN_ETHERCAT_H */
