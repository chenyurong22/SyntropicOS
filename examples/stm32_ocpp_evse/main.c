/**
 * @file main.c
 * @brief STM32 OCPP-J (Open Charge Point Protocol 1.6 / 2.0.1) Dual-Role Example.
 * @ingroup syn_examples
 */

#include "syntropic/proto/syn_ocpp.h"
#include "syntropic/syntropic.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static SYN_OCPP_Client g_evse_client;
static SYN_OCPP_Server g_csms_server;

static SYN_OCPP_MeterValues g_meter_readings = {
    .energy_wh = 14200U,
    .voltage_v = 230U,
    .current_a = 160U, /* 16.0A */
    .power_kw = 3680U, /* 3.68 kW */
    .soc_percent = 65U
};

/* Client Callbacks */
static void on_client_registered(SYN_OCPP_RegistrationStatus status, uint32_t interval, void *ctx) {
    (void)ctx;
    printf("[EVSE Client] Registered with CSMS! Heartbeat Interval: %u sec\n", (unsigned int)interval);
}

/* Server Callbacks */
static SYN_OCPP_RegistrationStatus on_server_boot(const SYN_OCPP_ChargePointInfo *info,
                                                   uint32_t *hb_sec, void *ctx) {
    (void)ctx;
    printf("[CSMS Server] Received BootNotification from Model: '%s' (Vendor: '%s')\n",
           info ? info->charge_point_model : "Unknown",
           info ? info->charge_point_vendor : "Unknown");
    if (hb_sec) *hb_sec = 120U;
    return SYN_OCPP_REGISTRATION_ACCEPTED;
}

int main(void) {
    printf("=== STM32 OCPP-J (v1.6 / v2.0.1) Dual-Role EVSE / CSMS Example ===\n");

    /* 1. Initialize EVSE Client and CSMS Server */
    syn_ocpp_init(&g_evse_client);
    syn_ocpp_set_callbacks(&g_evse_client, on_client_registered, NULL, NULL, NULL, NULL, NULL);

    syn_ocpp_server_init(&g_csms_server);
    syn_ocpp_server_set_callbacks(&g_csms_server, on_server_boot, NULL, NULL, NULL);

    SYN_OCPP_ChargePointInfo cp_info = {
        .charge_point_vendor = "SyntropicPower",
        .charge_point_model = "EVSE-AC-7KW",
        .serial_number = "SYN-EVSE-2026-888",
        .firmware_version = "v2.1.0"
    };

    char client_req_buf[512];
    size_t client_req_len = 0U;

    char server_resp_buf[512];
    size_t server_resp_len = 0U;

    /* 2. Client formats BootNotification.req */
    if (syn_ocpp_format_boot_notification(&g_evse_client, &cp_info, client_req_buf,
                                          sizeof(client_req_buf), &client_req_len) == SYN_OK) {
        printf("\n[1. EVSE -> CSMS] BootNotification.req:\n%s\n", client_req_buf);
    }

    /* 3. Server processes BootNotification.req and generates BootNotification.conf */
    if (syn_ocpp_server_process_message(&g_csms_server, client_req_buf, client_req_len,
                                         server_resp_buf, sizeof(server_resp_buf),
                                         &server_resp_len) == SYN_OK) {
        printf("\n[2. CSMS -> EVSE] BootNotification.conf:\n%s\n", server_resp_buf);
    }

    /* 4. Client processes BootNotification.conf response */
    syn_ocpp_process_message(&g_evse_client, server_resp_buf, server_resp_len, NULL, 0U, NULL);

    /* 5. CSMS Server formats RemoteStartTransaction command */
    if (syn_ocpp_server_format_remote_start(&g_csms_server, 1U, "RFID-VIP-101", server_resp_buf,
                                            sizeof(server_resp_buf), &server_resp_len) == SYN_OK) {
        printf("\n[3. CSMS -> EVSE] RemoteStartTransaction.req:\n%s\n", server_resp_buf);
    }

    return 0;
}
