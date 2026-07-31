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

extern int main_bare(void);
extern int main_sched(void);

static void silence_unused_warnings(void)
{
    (void)s_tasks;
    (void)s_profile_entries;
    (void)task_sensor_func;
    (void)task_control_func;
    (void)task_monitor_func;
}

int main(void)
{
    silence_unused_warnings();
#if defined(USE_BARE_LOOP)
    return main_bare();
#else
    return main_sched();
#endif
}
