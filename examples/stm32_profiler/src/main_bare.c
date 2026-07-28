/**
 * @file main_bare.c
 * @brief STM32 Bare-Metal CPU Profiler Example (Bare-Metal while(1) Polling Loop).
 *
 * Demonstrates manual execution time and CPU load monitoring inside a bare-metal loop:
 * - Direct execution of sensor sampling and control routines without OS scheduler
 * - Manual timestamp subtraction (syn_port_get_tick_ms) for period monitoring
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "syntropic/debug/syn_profiler.h"
#include "syntropic/port/syn_port_system.h"

#define TASK_SENSOR   0
#define TASK_CONTROL  1
#define TASK_MONITOR  2
#define TASK_COUNT    3

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

static void do_sensor_work(void)
{
    syn_port_delay_ms(1);
}

static void do_control_work(void)
{
    syn_port_delay_ms(1);
}

int main_bare(void)
{
    stm32_uart_send_string("SyntropicOS STM32 Bare-Metal Profiler Example Started\r\n");

    syn_profiler_init(&s_profiler, s_profile_entries, TASK_COUNT);
    syn_profiler_register(&s_profiler, TASK_SENSOR, "SensorTask");
    syn_profiler_register(&s_profiler, TASK_CONTROL, "ControlTask");
    syn_profiler_register(&s_profiler, TASK_MONITOR, "MonitorTask");

    uint32_t last_report_ms = syn_port_get_tick_ms();

    for (int i = 0; i < 100; i++) {
        /* Task 0: Sensor Work */
        syn_profiler_task_begin(&s_profiler, TASK_SENSOR);
        do_sensor_work();
        syn_profiler_task_end(&s_profiler, TASK_SENSOR);

        /* Task 1: Control Work */
        syn_profiler_task_begin(&s_profiler, TASK_CONTROL);
        do_control_work();
        syn_profiler_task_end(&s_profiler, TASK_CONTROL);

        /* Task 2: Monitoring & Dump */
        syn_profiler_task_begin(&s_profiler, TASK_MONITOR);
        uint32_t now_ms = syn_port_get_tick_ms();
        if (now_ms - last_report_ms >= 1000u) {
            last_report_ms = now_ms;
            syn_profiler_update(&s_profiler);
            stm32_uart_send_string("\r\n=== Real-Time Bare-Metal CPU Load Report ===\r\n");
            syn_profiler_dump(&s_profiler, print_profiler_output);
        }
        syn_profiler_task_end(&s_profiler, TASK_MONITOR);
    }

    stm32_uart_send_string("\r\n=== STM32 Bare-Metal Profiler Demo Complete ===\r\n");
    return 0;
}
