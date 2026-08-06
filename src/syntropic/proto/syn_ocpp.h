/**
 * @file syn_ocpp.h
 * @brief Open Charge Point Protocol over JSON (OCPP-J 1.6 / 2.0.1) Client Engine.
 * @ingroup syn_proto
 *
 * Implements a lightweight, zero-allocation OCPP-J client protocol engine for EVSE
 * charging stations. Supports OCPP-J frame types (Call, CallResult, CallError) and
 * core charging station operations: BootNotification, Heartbeat, StatusNotification,
 * Authorize, StartTransaction, StopTransaction, MeterValues, and RemoteStart/Stop.
 */

#ifndef SYN_OCPP_H
#define SYN_OCPP_H

#include "../common/syn_defs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @name OCPP-J Message Type Identifiers */
/**@{*/
#define SYN_OCPP_MSG_TYPE_CALL 2U       /**< Client/Server Request Call */
#define SYN_OCPP_MSG_TYPE_CALLRESULT 3U /**< Success Response */
#define SYN_OCPP_MSG_TYPE_CALLERROR 4U  /**< Error Response */
/**@}*/

/** @brief OCPP Connector Status Enumeration (OCPP 1.6 / 2.0.1). */
typedef enum {
    SYN_OCPP_STATUS_AVAILABLE = 0,  /**< Connector available for charging */
    SYN_OCPP_STATUS_PREPARING,      /**< EV connected, preparing session */
    SYN_OCPP_STATUS_CHARGING,       /**< Active charging session */
    SYN_OCPP_STATUS_SUSPENDED_EV,   /**< Suspended by EV */
    SYN_OCPP_STATUS_SUSPENDED_EVSE, /**< Suspended by EVSE station */
    SYN_OCPP_STATUS_FINISHING,      /**< Session ending */
    SYN_OCPP_STATUS_RESERVED,       /**< Reserved connector */
    SYN_OCPP_STATUS_UNAVAILABLE,    /**< Connector unavailable */
    SYN_OCPP_STATUS_FAULTED         /**< Fault condition */
} SYN_OCPP_ChargePointStatus;

/** @brief OCPP Registration Status Enumeration. */
typedef enum {
    SYN_OCPP_REGISTRATION_ACCEPTED = 0, /**< Registration accepted */
    SYN_OCPP_REGISTRATION_PENDING,      /**< Registration pending */
    SYN_OCPP_REGISTRATION_REJECTED      /**< Registration rejected */
} SYN_OCPP_RegistrationStatus;

/** @brief OCPP Authorization Status Enumeration. */
typedef enum {
    SYN_OCPP_AUTH_ACCEPTED = 0, /**< IdTag authorized */
    SYN_OCPP_AUTH_BLOCKED,      /**< IdTag blocked */
    SYN_OCPP_AUTH_EXPIRED,      /**< IdTag expired */
    SYN_OCPP_AUTH_INVALID       /**< IdTag invalid */
} SYN_OCPP_AuthorizationStatus;

/** @brief Charge Point information for BootNotification. */
typedef struct {
    const char *charge_point_vendor; /**< Vendor identifier string */
    const char *charge_point_model;  /**< Model identifier string */
    const char *serial_number;       /**< Charge point serial number */
    const char *firmware_version;    /**< Firmware version string */
} SYN_OCPP_ChargePointInfo;

/** @brief Meter Values reading structure. */
typedef struct {
    uint32_t energy_wh;  /**< Total active energy in Wh */
    uint16_t voltage_v;  /**< RMS Voltage in V */
    uint16_t current_a;  /**< RMS Current in A (deci-Amperes, 0.1A) */
    uint16_t power_kw;   /**< Active Power in W */
    uint8_t soc_percent; /**< State of Charge (0..100%) */
} SYN_OCPP_MeterValues;

/** @brief Remote Start Transaction handler callback. */
typedef bool (*SYN_OCPP_RemoteStartHandler)(uint32_t connector_id, const char *id_tag,
                                            void *user_ctx);

/** @brief Remote Stop Transaction handler callback. */
typedef bool (*SYN_OCPP_RemoteStopHandler)(int32_t transaction_id, void *user_ctx);

/** @brief Registration response callback. */
typedef void (*SYN_OCPP_RegistrationHandler)(SYN_OCPP_RegistrationStatus status,
                                             uint32_t heartbeat_interval_sec, void *user_ctx);

/** @brief Authorization response callback. */
typedef void (*SYN_OCPP_AuthorizationHandler)(const char *id_tag,
                                              SYN_OCPP_AuthorizationStatus status, void *user_ctx);

