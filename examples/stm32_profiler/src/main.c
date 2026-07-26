/**
 * @file main.c
 * @brief STM32 Task CPU Profiler & Execution Jitter Monitoring Example.
 *
 * Demonstrates real-time task CPU load measurement and profiling on STM32 (Cortex-M) targets:
 * - Registering cooperative tasks with the SyntropicOS profiler (syn_profiler)
 * - Measuring microsecond execution time, peak run time, and invocation count per task
 * - Calculating CPU utilization percentages (0.1% resolution)
 * - Outputting real-time CPU performance tables over UART
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "syntropic/debug/syn_profiler.h"
#include "syntropic/debug/syn_task_profile.h"
#include "syntropic/port/syn_port_system.h"

/* ── Task Identifiers ────────────────────────────────────────────────────── */
#define TASK_SENSOR   0
#define TASK_CONTROL  1
#define TASK_NETWORK  2
#define TASK_COUNT    3

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

/* ── Simulated Task Workloads ────────────────────────────────────────────── */

static void run_sensor_task(void)
{
    /* Simulate sensor sampling workload (~500 µs execution time) */
    syn_port_delay_ms(1);
}

static void run_control_task(void)
{
    /* Simulate PID control calculation workload (~200 µs execution time) */
    syn_port_delay_ms(1);
}

static void run_network_task(void)
{
    /* Simulate telemetry network packet dispatch (~100 µs execution time) */
    syn_port_delay_ms(1);
}

/* ── Main Entry Point ───────────────────────────────────────────────────── */

int main(void)
{
    stm32_uart_send_string("SyntropicOS STM32 Task CPU Profiler Example Started\r\n");

    /* 1. Initialize Task CPU Profiler (1000 ms measurement window) */
    syn_profiler_init(&s_profiler, s_profile_entries, TASK_COUNT);

    /* Assign task labels */
    s_profile_entries[TASK_SENSOR].name = "SensorTask";
    s_profile_entries[TASK_CONTROL].name = "ControlTask";
    s_profile_entries[TASK_NETWORK].name = "NetworkTask";

    uint32_t last_report_ms = syn_port_get_tick_ms();

    /* 2. Scheduler / Application Loop */
    for (int i = 0; i < 50; i++) {
        /* Task 0: Sensor Sampling */
        syn_profiler_task_begin(&s_profiler, TASK_SENSOR);
        run_sensor_task();
        syn_profiler_task_end(&s_profiler, TASK_SENSOR);

        /* Task 1: Control Loop */
        syn_profiler_task_begin(&s_profiler, TASK_CONTROL);
        run_control_task();
        syn_profiler_task_end(&s_profiler, TASK_CONTROL);

        /* Task 2: Network Telemetry */
        syn_profiler_task_begin(&s_profiler, TASK_NETWORK);
        run_network_task();
        syn_profiler_task_end(&s_profiler, TASK_NETWORK);

        /* Periodically (every 1000 ms) calculate CPU load % and dump report */
        uint32_t now_ms = syn_port_get_tick_ms();
        if (now_ms - last_report_ms >= 1000u) {
            last_report_ms = now_ms;

            /* Compute CPU utilization percentages */
            syn_profiler_update(&s_profiler);

            /* Dump formatted CPU load table to UART */
            stm32_uart_send_string("\r\n=== Real-Time CPU Load Report ===\r\n");
            syn_profiler_dump(&s_profiler, print_profiler_output);
        }
    }

    stm32_uart_send_string("\r\n=== STM32 Task CPU Profiler Demo Complete ===\r\n");
    return 0;
}
