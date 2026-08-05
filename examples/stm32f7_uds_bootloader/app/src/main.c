/**
 * @file main.c
 * @brief STM32F767 Active Application Firmware with ISO 14229-1 Pre/Post-Programming Engine.
 * @ingroup syn_examples
 *
 * Implements ISO 14229-1 / AUTOSAR FBL Pre-Programming Phase #1 & Post-Programming Phase #3:
 *
 * Pre-Programming Phase #1:
 *  1. 0x10 0x03 DiagnosticSessionControl (extendedSession)
 *  2. 0x85 0x02 ControlDTCSetting (off)
 *  3. 0x28 0x03 CommunicationControl (disableRxAndTx)
 *  4. 0x3E 0x80 TesterPresent keep-alive (suppressPosRspMsgIndicationBit = TRUE)
 *
 * Post-Programming Phase #3 (After Reset & Application Startup):
 * 15. 0x28 0x00 0x00 CommunicationControl (enableRxAndTx)
 * 16. 0x85 0x01 ControlDTCSetting (on)
 * 17. 0x10 0x01 DiagnosticSessionControl (defaultSession)
 */

#include "syntropic/proto/syn_isotp.h"
#include "syntropic/proto/syn_uds.h"
#include "syntropic/syntropic.h"
#include "syntropic/system/syn_boot.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static SYN_UDS_Server g_app_uds_server;

typedef struct {
    bool dtc_disabled;
    bool comm_disabled;
    uint32_t tester_present_timer_ms;
} App_ProgrammingState;

static App_ProgrammingState g_app_prog_state;
static uint8_t g_app_vin[17];

/* 0x10 DiagnosticSessionControl Session Transition Callback */
static bool on_app_session_transition(SYN_UDS_Session from_session,
                                       SYN_UDS_Session to_session, void *ctx) {
    (void)ctx;
    printf("[App UDS 0x10] Session Transition Request: 0x%02X -> 0x%02X\n",
           (unsigned int)from_session, (unsigned int)to_session);
    return true;
}

/* 0x11 ECUReset Post-TX Callback */
static void on_app_reset_cb(uint8_t reset_type, void *ctx) {
    (void)ctx;
    printf("[App UDS 0x11] Post-TX ECUReset Callback: Executing Reset Type 0x%02X\n",
           (unsigned int)reset_type);
}

/* CommunicationControl Handler (0x28) */
static bool on_app_comm_control(SYN_UDS_CommControlType control_type, uint8_t comm_type,
                                void *ctx) {
    (void)comm_type; (void)ctx;
    if (control_type == SYN_UDS_COMM_DISABLE_RX_AND_TX) {
        g_app_prog_state.comm_disabled = true;
        printf("[App UDS 0x28] CommunicationControl: RX and TX Disabled (Pre-Programming).\n");
        return true;
    } else if (control_type == SYN_UDS_COMM_ENABLE_RX_AND_TX) {
        g_app_prog_state.comm_disabled = false;
        printf("[App UDS 0x28] CommunicationControl: RX and TX Enabled (Post-Programming).\n");
        return true;
    }
    return false;
}

int main(void) {
    printf("=== STM32F767 Active Application Firmware (Running in Bank A @ 0x08020000) ===\n");
    memset(&g_app_prog_state, 0, sizeof(g_app_prog_state));
    memcpy(g_app_vin, "SYN-STM32F767-APP", 17);

    /* Initialize UDS Server in Application */
    syn_uds_init(&g_app_uds_server);

    /* 0x10 DiagnosticSessionControl: Register Session Transition Handler */
    syn_uds_set_session_transition_handler(&g_app_uds_server, on_app_session_transition, NULL);

    /* 0x11 ECUReset: Register Deferred Post-TX Reset Handler & Wait Duration */
    syn_uds_set_reset_handler(&g_app_uds_server, on_app_reset_cb, NULL);
    syn_uds_set_reset_wait_ms(&g_app_uds_server, 50U);

    /* 0x28 CommunicationControl: Register Rx/Tx Control Callback */
    syn_uds_register_comm_control(&g_app_uds_server, on_app_comm_control, NULL);

    /* 0x85 ControlDTCSetting: Register Application Diagnostic Trouble Code */
    syn_uds_register_dtc(&g_app_uds_server, 0x012345U, SYN_UDS_DTC_STATUS_TEST_FAILED,
                         SYN_UDS_DTC_SEVERITY_MAINTENANCE_REQUIRED);

    /* 0x2E WriteDataByIdentifier: Register VIN DID 0xF190 (Writable) */
    syn_uds_register_did(&g_app_uds_server, 0xF190U, g_app_vin, sizeof(g_app_vin), true);

    printf("Application Normal Operation. Servicing ISO 14229-1 Pre/Post-Programming Steps...\n");

    /* Simulated Pre/Post-Programming Steps with 0x3E TesterPresent / S3 Timer Service */
    for (int i = 0; i < 5; i++) {
        syn_uds_tick(&g_app_uds_server, 10);
    }

    /* Check and clear pending ECU reset state (0x11) */
    uint8_t pending_reset = syn_uds_get_pending_reset(&g_app_uds_server);
    if (pending_reset != 0U) {
        printf("[App UDS 0x11] Pending Reset Detected: Type 0x%02X. Clearing Reset State...\n",
               (unsigned int)pending_reset);
        syn_uds_clear_pending_reset(&g_app_uds_server);
    }

    return 0;
}
