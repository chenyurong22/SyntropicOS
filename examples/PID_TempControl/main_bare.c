/**
 * @file main_bare.c
 * @brief PID Temperature Controller Example (Bare-Metal while(1) Polling Loop Variant).
 *
 * Demonstrates 100 Hz PID closed-loop feedback calculation and 1 Hz telemetry reporting
 * in a single while(1) polling loop with manual tick subtraction.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "syntropic/control/syn_pid.h"
#include "syntropic/port/syn_port_system.h"

static SYN_PID s_pid;

int main_bare(void)
{
    /* Initialize integer PID controller (Kp=2.0, Ki=0.5, Kd=0.1 in Q16) */
    SYN_PID_Config cfg = {
        .kp = Q16_FROM_FLOAT(2.0f),
        .ki = Q16_FROM_FLOAT(0.5f),
        .kd = Q16_FROM_FLOAT(0.1f),
        .out_min = 0,
        .out_max = Q16_FROM_INT(100) /* 0 to 100% PWM */
    };
    syn_pid_init(&s_pid, &cfg);
    syn_pid_set_setpoint(&s_pid, Q16_FROM_INT(50)); /* Target 50 °C */

    uint32_t last_pid_ms  = syn_port_get_tick_ms();
    uint32_t last_tele_ms = syn_port_get_tick_ms();
    q16_t    current_temp = Q16_FROM_INT(20);

    while (1) {
        uint32_t now_ms = syn_port_get_tick_ms();

        /* High-Priority PID Control Loop (10ms / 100 Hz) */
        if (now_ms - last_pid_ms >= 10) {
            uint32_t dt_ms = now_ms - last_pid_ms;
            last_pid_ms    = now_ms;

            q16_t output = syn_pid_update(&s_pid, current_temp, dt_ms);
            /* Simulate plant response: temperature rises proportionally to PWM output */
            current_temp += (output >> 10);
        }

        /* Low-Priority Telemetry Logging (1000ms / 1 Hz) */
        if (now_ms - last_tele_ms >= 1000) {
            last_tele_ms = now_ms;
            /* Print current process value and setpoint */
        }
    }

    return 0;
}
