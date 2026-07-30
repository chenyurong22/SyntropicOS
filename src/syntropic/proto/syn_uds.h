/**
 * @file syn_uds.h
 * @brief ISO 14229 Unified Diagnostic Services (UDS) Server Implementation.
 *
 * Provides a zero-allocation, lightweight UDS server protocol stack for
 * automotive diagnostic session management, Data Identifier (DID) read/write,
 * security access seed/key unlocks, routine controls, and ECU reset over ISO-TP.
 */

#ifndef SYN_UDS_H
#define SYN_UDS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Configuration constants */
#ifndef SYN_UDS_MAX_DIDS
#define SYN_UDS_MAX_DIDS 16U
#endif

/* UDS Service Identifiers (SID) */
#define SYN_UDS_SID_DIAGNOSTIC_SESSION_CONTROL 0x10U
#define SYN_UDS_SID_ECU_RESET 0x11U
#define SYN_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION 0x14U
#define SYN_UDS_SID_READ_DTC_INFORMATION 0x19U
#define SYN_UDS_SID_READ_DATA_BY_IDENTIFIER 0x22U
#define SYN_UDS_SID_READ_MEMORY_BY_ADDRESS 0x23U
#define SYN_UDS_SID_READ_SCALING_DATA_BY_IDENTIFIER 0x24U
#define SYN_UDS_SID_SECURITY_ACCESS 0x27U
#define SYN_UDS_SID_COMMUNICATION_CONTROL 0x28U
#define SYN_UDS_SID_AUTHENTICATION 0x29U
#define SYN_UDS_SID_READ_DATA_BY_PERIODIC_IDENTIFIER 0x2AU
#define SYN_UDS_SID_DYNAMICALLY_DEFINE_DATA_IDENTIFIER 0x2CU
#define SYN_UDS_SID_WRITE_DATA_BY_IDENTIFIER 0x2EU
#define SYN_UDS_SID_INPUT_OUTPUT_CONTROL_BY_IDENTIFIER 0x2FU
#define SYN_UDS_SID_ROUTINE_CONTROL 0x31U
#define SYN_UDS_SID_REQUEST_DOWNLOAD 0x34U
#define SYN_UDS_SID_REQUEST_UPLOAD 0x35U
#define SYN_UDS_SID_TRANSFER_DATA 0x36U
#define SYN_UDS_SID_REQUEST_TRANSFER_EXIT 0x37U
#define SYN_UDS_SID_REQUEST_FILE_TRANSFER 0x38U
#define SYN_UDS_SID_WRITE_MEMORY_BY_ADDRESS 0x3DU
#define SYN_UDS_SID_TESTER_PRESENT 0x3EU
#define SYN_UDS_SID_ACCESS_TIMING_PARAMETER 0x83U
#define SYN_UDS_SID_SECURED_DATA_TRANSMISSION 0x84U
#define SYN_UDS_SID_CONTROL_DTC_SETTING 0x85U
#define SYN_UDS_SID_RESPONSE_ON_EVENT 0x86U
#define SYN_UDS_SID_LINK_CONTROL 0x87U

