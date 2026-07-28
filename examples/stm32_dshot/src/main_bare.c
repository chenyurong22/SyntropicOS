/**
 * @file main_bare.c
 * @brief STM32 DShot Digital ESC Example (Bare-Metal Loop Variant).
 *
 * Demonstrates encoding 11-bit throttle commands to DShot150/300/600 frames
 * and decoding Bidirectional DShot (BDShot) 20-bit GCR telemetry packets:
 * - `syn_dshot_us_to_throttle`
 * - `syn_dshot_encode`
 * - `syn_dshot_parse_telemetry`
 */

#include <stdbool.h>
#include <stdint.h>

#include "syntropic/output/syn_dshot.h"
#include "syntropic/output/syn_dshot_telemetry.h"
#include "syntropic/port/syn_port_system.h"

static void stm32_dshot_send_frame_dma(uint16_t raw_frame)
{
    /* Output 16-bit DShot frame via TIM DMA to ESC pin */
    (void)raw_frame;
}

int main_bare(void)
{
    uint32_t last_send_ms = syn_port_get_tick_ms();
    uint16_t rc_pulse_us = 1500; /* Simulated 1500 us RC stick input */

    while (1) {
        uint32_t now_ms = syn_port_get_tick_ms();

        if (now_ms - last_send_ms >= 1) { /* 1 kHz ESC update rate */
            last_send_ms = now_ms;

            /* 1. Convert pulse width (1000..2000 us) to DShot throttle (48..2047) */
            uint16_t dshot_cmd = syn_dshot_us_to_throttle(rc_pulse_us);

            /* 2. Encode 16-bit packet with 4-bit CRC */
            SYN_DShot_Packet packet;
            if (syn_dshot_encode(dshot_cmd, false, &packet) == SYN_OK) {
                stm32_dshot_send_frame_dma(packet.raw_frame);
            }

            /* 3. Decode incoming BDShot 20-bit GCR telemetry frame from ESC */
            uint32_t sample_gcr_20bit = 0xAA555;
            SYN_DShot_Telemetry telemetry;
            if (syn_dshot_parse_telemetry(sample_gcr_20bit, 7, &telemetry) == SYN_OK) {
                uint32_t motor_rpm = telemetry.rpm;
                (void)motor_rpm;
            }
        }
    }

    return 0;
}
