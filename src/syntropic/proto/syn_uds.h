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
#define SYN_UDS_SID_SECURITY_ACCESS 0x27U
#define SYN_UDS_SID_COMMUNICATION_CONTROL 0x28U
#define SYN_UDS_SID_WRITE_DATA_BY_IDENTIFIER 0x2EU
#define SYN_UDS_SID_ROUTINE_CONTROL 0x31U
#define SYN_UDS_SID_REQUEST_DOWNLOAD 0x34U
#define SYN_UDS_SID_REQUEST_UPLOAD 0x35U
#define SYN_UDS_SID_TRANSFER_DATA 0x36U
#define SYN_UDS_SID_REQUEST_TRANSFER_EXIT 0x37U
#define SYN_UDS_SID_TESTER_PRESENT 0x3EU
#define SYN_UDS_SID_ACCESS_TIMING_PARAMETER 0x83U
#define SYN_UDS_SID_SECURED_DATA_TRANSMISSION 0x84U
#define SYN_UDS_SID_CONTROL_DTC_SETTING 0x85U
#define SYN_UDS_SID_RESPONSE_ON_EVENT 0x86U

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
 * @brief Data Identifier (DID) Registry Entry.
 */
typedef struct {
    uint16_t did;
    uint8_t *data;
    uint16_t len;
    bool writable;
} SYN_UDS_DIDEntry;

/* UDS Timing & Security Constants */
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
    uint16_t p2_max_ms;                         /**< Default P2Server_max timing (ms). */
    uint16_t p2_star_max_10ms;                  /**< Default P2*Server_max timing (10ms units). */
    uint16_t active_p2_max_ms;                  /**< Active P2Server_max timing (ms). */
    uint16_t active_p2_star_max_10ms;           /**< Active P2*Server_max timing (10ms units). */
    SYN_UDS_AccessTimingHandler timing_cb;      /**< AccessTimingParameter callback. */
    void *timing_ctx;                           /**< Context pointer for timing callback. */
    SYN_UDS_SecuredDataHandler secured_data_cb; /**< SecuredDataTransmission callback. */
    void *secured_data_ctx;                     /**< Context pointer for secured data callback. */
    SYN_UDS_DIDEntry did_table[SYN_UDS_MAX_DIDS];
    uint8_t did_count;
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

#ifdef __cplusplus
}
#endif

#endif /* SYN_UDS_H */
