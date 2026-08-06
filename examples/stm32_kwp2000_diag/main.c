/**
 * @file main.c
 * @brief STM32 KWP2000 (ISO 14230-3 over CAN) Diagnostic Server Example.
 * @ingroup syn_examples
 */

#include "syntropic/proto/syn_isotp.h"
#include "syntropic/proto/syn_kwp2000.h"
#include "syntropic/syntropic.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static SYN_KWP2000_Server g_kwp_server;
static uint16_t g_engine_rpm = 2800U;
static uint8_t g_coolant_temp_c = 85U;
static uint8_t g_ecu_vin[17] = "SYN-STM32-KWP2000";

static bool on_kwp_session_transition(SYN_KWP2000_Session from_session,
                                       SYN_KWP2000_Session to_session, void *ctx) {
    (void)ctx;
    printf("[KWP2000 0x10] Session Transition Request: 0x%02X -> 0x%02X\n",
           (unsigned int)from_session, (unsigned int)to_session);
    return true;
}

static void on_kwp_reset_cb(uint8_t reset_type, void *ctx) {
    (void)ctx;
    printf("[KWP2000 0x11] Post-TX ECUReset Callback: Executing Reset Type 0x%02X\n",
           (unsigned int)reset_type);
}

int main(void) {
    printf("=== STM32 ISO 14230-3 KWP2000 Diagnostic Server Example ===\n");

    syn_kwp2000_init(&g_kwp_server);

    /* 0x10 StartDiagnosticSession */
    syn_kwp2000_set_session_handler(&g_kwp_server, on_kwp_session_transition, NULL);

    /* 0x11 ECUReset */
    syn_kwp2000_set_reset_handler(&g_kwp_server, on_kwp_reset_cb, NULL);

    /* 0x21 ReadDataByLocalIdentifier (LID) */
    syn_kwp2000_register_lid(&g_kwp_server, 0x01U, &g_engine_rpm, sizeof(g_engine_rpm), false);
    syn_kwp2000_register_lid(&g_kwp_server, 0x02U, &g_coolant_temp_c, sizeof(g_coolant_temp_c), false);

    /* 0x22 ReadDataByCommonIdentifier (CID) */
    syn_kwp2000_register_cid(&g_kwp_server, 0xF190U, g_ecu_vin, sizeof(g_ecu_vin), true);

    printf("KWP2000 Server Initialized. Servicing LID 0x01 (RPM), LID 0x02 (Temp), CID 0xF190 (VIN)...\n");

    /* Simulated request processing */
    uint8_t req_lid1[] = {0x21, 0x01};
    uint8_t resp_buf[64];
    uint16_t resp_len = 0U;

    if (syn_kwp2000_process_request(&g_kwp_server, req_lid1, sizeof(req_lid1), resp_buf,
                                    sizeof(resp_buf), &resp_len) == SYN_OK) {
        printf("[KWP2000 0x21 Response] Length = %u bytes, Status = 0x%02X, LID = 0x%02X\n",
               (unsigned int)resp_len, (unsigned int)resp_buf[0], (unsigned int)resp_buf[1]);
    }

    return 0;
}
