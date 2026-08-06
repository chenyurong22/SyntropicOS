/**
 * @file main.c
 * @brief STM32 OCPP-J (Open Charge Point Protocol 1.6 / 2.0.1) EVSE Example.
 * @ingroup syn_examples
 */

#include "syntropic/proto/syn_ocpp.h"
#include "syntropic/syntropic.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static SYN_OCPP_Client g_evse_client;
static SYN_OCPP_MeterValues g_meter_readings = {
    .energy_wh = 14200U,
    .voltage_v = 230U,
    .current_a = 160U, /* 16.0A */
    .power_kw = 3680U, /* 3.68 kW */
    .soc_percent = 65U
};

static void on_ocpp_registered(SYN_OCPP_RegistrationStatus status, uint32_t interval, void *ctx) {
    (void)ctx;
    printf("[OCPP Client] Registration Status: %s, Heartbeat Interval: %u sec\n",
           (status == SYN_OCPP_REGISTRATION_ACCEPTED) ? "ACCEPTED" : "PENDING/REJECTED",
           (unsigned int)interval);
}

static void on_ocpp_authorized(const char *id_tag, SYN_OCPP_AuthorizationStatus status, void *ctx) {
    (void)ctx;
    printf("[OCPP Client] IdTag '%s' Authorization: %s\n", id_tag ? id_tag : "UNKNOWN",
           (status == SYN_OCPP_AUTH_ACCEPTED) ? "ACCEPTED" : "DENIED");
}

static void on_ocpp_start_tx_resp(int32_t tx_id, SYN_OCPP_AuthorizationStatus status, void *ctx) {
    (void)status; (void)ctx;
    printf("[OCPP Client] StartTransaction Confirmed: Transaction ID #%d\n", (int)tx_id);
}

static bool on_ocpp_remote_start_cmd(uint32_t connector_id, const char *id_tag, void *ctx) {
    (void)ctx;
    printf("[OCPP CSMS Command] RemoteStartTransaction received on Connector #%u (IdTag: %s)\n",
           (unsigned int)connector_id, id_tag ? id_tag : "REMOTE");
    return true;
}

static bool on_ocpp_remote_stop_cmd(int32_t transaction_id, void *ctx) {
    (void)ctx;
    printf("[OCPP CSMS Command] RemoteStopTransaction received for Transaction ID #%d\n", (int)transaction_id);
    return true;
}

int main(void) {
    printf("=== STM32 OCPP-J (v1.6 / v2.0.1) EVSE Charging Station Example ===\n");

    syn_ocpp_init(&g_evse_client);
    syn_ocpp_set_callbacks(&g_evse_client, on_ocpp_registered, on_ocpp_authorized,
                           on_ocpp_start_tx_resp, on_ocpp_remote_start_cmd,
                           on_ocpp_remote_stop_cmd, NULL);

    SYN_OCPP_ChargePointInfo cp_info = {
        .charge_point_vendor = "SyntropicPower",
        .charge_point_model = "EVSE-AC-7KW",
        .serial_number = "SYN-EVSE-2026-888",
        .firmware_version = "v2.1.0"
    };

    char payload_buf[512];
    size_t payload_len = 0U;

    /* 1. Format BootNotification */
    if (syn_ocpp_format_boot_notification(&g_evse_client, &cp_info, payload_buf,
                                          sizeof(payload_buf), &payload_len) == SYN_OK) {
        printf("[OCPP TX] BootNotification.req:\n%s\n", payload_buf);
    }

    /* 2. Format StatusNotification (Preparing) */
    if (syn_ocpp_format_status_notification(&g_evse_client, 1U, SYN_OCPP_STATUS_PREPARING,
                                             "NoError", payload_buf, sizeof(payload_buf),
                                             &payload_len) == SYN_OK) {
        printf("[OCPP TX] StatusNotification.req:\n%s\n", payload_buf);
    }

    /* 3. Format Authorize (RFID Scan) */
    if (syn_ocpp_format_authorize(&g_evse_client, "RFID-USER-9876", payload_buf,
                                  sizeof(payload_buf), &payload_len) == SYN_OK) {
        printf("[OCPP TX] Authorize.req:\n%s\n", payload_buf);
    }

    /* 4. Format StartTransaction */
    if (syn_ocpp_format_start_transaction(&g_evse_client, 1U, "RFID-USER-9876",
                                           g_meter_readings.energy_wh, payload_buf,
                                           sizeof(payload_buf), &payload_len) == SYN_OK) {
        printf("[OCPP TX] StartTransaction.req:\n%s\n", payload_buf);
    }

    /* 5. Format MeterValues */
    if (syn_ocpp_format_meter_values(&g_evse_client, 1U, &g_meter_readings, payload_buf,
                                     sizeof(payload_buf), &payload_len) == SYN_OK) {
        printf("[OCPP TX] MeterValues.req:\n%s\n", payload_buf);
    }

    /* 6. Simulate incoming Central System Response */
    const char *cs_response = "[3,\"1\",{\"status\":\"Accepted\",\"interval\":60}]";
    printf("[OCPP RX] Central System BootNotification.conf:\n%s\n", cs_response);
    syn_ocpp_process_message(&g_evse_client, cs_response, strlen(cs_response), NULL, 0U, NULL);

    return 0;
}
