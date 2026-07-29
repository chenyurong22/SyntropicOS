/**
 * @file stm32_ccp_example.c
 * @brief SyntropicOS STM32 CAN Calibration Protocol (CCP v2.1) Example Application.
 *
 * Demonstrates integrating CCP v2.1 slave protocol stack on STM32 with bxCAN.
 * Enables live tuning of PID parameters, motor control variables, and periodic
 * DAQ list measurement streaming.
 */

#include "syntropic/proto/syn_ccp.h"
#include "syntropic/drivers/syn_can.h"
#include "syntropic/control/syn_pid.h"
#include "syntropic/log/syn_log.h"
#include "syntropic/sched/syn_sched.h"

#include <stdbool.h>
#include <stdint.h>

#define STM32_CCP_STATION_ADDR 0x0100U

/* Live tunable PID controller parameters */
static SYN_PID g_pid;
static float g_kp = 2.5f;
static float g_ki = 0.1f;
static float g_kd = 0.05f;
static float g_setpoint = 100.0f;
static float g_process_var = 0.0f;

/* CCP Slave Instance */
static SYN_CCP_Slave g_ccp;

/* Simulated CAN hardware receive hook */
static void on_can_frame_received(uint32_t can_id, const uint8_t data[8], uint8_t len)
{
    (void)len;
    /* Master Command Receive Object (CRO) CAN ID = 0x100 */
    if (can_id == 0x100U) {
        uint8_t dto[8] = {0};
        if (syn_ccp_process_cro(&g_ccp, data, dto)) {
            /* Transmit Command Return Message (DTO/CRM) on CAN ID = 0x101 */
            syn_can_send(0, 0x101U, dto, 8);
        }
    }
}

/* Periodic 10ms timer task servicing live DAQ streaming & PID control */
static void app_10ms_task(void *arg)
{
    (void)arg;

    /* Update simulated system & compute PID */
    float control_output = syn_pid_update(&g_pid, g_setpoint, g_process_var, 0.010f);
    g_process_var += (control_output * 0.01f);

    /* Service periodic CCP DAQ Event Channel 1 */
    uint8_t daq_dto[8] = {0};
    uint8_t list_idx = 0;
    uint8_t odt_idx = 0;
    if (syn_ccp_service_daq(&g_ccp, 0x01U, daq_dto, &list_idx, &odt_idx)) {
        /* Send DAQ packet on CAN ID = 0x102 + list_idx */
        syn_can_send(0, 0x102U + list_idx, daq_dto, 8);
    }
}

int main(void)
{
    /* Initialize SyntropicOS Scheduler & CAN Driver */
    syn_can_init(0, 500000U);
    syn_can_set_rx_callback(0, on_can_frame_received);

    /* Initialize live tunable PID controller */
    syn_pid_init(&g_pid, g_kp, g_ki, g_kd, -100.0f, 100.0f);

    /* Initialize CCP v2.1 slave stack */
    syn_ccp_init(&g_ccp, STM32_CCP_STATION_ADDR);

    SYN_LOG_INFO("STM32 CCP v2.1 Calibration Example Started (Station Addr: 0x%04X)",
                 STM32_CCP_STATION_ADDR);

    /* Main event loop */
    while (1) {
        app_10ms_task(NULL);
    }

    return 0;
}