/* UDS Service 0x19 ReadDTCInformation Sub-functions */
#define SYN_UDS_DTC_REPORT_NUMBER_BY_STATUS_MASK 0x01U
#define SYN_UDS_DTC_REPORT_BY_STATUS_MASK 0x02U
#define SYN_UDS_DTC_REPORT_SNAPSHOT_IDENTIFICATION 0x03U
#define SYN_UDS_DTC_REPORT_SNAPSHOT_RECORD_BY_DTC 0x04U
#define SYN_UDS_DTC_REPORT_STORED_DATA_BY_RECORD_NUM 0x05U
#define SYN_UDS_DTC_REPORT_EXT_DATA_RECORD_BY_DTC 0x06U
#define SYN_UDS_DTC_REPORT_NUMBER_BY_SEVERITY_MASK 0x07U
#define SYN_UDS_DTC_REPORT_BY_SEVERITY_MASK 0x08U
#define SYN_UDS_DTC_REPORT_SEVERITY_INFO 0x09U
#define SYN_UDS_DTC_REPORT_SUPPORTED 0x0AU
#define SYN_UDS_DTC_REPORT_FIRST_TEST_FAILED 0x0BU
#define SYN_UDS_DTC_REPORT_FIRST_CONFIRMED 0x0CU
#define SYN_UDS_DTC_REPORT_MOST_RECENT_TEST_FAILED 0x0DU
#define SYN_UDS_DTC_REPORT_MOST_RECENT_CONFIRMED 0x0EU
#define SYN_UDS_DTC_REPORT_MIRROR_MEMORY_BY_STATUS_MASK 0x0FU
#define SYN_UDS_DTC_REPORT_MIRROR_MEMORY_EXT_DATA 0x10U
#define SYN_UDS_DTC_REPORT_NUMBER_MIRROR_MEMORY_BY_STATUS_MASK 0x11U
#define SYN_UDS_DTC_REPORT_NUMBER_EMISSIONS_OBD_BY_STATUS_MASK 0x12U
#define SYN_UDS_DTC_REPORT_EMISSIONS_OBD_BY_STATUS_MASK 0x13U
#define SYN_UDS_DTC_REPORT_FAULT_DETECTION_COUNTER 0x14U
#define SYN_UDS_DTC_REPORT_WITH_PERMANENT_STATUS 0x15U
#define SYN_UDS_DTC_REPORT_EXT_DATA_RECORD_BY_RECORD_NUM 0x16U
#define SYN_UDS_DTC_REPORT_USER_DEF_MEMORY_BY_STATUS_MASK 0x17U
#define SYN_UDS_DTC_REPORT_USER_DEF_MEMORY_SNAPSHOT_BY_DTC 0x18U
#define SYN_UDS_DTC_REPORT_USER_DEF_MEMORY_EXT_DATA_BY_DTC 0x19U
#define SYN_UDS_DTC_REPORT_WWH_OBD_BY_MASK_RECORD 0x42U
#define SYN_UDS_DTC_REPORT_WWH_OBD_WITH_PERMANENT_STATUS 0x55U

/* UDS GroupOfDTC definitions (ISO 14229-1 / SAE J2012) */
#define SYN_UDS_DTC_GROUP_POWERTRAIN 0x000000U
#define SYN_UDS_DTC_GROUP_CHASSIS 0x400000U
#define SYN_UDS_DTC_GROUP_BODY 0x800000U
#define SYN_UDS_DTC_GROUP_NETWORK 0xC00000U
#define SYN_UDS_DTC_GROUP_ALL 0xFFFFFFU

/* UDS Response Identifiers */
#define SYN_UDS_RESPONSE_NEGATIVE 0x7FU

/* UDS Negative Response Codes (NRC) */
#define SYN_UDS_NRC_SUCCESS 0x00U
#define SYN_UDS_NRC_SERVICE_NOT_SUPPORTED 0x11U
#define SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED 0x12U
#define SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH 0x13U
#define SYN_UDS_NRC_RESPONSE_TOO_LONG 0x14U
#define SYN_UDS_NRC_CONDITIONS_NOT_CORRECT 0x22U
#define SYN_UDS_NRC_REQUEST_OUT_OF_RANGE 0x31U
#define SYN_UDS_NRC_SECURITY_ACCESS_DENIED 0x33U
#define SYN_UDS_NRC_INVALID_KEY 0x35U
#define SYN_UDS_NRC_EXCEEDED_NUMBER_OF_ATTEMPTS 0x36U
#define SYN_UDS_NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED 0x37U
#define SYN_UDS_NRC_REQUEST_SEQUENCE_ERROR 0x24U
#define SYN_UDS_NRC_TRANSFER_DATA_SUSPENDED 0x71U
#define SYN_UDS_NRC_GENERAL_PROGRAMMING_FAILURE 0x72U
#define SYN_UDS_NRC_WRONG_BLOCK_SEQUENCE_COUNTER 0x73U

/* UDS Transfer States */
typedef enum {
    SYN_UDS_TRANSFER_IDLE = 0U,
    SYN_UDS_TRANSFER_DOWNLOAD = 1U,
    SYN_UDS_TRANSFER_UPLOAD = 2U
} SYN_UDS_TransferState;

/* UDS Diagnostic Sessions */
typedef enum {
    SYN_UDS_SESSION_DEFAULT = 0x01U,
    SYN_UDS_SESSION_PROGRAMMING = 0x02U,
    SYN_UDS_SESSION_EXTENDED = 0x03U
} SYN_UDS_Session;

