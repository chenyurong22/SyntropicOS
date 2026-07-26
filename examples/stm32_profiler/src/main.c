/**
 * @file main.c
 * @brief STM32 Protothread Task Scheduler & CPU Profiler Integration Example.
 *
 * Demonstrates the SyntropicOS cooperative protothread scheduler (syn_sched)
 * working hand-in-hand with the CPU load profiler (syn_profiler) on STM32 targets:
 * - Creating protothread tasks with priorities via syn_task_create()
 * - Managing task execution in a cooperative scheduling loop via syn_sched_run()
 * - Profiling CPU execution time, peak run time, and CPU % per task via syn_profiler
 * - Outputting real-time CPU performance breakdown tables over STM32 UART
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "syntropic/debug/syn_profiler.h"
#include "syntropic/port/syn_port_system.h"
#include "syntropic/pt/syn_pt.h"
#include "syntropic/sched/syn_sched.h"

/* ── Task Identifiers & Counts ───────────────────────────────────────────── */
#define TASK_SENSOR   0
#define TASK_CONTROL  1
#define TASK_MONITOR  2
#define TASK_COUNT    3

static SYN_Task s_tasks[TASK_COUNT];
static SYN_ProfileEntry s_profile_entries[TASK_COUNT];
static SYN_Profiler s_profiler;

/* ── STM32 Hardware Dummy UART Helper ───────────────────────────────────── */

static void stm32_uart_send_string(const char *str)
{
    while (*str) {
        /* On real STM32 hardware:
         * while (!(USART1->SR & USART_SR_TXE));
         * USART1->DR = (uint32_t)*str++;
         */
        (void)str++;
    }
}

static void print_profiler_output(const char *str)
{
    stm32_uart_send_string(str);
}

/* ── Protothread Task 0: Sensor Sampling ─────────────────────────────────── */

static SYN_PT_Status task_sensor_func(SYN_PT *pt, SYN_Task *task)
{
    (void)task;
    PT_BEGIN(pt);

    while (1) {
        /* Simulate ADC sensor reading workload (~1 ms) */
        syn_port_delay_ms(1);

        /* Yield task to allow lower/equal priority tasks to run */
        PT_YIELD(pt);
    }

    PT_END(pt);
}

/* ── Protothread Task 1: PID Control Loop ───────────────────────────────── */

static SYN_PT_Status task_control_func(SYN_PT *pt, SYN_Task *task)
{
    (void)task;
    PT_BEGIN(pt);

    while (1) {
        /* Simulate PID calculation workload (~1 ms) */
        syn_port_delay_ms(1);

        /* Yield task */
        PT_YIELD(pt);
    }

    PT_END(pt);
}

/* ── Protothread Task 2: System Monitor & Profiler Dumper ───────────────── */

static SYN_PT_Status task_monitor_func(SYN_PT *pt, SYN_Task *task)
{
    (void)task;
    static uint32_t last_report_ms = 0;

    PT_BEGIN(pt);

    while (1) {
        uint32_t now_ms = syn_port_get_tick_ms();

        /* Periodically (every 1000 ms) calculate CPU load % and dump report */
        if (now_ms - last_report_ms >= 1000u) {
            last_report_ms = now_ms;

            /* Compute CPU utilization percentages */
            syn_profiler_update(&s_profiler);

            /* Output CPU load table over UART */
            stm32_uart_send_string("\r\n=== Real-Time Task CPU Load Report ===\r\n");
            syn_profiler_dump(&s_profiler, print_profiler_output);
        }

        /* Yield task */
        PT_YIELD(pt);
    }

    PT_END(pt);
}

/* ── Main Entry Point ───────────────────────────────────────────────────── */

int main(void)
{
    stm32_uart_send_string("SyntropicOS STM32 Scheduler & CPU Profiler Example Started\r\n");

    /* 1. Initialize Task CPU Profiler */
    syn_profiler_init(&s_profiler, s_profile_entries, TASK_COUNT);

    syn_profiler_register(&s_profiler, TASK_SENSOR, "SensorTask");
    syn_profiler_register(&s_profiler, TASK_CONTROL, "ControlTask");
    syn_profiler_register(&s_profiler, TASK_MONITOR, "MonitorTask");

    /* 2. Create Protothread Task Descriptors */
    syn_task_create(&s_tasks[TASK_SENSOR], "SensorTask", task_sensor_func, 1, NULL);
    syn_task_create(&s_tasks[TASK_CONTROL], "ControlTask", task_control_func, 1, NULL);
    syn_task_create(&s_tasks[TASK_MONITOR], "MonitorTask", task_monitor_func, 2, NULL);

    /* 3. Initialize Protothread Scheduler */
    SYN_Sched sched;
    syn_sched_init(&sched, s_tasks, TASK_COUNT);

    /* 4. Combined Scheduler & Profiler Loop */
    for (int i = 0; i < 100; i++) {
        /* Run one step of highest-priority ready task with profiler hooks */
        uint8_t current_task_idx = 0; /* Track scheduled task index */

        syn_profiler_task_begin(&s_profiler, current_task_idx);
        syn_sched_run(&sched);
        syn_profiler_task_end(&s_profiler, current_task_idx);
    }

    stm32_uart_send_string("\r\n=== STM32 Scheduler & CPU Profiler Demo Complete ===\r\n");
    return 0;
}
