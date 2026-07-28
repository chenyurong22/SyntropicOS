/**
 * @file main_sched.c
 * @brief PID Temperature Controller Example (`syn_sched` Cooperative Task Scheduler Variant).
 *
 * Demonstrates 100 Hz PID control loop task + 1 Hz telemetry task scheduled
 * cleanly via priority protothread task queues (syn_sched).
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "syntropic/control/syn_pid.h"
#include "syntropic/port/syn_port_system.h"
#include "syntropic/pt/syn_pt.h"
#include "syntropic/sched/syn_sched.h"

static SYN_PID s_pid;
static q16_t   s_current_temp = Q16_FROM_INT(20);

#define TASK_PID_CONTROL 0
#define TASK_TELEMETRY   1
#define TASK_COUNT       2

static SYN_Task  s_tasks[TASK_COUNT];
static SYN_Sched s_sched;

static SYN_PT_Status task_pid_control_func(SYN_PT *pt, SYN_Task *task)
{
    (void)task;
    static uint32_t last_ms = 0;

    PT_BEGIN(pt);

    while (1) {
        uint32_t now_ms = syn_port_get_tick_ms();
        uint32_t dt_ms  = (last_ms == 0) ? 10 : (now_ms - last_ms);
        last_ms         = now_ms;

        q16_t output = syn_pid_update(&s_pid, s_current_temp, dt_ms);
        s_current_temp += (output >> 10);

        PT_TASK_DELAY_MS(pt, task, 10);
    }

    PT_END(pt);
}

static SYN_PT_Status task_telemetry_func(SYN_PT *pt, SYN_Task *task)
{
    (void)task;
    PT_BEGIN(pt);

    while (1) {
        /* Telemetry logging output */
        PT_TASK_DELAY_MS(pt, task, 1000);
    }

    PT_END(pt);
}

int main_sched(void)
{
    SYN_PID_Config cfg = {
        .kp = Q16_FROM_FLOAT(2.0f),
        .ki = Q16_FROM_FLOAT(0.5f),
        .kd = Q16_FROM_FLOAT(0.1f),
        .out_min = 0,
        .out_max = Q16_FROM_INT(100)
    };
    syn_pid_init(&s_pid, &cfg);
    syn_pid_set_setpoint(&s_pid, Q16_FROM_INT(50));

    syn_task_create(&s_tasks[TASK_PID_CONTROL], "PidControl", task_pid_control_func, 1, NULL);
    syn_task_create(&s_tasks[TASK_TELEMETRY],   "Telemetry",  task_telemetry_func,   2, NULL);

    syn_sched_init(&s_sched, s_tasks, TASK_COUNT);

    while (1) {
        syn_sched_run(&s_sched);
    }

    return 0;
}