/* UDS Security Access States */
typedef enum {
    SYN_UDS_SECURITY_LOCKED = 0x00U,
    SYN_UDS_SECURITY_SEED_SENT = 0x01U,
    SYN_UDS_SECURITY_UNLOCKED = 0x02U
} SYN_UDS_SecurityState;

/* UDS CommunicationControl (0x28) Subfunctions */
typedef enum {
    SYN_UDS_COMM_ENABLE_RX_AND_TX = 0x00U,
    SYN_UDS_COMM_ENABLE_RX_DISABLE_TX = 0x01U,
    SYN_UDS_COMM_DISABLE_RX_ENABLE_TX = 0x02U,
    SYN_UDS_COMM_DISABLE_RX_AND_TX = 0x03U,
    SYN_UDS_COMM_ENABLE_RX_TX_ENHANCED = 0x04U,
    SYN_UDS_COMM_ENABLE_RX_ENHANCED = 0x05U
} SYN_UDS_CommControlType;

/* UDS AccessTimingParameter (0x83) Subfunctions */
typedef enum {
    SYN_UDS_TIMING_READ_EXTENDED = 0x01U,
    SYN_UDS_TIMING_SET_TO_DEFAULT = 0x02U,
    SYN_UDS_TIMING_READ_ACTIVE = 0x03U,
    SYN_UDS_TIMING_SET_TO_GIVEN = 0x04U
} SYN_UDS_AccessTimingType;

/**
 * @brief CommunicationControl (0x28) callback function signature.
 */
typedef bool (*SYN_UDS_CommControlHandler)(SYN_UDS_CommControlType control_type, uint8_t comm_type,
                                           void *ctx);

/**
 * @brief AccessTimingParameter (0x83) callback function signature.
 */
typedef bool (*SYN_UDS_AccessTimingHandler)(SYN_UDS_AccessTimingType timing_type,
                                            uint16_t *p2_max_ms, uint16_t *p2_star_max_10ms,
                                            void *ctx);

/**
 * @brief SecuredDataTransmission (0x84) callback function signature.
 */
typedef bool (*SYN_UDS_SecuredDataHandler)(const uint8_t *in_data, uint16_t in_len,
                                           uint8_t *out_buf, uint16_t max_out_len,
                                           uint16_t *out_len, void *ctx);

/**
 * @brief ReadMemoryByAddress (0x23) and WriteMemoryByAddress (0x3D) callback function signature.
 */
typedef bool (*SYN_UDS_MemoryHandler)(bool is_write, uint32_t address, uint32_t size,
                                      uint8_t *data_buf, void *ctx);

/**
 * @brief Authentication (0x29) callback function signature.
 */
typedef bool (*SYN_UDS_AuthHandler)(uint8_t subfunction, const uint8_t *in_data, uint16_t in_len,
                                    uint8_t *out_buf, uint16_t max_out_len, uint16_t *out_len,
                                    void *ctx);

/**
 * @brief RequestFileTransfer (0x38) callback function signature.
 */
typedef bool (*SYN_UDS_FileTransferHandler)(uint8_t mode, const char *file_path, uint16_t path_len,
                                            uint8_t *out_buf, uint16_t max_out_len,
                                            uint16_t *out_len, void *ctx);

/**
 * @brief Data Identifier (DID) Registry Entry.
 */
typedef struct {
    uint16_t did;
    uint8_t *data;
    uint16_t len;
    bool writable;
} SYN_UDS_DIDEntry;

/* UDS Timing & Security Constants */
#ifndef SYN_UDS_MAX_DTCS
#define SYN_UDS_MAX_DTCS 32U
#endif

#define SYN_UDS_DTC_STATUS_AVAILABILITY_MASK 0xFFU

/* ISO 14229-1 DTCStatusByte Bit Definitions */
#define SYN_UDS_DTC_STATUS_TEST_FAILED (1U << 0)
#define SYN_UDS_DTC_STATUS_TEST_FAILED_THIS_OP_CYCLE (1U << 1)
#define SYN_UDS_DTC_STATUS_PENDING_DTC (1U << 2)
#define SYN_UDS_DTC_STATUS_CONFIRMED_DTC (1U << 3)
#define SYN_UDS_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR (1U << 4)
#define SYN_UDS_DTC_STATUS_TEST_FAILED_SINCE_LAST_CLEAR (1U << 5)
#define SYN_UDS_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OP_CYCLE (1U << 6)
#define SYN_UDS_DTC_STATUS_WARNING_INDICATOR_REQUESTED (1U << 7)