/** @brief Start Transaction response callback. */
typedef void (*SYN_OCPP_StartTxHandler)(int32_t transaction_id, SYN_OCPP_AuthorizationStatus status,
                                        void *user_ctx);

/** @brief OCPP Client instance state. */
typedef struct {
    SYN_OCPP_RegistrationStatus registration_status; /**< Central System registration state */
    uint32_t heartbeat_interval_sec;                 /**< Heartbeat interval in seconds */
    uint32_t heartbeat_timer_ms;                     /**< Heartbeat countdown timer ms */

    uint32_t connector_id;                       /**< Active connector ID (e.g. 1) */
    SYN_OCPP_ChargePointStatus connector_status; /**< Current connector status */

    int32_t active_transaction_id; /**< Active transaction ID (-1 if none) */
    char active_id_tag[32];        /**< Active authorized RFID tag */
    uint32_t tx_start_energy_wh;   /**< Meter Wh at transaction start */

    uint32_t message_counter; /**< Monotonic message ID counter */

    SYN_OCPP_RegistrationHandler reg_cb;   /**< Registration response callback */
    SYN_OCPP_AuthorizationHandler auth_cb; /**< Authorization response callback */
    SYN_OCPP_StartTxHandler start_tx_cb;   /**< StartTx response callback */

    SYN_OCPP_RemoteStartHandler remote_start_cb; /**< RemoteStart command callback */
    SYN_OCPP_RemoteStopHandler remote_stop_cb;   /**< RemoteStop command callback */
    void *user_ctx;                              /**< User context pointer */
} SYN_OCPP_Client;

/* ── API Declarations ────────────────────────────────────────────────── */

/**
 * @brief Initialize an OCPP-J client instance.
 * @param client Pointer to client instance.
 * @return SYN_OK on success, SYN_INVALID_PARAM if client is NULL.
 */
SYN_Status syn_ocpp_init(SYN_OCPP_Client *client);

/**
 * @brief Set event and remote command callbacks for OCPP client.
 * @param client Pointer to client instance.
 * @param reg_cb Registration response callback.
 * @param auth_cb Authorization response callback.
 * @param start_tx_cb StartTransaction response callback.
 * @param remote_start_cb RemoteStart command callback.
 * @param remote_stop_cb RemoteStop command callback.
 * @param user_ctx User context pointer.
 * @return SYN_OK on success, SYN_INVALID_PARAM if client is NULL.
 */
SYN_Status syn_ocpp_set_callbacks(SYN_OCPP_Client *client, SYN_OCPP_RegistrationHandler reg_cb,
                                  SYN_OCPP_AuthorizationHandler auth_cb,
                                  SYN_OCPP_StartTxHandler start_tx_cb,
                                  SYN_OCPP_RemoteStartHandler remote_start_cb,
                                  SYN_OCPP_RemoteStopHandler remote_stop_cb, void *user_ctx);

/**
 * @brief Format an OCPP-J BootNotification.req Call frame.
 * @param client Pointer to client instance.
 * @param info Pointer to charge point vendor/model info.
 * @param out_buf Output text buffer for JSON payload.
 * @param max_len Capacity of output buffer.
 * @param out_len Pointer to store output byte count.
 * @return SYN_OK on success, SYN_INVALID_PARAM or SYN_ERROR on failure.
 */
SYN_Status syn_ocpp_format_boot_notification(SYN_OCPP_Client *client,
                                             const SYN_OCPP_ChargePointInfo *info, char *out_buf,
                                             size_t max_len, size_t *out_len);

/**
 * @brief Format an OCPP-J Heartbeat.req Call frame.
 * @param client Pointer to client instance.
 * @param out_buf Output text buffer for JSON payload.
 * @param max_len Capacity of output buffer.
 * @param out_len Pointer to store output byte count.
 * @return SYN_OK on success, SYN_INVALID_PARAM or SYN_ERROR on failure.
 */
SYN_Status syn_ocpp_format_heartbeat(SYN_OCPP_Client *client, char *out_buf, size_t max_len,
                                     size_t *out_len);

/**
 * @brief Format an OCPP-J StatusNotification.req Call frame.
 * @param client Pointer to client instance.
 * @param connector_id Connector ID (1..N).
 * @param status Connector status enumeration.
 * @param error_code Error code string (e.g. "NoError").
 * @param out_buf Output text buffer for JSON payload.
 * @param max_len Capacity of output buffer.
 * @param out_len Pointer to store output byte count.
 * @return SYN_OK on success, SYN_INVALID_PARAM or SYN_ERROR on failure.
 */
