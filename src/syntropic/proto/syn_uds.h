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
#define SYN_UDS_SID_READ_DATA_BY_IDENTIFIER 0x22U
#define SYN_UDS_SID_SECURITY_ACCESS 0x27U
#define SYN_UDS_SID_WRITE_DATA_BY_IDENTIFIER 0x2EU
#define SYN_UDS_SID_ROUTINE_CONTROL 0x31U
#define SYN_UDS_SID_TESTER_PRESENT 0x3EU

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

/**
 * @brief Data Identifier (DID) Registry Entry.
 */
typedef struct {
    uint16_t did;
    uint8_t *data;
    uint16_t len;
    bool writable;
} SYN_UDS_DIDEntry;

/* UDS Timing Constants */
#ifndef SYN_UDS_S3_TIMEOUT_MS
#define SYN_UDS_S3_TIMEOUT_MS 5000U
#endif

/**
 * @brief UDS Server Instance Context.
 */
typedef struct {
    SYN_UDS_Session session;
    SYN_UDS_SecurityState security_state;
    uint32_t current_seed;
    uint32_t s3_timer_ms;
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