/* ISO 14229-1 DTCSeverityByte Definitions (Bits 7..5) */
#define SYN_UDS_DTC_SEVERITY_NO_SEVERITY (0x00U << 5)
#define SYN_UDS_DTC_SEVERITY_MAINTENANCE_REQUIRED (0x01U << 5)
#define SYN_UDS_DTC_SEVERITY_CHECK_AT_NEXT_STOP (0x02U << 5)
#define SYN_UDS_DTC_SEVERITY_CHECK_IMMEDIATELY (0x04U << 5)
#define SYN_UDS_DTC_SEVERITY_MASK (0x07U << 5)
#define SYN_UDS_DTC_CLASS_MASK (0x1FU)

/* ISO 14229-1 DTCFormatIdentifier Definitions */
#define SYN_UDS_DTC_FORMAT_ISO14229_1 0x00U
#define SYN_UDS_DTC_FORMAT_ISO15031_6 0x01U
#define SYN_UDS_DTC_FORMAT_SAE_J1939_73 0x02U
#define SYN_UDS_DTC_FORMAT_ISO27145_4 0x03U

/**
 * @brief Diagnostic Trouble Code (DTC) Registry Entry.
 */
typedef struct {
    uint32_t dtc;     /**< 24-bit Diagnostic Trouble Code (e.g., 0x012345). */
    uint8_t status;   /**< DTCStatusByte bitmask.                           */
    uint8_t severity; /**< DTCSeverityByte (High 3 bits severity, low 5 bits class). */
    int8_t fault_cnt; /**< Fault detection counter (-128 to 127).          */
} SYN_UDS_DTCEntry;

/**
 * @brief ReadDTCInformation (0x19) callback function signature.
 */
typedef bool (*SYN_UDS_DTCHandler)(uint8_t subfunction, const uint8_t *in_data, uint16_t in_len,
                                   uint8_t *out_buf, uint16_t max_out_len, uint16_t *out_len,
                                   void *ctx);

#ifndef SYN_UDS_S3_TIMEOUT_MS
#define SYN_UDS_S3_TIMEOUT_MS 5000U
#endif

#ifndef SYN_UDS_SECURITY_MAX_ATTEMPTS
#define SYN_UDS_SECURITY_MAX_ATTEMPTS 3U
#endif

#ifndef SYN_UDS_SECURITY_DELAY_MS
#define SYN_UDS_SECURITY_DELAY_MS 10000U
#endif

/**
 * @brief UDS Server Instance Context.
 */
typedef struct {
    SYN_UDS_Session session;
    SYN_UDS_SecurityState security_state;
    uint32_t current_seed;
    uint32_t s3_timer_ms;
    uint8_t security_error_count;
    uint32_t security_delay_timer_ms;
    SYN_UDS_CommControlType comm_control_state;
    uint8_t comm_type;
    SYN_UDS_CommControlHandler comm_control_cb;
    void *comm_control_ctx;
    uint16_t p2_max_ms;                           /**< Default P2Server_max timing (ms). */
    uint16_t p2_star_max_10ms;                    /**< Default P2*Server_max timing (10ms units). */
    uint16_t active_p2_max_ms;                    /**< Active P2Server_max timing (ms). */
    uint16_t active_p2_star_max_10ms;             /**< Active P2*Server_max timing (10ms units). */
    SYN_UDS_AccessTimingHandler timing_cb;        /**< AccessTimingParameter callback. */
    void *timing_ctx;                             /**< Context pointer for timing callback. */
    SYN_UDS_SecuredDataHandler secured_data_cb;   /**< SecuredDataTransmission callback. */
    void *secured_data_ctx;                       /**< Context pointer for secured data callback. */
    SYN_UDS_MemoryHandler memory_cb;              /**< Memory (0x23/0x3D) callback. */
    void *memory_ctx;                             /**< Context pointer for memory callback. */
    SYN_UDS_AuthHandler auth_cb;                  /**< Authentication (0x29) callback. */
    void *auth_ctx;                               /**< Context pointer for auth callback. */
    SYN_UDS_FileTransferHandler file_transfer_cb; /**< File transfer (0x38) callback. */
    void *file_transfer_ctx;   /**< Context pointer for file transfer callback. */
    SYN_UDS_DTCHandler dtc_cb; /**< ReadDTCInformation (0x19) callback. */
    void *dtc_ctx;             /**< Context pointer for DTC callback. */
    SYN_UDS_DIDEntry did_table[SYN_UDS_MAX_DIDS];
    uint8_t did_count;
    SYN_UDS_DTCEntry dtc_table[SYN_UDS_MAX_DTCS];
    uint8_t dtc_count;
    SYN_UDS_TransferState transfer_state;
    uint32_t transfer_address;
    uint32_t transfer_size;
    uint32_t transfer_bytes_processed;
    uint8_t expected_block_seq;
    uint8_t reset_type_requested;
} SYN_UDS_Server;

