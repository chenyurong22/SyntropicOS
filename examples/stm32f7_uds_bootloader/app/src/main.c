/**
 * @file main.c
 * @brief STM32F767 Active Application Firmware with ISO 14229-1 Pre-Programming Engine.
 * @ingroup syn_examples
 *
 * Implements ISO 14229-1 / AUTOSAR FBL Pre-Programming Phase #1:
 *  1. 0x10 0x03 DiagnosticSessionControl (extendedSession)
 *  2. 0x85 0x02 ControlDTCSetting (off)
 *  3. 0x28 0x03 CommunicationControl (disableRxAndTx)
 *  4. 0x3E 0x80 TesterPresent keep-alive (suppressPosRspMsgIndicationBit = TRUE)
 *  5. 0x10 0x02 DiagnosticSessionControl (programmingSession) -> Transition to FBL
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
} App_PreProgrammingState;

static App_PreProgrammingState g_app_preprog;

/* CommunicationControl Handler (0x28) */
static bool on_app_comm_control(SYN_UDS_CommControlType control_type, uint8_t comm_type,
                                void *ctx) {
    (void)comm_type; (void)ctx;
    if (control_type == SYN_UDS_COMM_DISABLE_RX_AND_TX) {
        g_app_preprog.comm_disabled = true;
        printf("[App UDS 0x28] CommunicationControl: RX and TX Disabled.\n");
        return true;
    }
    return false;
}

int main(void) {
    printf("=== STM32F767 Active Application Firmware (Running in Bank A @ 0x08020000) ===\n");
    memset(&g_app_preprog, 0, sizeof(g_app_preprog));

    /* Initialize UDS Server in Application */
    syn_uds_init(&g_app_uds_server);
    syn_uds_register_comm_control(&g_app_uds_server, on_app_comm_control, NULL);

    printf("Application Normal Operation. Listening for UDS Pre-Programming Phase #1...\n");

    /* Simulated Pre-Programming Steps */
    for (int i = 0; i < 5; i++) {
        syn_uds_tick(&g_app_uds_server, 10);
    }

    return 0;
}
