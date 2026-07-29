/**
 * @file stm32_xcp_example.c
 * @brief SyntropicOS STM32 XCP (ASAM MCD-1 XCP v1.x) Example Application.
 *
 * Demonstrates integrating ASAM XCP slave protocol stack on STM32 with bxCAN.
 * Enables live tuning of PID parameters, motor control variables, and periodic
 * DAQ list measurement streaming.
 */

#include "syntropic/proto/syn_xcp.h"
#include "syntropic/drivers/syn_can.h"
#include "syntropic/control/syn_pid.h"
#include "syntropic/log/syn_log.h"
#include "syntropic/sched/syn_sched.h"

#include <stdbool.h>
#include <stdint.h>

#define STM32_XCP_STATION_ID 0x0200U

/* Live tunable PID controller parameters */
static SYN_PID g_pid;
static float g_kp = 2.5f;
static float g_ki = 0.1f;
static float g_kd = 0.05f;
static float g_setpoint = 100.0f;
static float g_process_var = 0.0f;

/* XCP Slave Instance Context */
static SYN_XCP_Slave g_xcp;

/* Simulated CAN hardware receive hook */
static void on_can_frame_received(uint32_t can_id, const uint8_t data[8], uint8_t len)
{
    (void)len;
    /* Master Command Transfer Object (CTO) CAN ID = 0x200 */
    if (can_id == 0x200U) {
        uint8_t dto[8] = {0};
        if (syn_xcp_process_cto(&g_xcp, data, dto)) {
            /* Transmit Command Return Message (DTO/RES) on CAN ID = 0x201 */
            syn_can_send(0, 0x201U, dto, 8);
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

    /* Service periodic XCP DAQ Event Channel 1 */
    uint8_t daq_dto[8] = {0};
    uint8_t list_idx = 0;
    uint8_t odt_idx = 0;
    if (syn_xcp_service_daq(&g_xcp, 0x01U, daq_dto, &list_idx, &odt_idx)) {
        /* Send DAQ telemetry packet on CAN ID = 0x202 + list_idx */
        syn_can_send(0, 0x202U + list_idx, daq_dto, 8);
    }
}

void stm32_xcp_example_init(void)
{
    /* Initialize PID controller */
    syn_pid_init(&g_pid, g_kp, g_ki, g_kd);

    /* Initialize XCP Slave */
    syn_xcp_init(&g_xcp, STM32_XCP_STATION_ID);

    SYN_LOG_INFO("STM32 XCP Slave Example initialized on Station ID 0x%04X", STM32_XCP_STATION_ID);

    /* Register periodic 10ms timer task */
    syn_sched_register_task("xcp_pid_task", app_10ms_task, NULL, 10);
}