/**
 * @brief Initialize UDS Server context.
 *
 * @param server Pointer to UDS server instance.
 * @return true on success, false if server is NULL.
 */
bool syn_uds_init(SYN_UDS_Server *server);

/**
 * @brief Advance periodic S3 server timer by dt_ms milliseconds.
 * Reverts session to DEFAULT and locks security state if S3 timer expires (5000ms).
 *
 * @param server Pointer to UDS server instance.
 * @param dt_ms Milliseconds elapsed since last tick.
 */
void syn_uds_tick(SYN_UDS_Server *server, uint32_t dt_ms);

/**
 * @brief Get pending ECU reset sub-function requested by 0x11 service.
 *
 * @param server Pointer to UDS server instance.
 * @return Pending reset type (e.g. 0x01 = hard, 0x02 = keyOffOn, 0x03 = soft), or 0 if none.
 */
uint8_t syn_uds_get_pending_reset(const SYN_UDS_Server *server);

/**
 * @brief Clear pending ECU reset flag after executing reset or transmitting response.
 *
 * @param server Pointer to UDS server instance.
 */
void syn_uds_clear_pending_reset(SYN_UDS_Server *server);

/**
 * @brief Register Data Identifier (DID) mapping in UDS Server table.
 *
 * @param server Pointer to UDS server instance.
 * @param did 16-bit Data Identifier code (e.g., 0xF190 for VIN).
 * @param data Pointer to data memory buffer.
 * @param len Data byte length.
 * @param writable True if DID allows WriteDataByIdentifier (0x2E).
 * @return true on success, false if table full or invalid params.
 */
bool syn_uds_register_did(SYN_UDS_Server *server, uint16_t did, uint8_t *data, uint16_t len,
                          bool writable);

/**
 * @brief Register Diagnostic Trouble Code (DTC) in UDS Server table.
 *
 * @param server Pointer to UDS server instance.
 * @param dtc 24-bit Diagnostic Trouble Code (e.g., 0x012345).
 * @param status Initial DTCStatusByte bitmask.
 * @param severity DTCSeverityByte (severity & class).
 * @return true on success, false if table full or invalid params.
 */
bool syn_uds_register_dtc(SYN_UDS_Server *server, uint32_t dtc, uint8_t status, uint8_t severity);

/**
 * @brief Register ReadDTCInformation (0x19) callback handler.
 *
 * @param server Pointer to UDS server instance.
 * @param handler Callback function invoked for custom DTC subfunctions.
 * @param ctx User context passed to handler.
 * @return true on success, false if server is NULL.
 */
bool syn_uds_register_dtc_handler(SYN_UDS_Server *server, SYN_UDS_DTCHandler handler, void *ctx);

/**
 * @brief Register CommunicationControl (0x28) callback handler.
 *
 * @param server  Pointer to UDS server instance.
 * @param handler Callback function invoked when Service 0x28 is processed.
 * @param ctx     User context passed to handler.
 * @return true on success, false if server is NULL.
 */
bool syn_uds_register_comm_control(SYN_UDS_Server *server, SYN_UDS_CommControlHandler handler,
                                   void *ctx);

