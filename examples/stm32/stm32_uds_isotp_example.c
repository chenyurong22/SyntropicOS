/**
 * @file stm32_uds_isotp_example.c
 * @brief SyntropicOS STM32 UDS (ISO 14229) over ISO-TP (ISO 15765-2) CAN Example.
 *
 * Demonstrates integrating UDS diagnostic server on STM32 using ISO-TP transport over CAN.
 * Supports DiagnosticSessionControl, SecurityAccess, ReadDataByIdentifier (RDBI),
 * WriteDataByIdentifier (WDBI), RoutineControl, and ECUReset.
 */

#include "syntropic/proto/syn_uds.h"
#include "syntropic/proto/syn_isotp.h"
#include "syntropic/drivers/syn_can.h"
#include "syntropic/log/syn_log.h"
#include "syntropic/sched/syn_sched.h"

#include <stdbool.h>
#include <stdint.h>

#define STM32_UDS_RX_CAN_ID 0x7E0U
#define STM32_UDS_TX_CAN_ID 0x7E8U

/* UDS Server Instance Context */
static SYN_UDS_Server g_uds;

/* ISO-TP Transport Instance Context */
static SYN_ISOTP_Link g_isotp;

/* Shared DID Memory Buffers */
static uint8_t g_vin_did[17] = "SYN1234567890ABCD";
static uint8_t g_battery_volts[2] = {0x00, 0x78}; /* 12.0 Volts */

/* ISO-TP CAN Frame Transmit Callback Hook */
static bool isotp_can_tx_callback(uint32_t arbitration_id, const uint8_t *data, uint8_t size)
{
    (void)arbitration_id;
    return syn_can_send(0, STM32_UDS_TX_CAN_ID, data, size);
}

/* Simulated CAN hardware receive hook */
static void on_can_frame_received(uint32_t can_id, const uint8_t data[8], uint8_t len)
{
    if (can_id == STM32_UDS_RX_CAN_ID) {
        syn_isotp_on_can_rx(&g_isotp, data, len);
    }
}

/* Periodic 10ms timer task servicing ISO-TP transport & UDS request processing */
static void app_10ms_task(void *arg)
{
    (void)arg;

    /* Poll ISO-TP link */
    syn_isotp_poll(&g_isotp, 10);

    /* Check if full UDS request payload received over ISO-TP */
    uint8_t req_buf[256] = {0};
    uint16_t req_len = 0;
    if (syn_isotp_receive(&g_isotp, req_buf, sizeof(req_buf), &req_len)) {
        uint8_t resp_buf[256] = {0};
        uint16_t resp_len = 0;

        /* Process UDS diagnostic request */
        if (syn_uds_process_request(&g_uds, req_buf, req_len, resp_buf, sizeof(resp_buf), &resp_len)) {
            /* Transmit UDS response over ISO-TP multi-frame transport */
            syn_isotp_send(&g_isotp, resp_buf, resp_len);
        }
    }
}

void stm32_uds_isotp_example_init(void)
{
    /* Initialize UDS Server */
    syn_uds_init(&g_uds);

    /* Register Data Identifiers (DIDs) */
    syn_uds_register_did(&g_uds, 0xF190U, g_vin_did, sizeof(g_vin_did), false);           /* VIN DID (Read-only) */
    syn_uds_register_did(&g_uds, 0x0100U, g_battery_volts, sizeof(g_battery_volts), true); /* Battery Voltage DID (Writable) */

    /* Initialize ISO-TP Link */
    syn_isotp_init(&g_isotp, STM32_UDS_RX_CAN_ID, STM32_UDS_TX_CAN_ID, isotp_can_tx_callback);

    SYN_LOG_INFO("STM32 UDS Server (ISO 14229 over ISO 15765-2) initialized on CAN RX:0x%03X TX:0x%03X",
                 STM32_UDS_RX_CAN_ID, STM32_UDS_TX_CAN_ID);

    /* Register periodic 10ms timer task */
    syn_sched_register_task("uds_isotp_task", app_10ms_task, NULL, 10);
}
