/**
 * @file syn_xcp.h
 * @brief ASAM Universal Measurement and Calibration Protocol v1.x (XCP) Slave.
 *
 * Provides a zero-allocation, lightweight XCP v1.x slave protocol stack for
 * ECU calibration, live parameter measurement, memory upload/download, and DAQ
 * (Data Acquisition) list streaming over CAN.
 */

#ifndef SYN_XCP_H
#define SYN_XCP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @name Configuration Constants */
/**@{*/
#ifndef SYN_XCP_MAX_DAQ_LISTS
#define SYN_XCP_MAX_DAQ_LISTS 4U /**< Maximum DAQ list count */
#endif

#ifndef SYN_XCP_MAX_ODT_PER_DAQ
#define SYN_XCP_MAX_ODT_PER_DAQ 8U /**< Maximum ODTs per DAQ list */
#endif

#ifndef SYN_XCP_MAX_ENTRIES_PER_ODT
#define SYN_XCP_MAX_ENTRIES_PER_ODT 7U /**< Maximum entries per ODT */
#endif
/**@}*/

/** @name XCP Command Codes (CTO) */
/**@{*/
#define SYN_XCP_CMD_CONNECT 0xFFU             /**< Connect command */
#define SYN_XCP_CMD_DISCONNECT 0xFEU          /**< Disconnect command */
#define SYN_XCP_CMD_GET_STATUS 0xFDU          /**< Get status command */
#define SYN_XCP_CMD_SYNCH 0xFCU               /**< Synchronize command */
#define SYN_XCP_CMD_GET_COMM_MODE_INFO 0xFBU  /**< Get comm mode info */
#define SYN_XCP_CMD_GET_ID 0xFAU              /**< Get station ID */
#define SYN_XCP_CMD_SET_REQUEST 0xF9U         /**< Set request */
#define SYN_XCP_CMD_GET_SEED 0xF8U            /**< Get seed for unlock */
#define SYN_XCP_CMD_UNLOCK 0xF7U              /**< Unlock resource with key */
#define SYN_XCP_CMD_SET_MTA 0xF6U             /**< Set memory transfer address */
#define SYN_XCP_CMD_UPLOAD 0xF5U              /**< Upload memory block */
#define SYN_XCP_CMD_SHORT_UPLOAD 0xF4U        /**< Short upload */
#define SYN_XCP_CMD_DOWNLOAD 0xF0U            /**< Download memory block */
#define SYN_XCP_CMD_DOWNLOAD_MAX 0xEEU        /**< Fast max download */
#define SYN_XCP_CMD_SET_DAQ_PTR 0xE2U         /**< Set DAQ pointer */
#define SYN_XCP_CMD_WRITE_DAQ 0xE1U           /**< Write DAQ entry */
#define SYN_XCP_CMD_SET_DAQ_LIST_MODE 0xE0U   /**< Set DAQ list mode */
#define SYN_XCP_CMD_START_STOP_DAQ_LIST 0xDEU /**< Start/stop single DAQ list */
#define SYN_XCP_CMD_START_STOP_SYNCH 0xDDU    /**< Start/stop synchronous DAQ */
/**@}*/

/** @name XCP Response Packet Identifiers (CTO Response) */
/**@{*/
#define SYN_XCP_PID_RES 0xFFU  /**< Response packet */
#define SYN_XCP_PID_ERR 0xFEU  /**< Error packet */
#define SYN_XCP_PID_EV 0xFDU   /**< Event packet */
#define SYN_XCP_PID_SERV 0xFCU /**< Service packet */
/**@}*/

/** @name XCP Error Return Codes */
/**@{*/
#define SYN_XCP_ERR_SUCCESS 0x00U         /**< Action succeeded */
#define SYN_XCP_ERR_CMD_SYNTAX 0x20U      /**< Command syntax error */
#define SYN_XCP_ERR_OUT_OF_RANGE 0x22U    /**< Parameter out of range */
#define SYN_XCP_ERR_WRITE_PROTECTED 0x23U /**< Memory write protected */
#define SYN_XCP_ERR_ACCESS_DENIED 0x24U   /**< Access denied */
#define SYN_XCP_ERR_NOT_CONNECTED 0x28U   /**< Station not connected */
#define SYN_XCP_ERR_CMD_UNKNOWN 0x30U     /**< Command unknown */
/**@}*/