/**
 * @brief Register AccessTimingParameter (0x83) callback handler.
 *
 * @param server  Pointer to UDS server instance.
 * @param handler Callback function invoked when Service 0x83 is processed.
 * @param ctx     User context passed to handler.
 * @return true on success, false if server is NULL.
 */
bool syn_uds_register_access_timing(SYN_UDS_Server *server, SYN_UDS_AccessTimingHandler handler,
                                    void *ctx);

/**
 * @brief Register SecuredDataTransmission (0x84) callback handler.
 *
 * @param server  Pointer to UDS server instance.
 * @param handler Callback function invoked when Service 0x84 is processed.
 * @param ctx     User context passed to handler.
 * @return true on success, false if server is NULL.
 */
bool syn_uds_register_secured_data(SYN_UDS_Server *server, SYN_UDS_SecuredDataHandler handler,
                                   void *ctx);

/**
 * @brief Register ReadMemoryByAddress (0x23) and WriteMemoryByAddress (0x3D) handler.
 *
 * @param server  Pointer to UDS server instance.
 * @param handler Callback function invoked when memory services are processed.
 * @param ctx     User context passed to handler.
 * @return true on success, false if server is NULL.
 */
bool syn_uds_register_memory_handler(SYN_UDS_Server *server, SYN_UDS_MemoryHandler handler,
                                     void *ctx);

/**
 * @brief Register Authentication (0x29) handler.
 *
 * @param server  Pointer to UDS server instance.
 * @param handler Callback function invoked when Service 0x29 is processed.
 * @param ctx     User context passed to handler.
 * @return true on success, false if server is NULL.
 */
bool syn_uds_register_auth_handler(SYN_UDS_Server *server, SYN_UDS_AuthHandler handler, void *ctx);

/**
 * @brief Register RequestFileTransfer (0x38) handler.
 *
 * @param server  Pointer to UDS server instance.
 * @param handler Callback function invoked when Service 0x38 is processed.
 * @param ctx     User context passed to handler.
 * @return true on success, false if server is NULL.
 */
bool syn_uds_register_file_transfer(SYN_UDS_Server *server, SYN_UDS_FileTransferHandler handler,
                                    void *ctx);

/**
 * @brief Process incoming UDS request diagnostic payload and format response.
 *
 * @param server Pointer to UDS server instance.
 * @param req Pointer to input request bytes.
 * @param req_len Length of request payload in bytes.
 * @param resp_buf Output buffer for response bytes.
 * @param max_resp_len Capacity of output response buffer.
 * @param resp_len Pointer to store generated response byte length.
 * @return true if response frame generated, false on invalid parameters.
 */
bool syn_uds_process_request(SYN_UDS_Server *server, const uint8_t *req, uint16_t req_len,
                             uint8_t *resp_buf, uint16_t max_resp_len, uint16_t *resp_len);

/**
 * @brief Report diagnostic test result for a registered DTC according to ISO 14229-1 state
 * transitions.
 *
 * Updates DTC status bits (testFailed, testFailedThisOperationCycle, pendingDTC, confirmedDTC,
 * etc.) and fault detection counter.
 *
 * @param server Pointer to UDS server instance.
 * @param dtc 24-bit Diagnostic Trouble Code.
 * @param failed True if test failed, false if test passed.
 * @return true if DTC found and updated, false otherwise.
 */
bool syn_uds_dtc_report_test_result(SYN_UDS_Server *server, uint32_t dtc, bool failed);

/**
 * @brief Advance server state to a new operation cycle.
 * Clears testFailedThisOperationCycle and sets testNotCompletedThisOperationCycle for all
 * registered DTCs.
 *
 * @param server Pointer to UDS server instance.
 * @return true on success, false if server is NULL.
 */
bool syn_uds_dtc_start_operation_cycle(SYN_UDS_Server *server);

/**
 * @brief Get current 8-bit status byte for a registered DTC.
 *
 * @param server Pointer to UDS server instance.
 * @param dtc 24-bit Diagnostic Trouble Code.
 * @param out_status Output pointer to store DTC status byte.
 * @return true if DTC found, false otherwise.
 */
bool syn_uds_dtc_get_status(SYN_UDS_Server *server, uint32_t dtc, uint8_t *out_status);

#ifdef __cplusplus
}
#endif

#endif /* SYN_UDS_H */
