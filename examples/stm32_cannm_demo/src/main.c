/**
 * @file main.c
 * @brief STM32 AUTOSAR CAN Network Management (CanNm) Demo.
 * @ingroup syn_examples
 *
 * Demonstrates AUTOSAR CAN Network Management state transitions, active wakeup,
 * and CAN NM PDU frame generation.
 */

#include "syntropic/syntropic.h"
#include <stdio.h>

static SYN_CanNM_Session cannm;

int main(void) {
    printf("SyntropicOS AUTOSAR CAN Network Management (CanNm) Demo\n");

    /* Initialize CAN NM with Node ID 0x01 (CAN ID 0x401) */
    SYN_CanNM_Config cfg = {
        .node_id = 0x01U,
        .can_id_base = 0x400U,
        .can_id_mask = 0x7F0U,
        .msg_cycle_ms = 100U,
        .nm_timeout_ms = 1000U,
        .wait_bus_sleep_ms = 1500U,
        .repeat_msg_time_ms = 1600U
    };
    syn_cannm_init(&cannm, &cfg);

    /* Set ECU Status in CAN NM User Payload */
    uint8_t status_data[6] = {0x01, 0x02, 0x00, 0x00, 0x00, 0x00};
    syn_cannm_set_user_data(&cannm, status_data, sizeof(status_data));

    /* Request Network Active Operation */
    printf("Requesting Network Active Operation...\n");
    syn_cannm_request_network(&cannm);

    SYN_CAN_Frame tx_frame;
    for (int ms = 0; ms <= 2000; ms += 50) {
        if (syn_cannm_step(&cannm, 50, &tx_frame)) {
            printf("[t=%4dms] Tx CAN NM PDU -> ID: 0x%03X, CBV: 0x%02X\n",
                   ms, (unsigned int)tx_frame.id, tx_frame.data[1]);
        }
    }

    /* Release Network Request to allow sleep sequence */
    printf("Releasing Network Request...\n");
    syn_cannm_release_network(&cannm);

    for (int ms = 2050; ms <= 5000; ms += 50) {
        syn_cannm_step(&cannm, 50, &tx_frame);
    }

    printf("CAN NM State after sleep sequence: %d (BUS_SLEEP)\n", (int)cannm.state);
    return 0;
}