SYN_Status syn_ocpp_format_status_notification(SYN_OCPP_Client *client, uint32_t connector_id,
                                               SYN_OCPP_ChargePointStatus status,
                                               const char *error_code, char *out_buf,
                                               size_t max_len, size_t *out_len);

/**
 * @brief Format an OCPP-J Authorize.req Call frame.
 * @param client Pointer to client instance.
 * @param id_tag RFID tag string.
 * @param out_buf Output text buffer for JSON payload.
 * @param max_len Capacity of output buffer.
 * @param out_len Pointer to store output byte count.
 * @return SYN_OK on success, SYN_INVALID_PARAM or SYN_ERROR on failure.
 */
SYN_Status syn_ocpp_format_authorize(SYN_OCPP_Client *client, const char *id_tag, char *out_buf,
                                     size_t max_len, size_t *out_len);

/**
 * @brief Format an OCPP-J StartTransaction.req Call frame.
 * @param client Pointer to client instance.
 * @param connector_id Connector ID.
 * @param id_tag Authorized RFID tag string.
 * @param meter_start_wh Current meter reading in Wh.
 * @param out_buf Output text buffer for JSON payload.
 * @param max_len Capacity of output buffer.
 * @param out_len Pointer to store output byte count.
 * @return SYN_OK on success, SYN_INVALID_PARAM or SYN_ERROR on failure.
 */
SYN_Status syn_ocpp_format_start_transaction(SYN_OCPP_Client *client, uint32_t connector_id,
                                             const char *id_tag, uint32_t meter_start_wh,
                                             char *out_buf, size_t max_len, size_t *out_len);

/**
 * @brief Format an OCPP-J StopTransaction.req Call frame.
 * @param client Pointer to client instance.
 * @param transaction_id Active transaction ID.
 * @param meter_stop_wh Meter reading in Wh at transaction stop.
 * @param reason Reason string (e.g. "EVDisconnected", "Local", "Remote").
 * @param out_buf Output text buffer for JSON payload.
 * @param max_len Capacity of output buffer.
 * @param out_len Pointer to store output byte count.
 * @return SYN_OK on success, SYN_INVALID_PARAM or SYN_ERROR on failure.
 */
SYN_Status syn_ocpp_format_stop_transaction(SYN_OCPP_Client *client, int32_t transaction_id,
                                            uint32_t meter_stop_wh, const char *reason,
                                            char *out_buf, size_t max_len, size_t *out_len);

/**
 * @brief Format an OCPP-J MeterValues.req Call frame.
 * @param client Pointer to client instance.
 * @param connector_id Connector ID.
 * @param values Pointer to current meter readings.
 * @param out_buf Output text buffer for JSON payload.
 * @param max_len Capacity of output buffer.
 * @param out_len Pointer to store output byte count.
 * @return SYN_OK on success, SYN_INVALID_PARAM or SYN_ERROR on failure.
 */
SYN_Status syn_ocpp_format_meter_values(SYN_OCPP_Client *client, uint32_t connector_id,
                                        const SYN_OCPP_MeterValues *values, char *out_buf,
                                        size_t max_len, size_t *out_len);

/**
 * @brief Process an incoming OCPP-J JSON frame (Call, CallResult, CallError).
 * @param client Pointer to client instance.
 * @param in_buf Incoming JSON text frame.
 * @param in_len Length of JSON text frame in bytes.
 * @param out_resp Optional output buffer for immediate response CallResult/CallError.
 * @param max_resp_len Capacity of output response buffer.
 * @param out_resp_len Pointer to store output response byte count.
 * @return SYN_OK on success, SYN_INVALID_PARAM or SYN_ERROR on failure.
 */
SYN_Status syn_ocpp_process_message(SYN_OCPP_Client *client, const char *in_buf, size_t in_len,
                                    char *out_resp, size_t max_resp_len, size_t *out_resp_len);

/**
 * @brief Tick periodic Heartbeat timer for OCPP client.
 * @param client Pointer to client instance.
 * @param dt_ms Elapsed time step in milliseconds.
 * @param out_hb_buf Optional output buffer to format Heartbeat.req if timer expires.
 * @param max_len Capacity of output heartbeat buffer.
 * @param out_len Pointer to store formatted heartbeat byte count (0 if no heartbeat needed).
 */
void syn_ocpp_tick(SYN_OCPP_Client *client, uint32_t dt_ms, char *out_hb_buf, size_t max_len,
                   size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif /* SYN_OCPP_H */
