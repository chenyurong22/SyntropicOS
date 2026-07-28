/**
 * @file main_bare.c
 * @brief STM32 BDShot Telemetry Example (Bare-Metal Polling Variant).
 *
 * Demonstrates 20-bit GCR telemetry decoding and motor RPM feedback using syn_dshot_telemetry:
 * - `syn_dshot_decode_gcr_20bit`
 * - `syn_dshot_parse_telemetry`
 */

#include <stdbool.h>
#include <stdint.h>

#include "syntropic/output/syn_dshot_telemetry.h"
#include "syntropic/port/syn_port_system.h"

#define MOTOR_POLE_PAIRS 7U /* 14-pole BLDC motor */

int main_bare(void)
{
    uint32_t last_log_ms = syn_port_get_tick_ms();

    /* Simulated raw 20-bit GCR BDShot telemetry frames from ESC return pulse stream */
    const uint32_t gcr_frames[] = {
        0xAA555,
        0x55AAA,
        0x33CC5
    };
    const size_t frame_count = sizeof(gcr_frames) / sizeof(gcr_frames[0]);
    size_t frame_idx = 0;

    while (1) {
        uint32_t now_ms = syn_port_get_tick_ms();

        if (now_ms - last_log_ms >= 50) { /* 20 Hz update loop */
            last_log_ms = now_ms;

            uint32_t raw_gcr = gcr_frames[frame_idx];
            frame_idx = (frame_idx + 1) % frame_count;

            SYN_DShot_Telemetry telemetry;
            if (syn_dshot_parse_telemetry(raw_gcr, MOTOR_POLE_PAIRS, &telemetry) == SYN_OK) {
                if (telemetry.valid) {
                    uint32_t period_us = telemetry.period_us;
                    uint32_t erpm      = telemetry.erpm;
                    uint32_t rpm       = telemetry.rpm;
                    (void)period_us;
                    (void)erpm;
                    (void)rpm;
                }
            }
        }
    }

    return 0;
}