/** @name XCP Resource Protection Masks */
/**@{*/
#define SYN_XCP_RESOURCE_CAL_PAG 0x01U /**< Calibration/paging resource mask */
#define SYN_XCP_RESOURCE_DAQ 0x04U     /**< Data acquisition resource mask */
#define SYN_XCP_RESOURCE_STIM 0x08U    /**< Data stimulation resource mask */
#define SYN_XCP_RESOURCE_PGM 0x10U     /**< Flash programming resource mask */
/**@}*/

/**
 * @brief ODT Element mapping entry.
 */
typedef struct {
    uintptr_t address; /**< Target memory address */
    uint8_t size;      /**< Byte count (1, 2, 4 bytes) */
} SYN_XCP_ODTEntry;

/**
 * @brief ODT (Object Descriptor Table) structure.
 */
typedef struct {
    SYN_XCP_ODTEntry entries[SYN_XCP_MAX_ENTRIES_PER_ODT]; /**< ODT entries array */
    uint8_t entry_count;                                   /**< Active entry count */
} SYN_XCP_ODT;

/**
 * @brief DAQ List structure.
 */
typedef struct {
    SYN_XCP_ODT odts[SYN_XCP_MAX_ODT_PER_DAQ]; /**< Array of ODT blocks */
    uint8_t odt_count;                         /**< Configured ODT count */
    uint8_t current_odt_idx;                   /**< Current ODT index */
    uint8_t current_entry_idx;                 /**< Current entry index */
    bool running;                              /**< DAQ list running state */
    uint8_t mode;                              /**< DAQ list mode byte */
    uint8_t event_channel;                     /**< Event channel assignment */
    uint16_t prescaler;                        /**< Transmission prescaler */
    uint16_t cycle_counter;                    /**< Cycle counter */
} SYN_XCP_DAQList;

/**
 * @brief XCP Slave instance context.
 */
typedef struct {
    uint16_t station_id;                              /**< XCP station ID */
    bool connected;                                   /**< Connection state */
    uint8_t unlocked_resources;                       /**< Unlocked resources bitmask */
    uint8_t mta_ext;                                  /**< Memory transfer address extension */
    uintptr_t mta_addr;                               /**< Memory transfer target address */
    uint8_t current_daq_ptr_list;                     /**< Active DAQ pointer list index */
    uint8_t current_daq_ptr_odt;                      /**< Active DAQ pointer ODT index */
    uint8_t current_daq_ptr_idx;                      /**< Active DAQ pointer entry index */
    SYN_XCP_DAQList daq_lists[SYN_XCP_MAX_DAQ_LISTS]; /**< Configured DAQ lists array */
} SYN_XCP_Slave;

/**
 * @brief Initialize XCP slave instance context.
 *
 * @param slave Pointer to XCP slave structure.
 * @param station_id XCP slave node/station ID.
 * @return true on success, false if slave is NULL.
 */
bool syn_xcp_init(SYN_XCP_Slave *slave, uint16_t station_id);

/**
 * @brief Set XCP Memory Transfer Address (MTA).
 *
 * @param slave Pointer to XCP slave structure.
 * @param ext Address extension byte.
 * @param addr 32-bit/64-bit target memory pointer.
 * @return true on success, false if slave is NULL.
 */
bool syn_xcp_set_mta(SYN_XCP_Slave *slave, uint8_t ext, uintptr_t addr);

/**
 * @brief Process incoming XCP CTO command packet and prepare response.
 *
 * @param slave Pointer to XCP slave structure.
 * @param cto 8-byte input CTO CAN frame buffer.
 * @param dto_out 8-byte output response DTO/CRM frame buffer.
 * @return true if response frame populated, false on invalid parameters.
 */
bool syn_xcp_process_cto(SYN_XCP_Slave *slave, const uint8_t cto[8], uint8_t dto_out[8]);

/**
 * @brief Service periodic XCP DAQ sampling event channel tick.
 *
 * @param slave Pointer to XCP slave structure.
 * @param event_channel Event channel index triggered.
 * @param dto_out 8-byte output CAN telemetry buffer.
 * @param list_idx In/Out pointer to DAQ list state iterator.
 * @param odt_idx In/Out pointer to ODT state iterator.
 * @return true if a DAQ DTO packet was generated, false if no packet pending.
 */
bool syn_xcp_service_daq(SYN_XCP_Slave *slave, uint8_t event_channel, uint8_t dto_out[8],
                         uint8_t *list_idx, uint8_t *odt_idx);

#ifdef __cplusplus
}
#endif

#endif /* SYN_XCP_H */
