/**
 * @file main_sched.c
 * @brief STM32 Protothread Task Scheduler & CPU Profiler Integration Example (`syn_sched` Variant).
 *
 * Demonstrates the SyntropicOS cooperative protothread scheduler (syn_sched)
 * working hand-in-hand with the CPU load profiler (syn_profiler) on STM32 targets.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "syntropic/debug/syn_profiler.h"
#include "syntropic/port/syn_port_system.h"
#include "syntropic/pt/syn_pt.h"
#include "syntropic/sched/syn_sched.h"

#define TASK_SENSOR   0
#define TASK_CONTROL  1
#define TASK_MONITOR  2
#define TASK_COUNT    3

static SYN_Task s_tasks[TASK_COUNT];
static SYN_ProfileEntry s_profile_entries[TASK_COUNT];
static SYN_Profiler s_profiler;

static void stm32_uart_send_string(const char *str)
{
    while (*str) {
        (void)str++;
    }
}

static void print_profiler_output(const char *str)
{
    stm32_uart_send_string(str);
}

static SYN_PT_Status task_sensor_func(SYN_PT *pt, SYN_Task *task)
{
    (void)task;
    PT_BEGIN(pt);

    while (1) {
        syn_port_delay_ms(1);
        PT_YIELD(pt);
    }

    PT_END(pt);
}

static SYN_PT_Status task_control_func(SYN_PT *pt, SYN_Task *task)
{
    (void)task;
    PT_BEGIN(pt);

    while (1) {
        syn_port_delay_ms(1);
        PT_YIELD(pt);
    }

    PT_END(pt);
}

static SYN_PT_Status task_monitor_func(SYN_PT *pt, SYN_Task *task)
{
    (void)task;
    static uint32_t last_report_ms = 0;

    PT_BEGIN(pt);

    while (1) {
        uint32_t now_ms = syn_port_get_tick_ms();

        if (now_ms - last_report_ms >= 1000u) {
            last_report_ms = now_ms;

            syn_profiler_update(&s_profiler);
            stm32_uart_send_string("\r\n=== Real-Time Task CPU Load Report ===\r\n");
            syn_profiler_dump(&s_profiler, print_profiler_output);
        }

        PT_YIELD(pt);
    }

    PT_END(pt);
}

int main_sched(void)
{
    stm32_uart_send_string("SyntropicOS STM32 Scheduler & CPU Profiler Example Started\r\n");

    syn_profiler_init(&s_profiler, s_profile_entries, TASK_COUNT);

    syn_profiler_register(&s_profiler, TASK_SENSOR, "SensorTask");
    syn_profiler_register(&s_profiler, TASK_CONTROL, "ControlTask");
    syn_profiler_register(&s_profiler, TASK_MONITOR, "MonitorTask");

    syn_task_create(&s_tasks[TASK_SENSOR], "SensorTask", task_sensor_func, 1, NULL);
    syn_task_create(&s_tasks[TASK_CONTROL], "ControlTask", task_control_func, 1, NULL);
    syn_task_create(&s_tasks[TASK_MONITOR], "MonitorTask", task_monitor_func, 2, NULL);

    SYN_Sched sched;
    syn_sched_init(&sched, s_tasks, TASK_COUNT);

    for (int i = 0; i < 100; i++) {
        uint8_t current_task_idx = 0;

        syn_profiler_task_begin(&s_profiler, current_task_idx);
        syn_sched_run(&sched);
        syn_profiler_task_end(&s_profiler, current_task_idx);
    }

    stm32_uart_send_string("\r\n=== STM32 Scheduler & CPU Profiler Demo Complete ===\r\n");
    return 0;
}
