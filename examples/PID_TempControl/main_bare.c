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
    /* Initialize integer PID controller (Kp=2.0, Ki=0.5, Kd=0.1) */
    SYN_PID_Config cfg = SYN_PID_GAINS(2.0f, 0.5f, 0.1f, 256, 0, 100);
    syn_pid_init(&s_pid, &cfg);

    uint32_t last_pid_ms  = syn_port_get_tick_ms();
    uint32_t last_tele_ms = syn_port_get_tick_ms();
    int32_t setpoint_temp = 50;
    int32_t current_temp  = 20;

    while (1) {
        uint32_t now_ms = syn_port_get_tick_ms();

        /* High-Priority PID Control Loop (10ms / 100 Hz) */
        if (now_ms - last_pid_ms >= 10) {
            uint32_t dt_ms = now_ms - last_pid_ms;
            last_pid_ms    = now_ms;

            int32_t output = syn_pid_update(&s_pid, setpoint_temp, current_temp, dt_ms);
            /* Simulate plant response: temperature rises proportionally to PWM output */
            current_temp += (output >> 4);
        }

        /* Low-Priority Telemetry Logging (1000ms / 1 Hz) */
        if (now_ms - last_tele_ms >= 1000) {
            last_tele_ms = now_ms;
            printf("Temp: %ld C, Setpoint: %ld C, Output: %ld%%\n",
                   (long)current_temp, (long)setpoint_temp, (long)s_pid.output);
        }
    }

    return 0;
}
