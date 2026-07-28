/**
 * @file main_bare.c
 * @brief STM32 PPM RC Receiver Example (Bare-Metal Loop Variant).
 *
 * Demonstrates decoding multi-channel RC PPM frame pulse widths using syn_ppm:
 * - Pulse processing from Timer Input Capture interrupt or pulse queue (`syn_ppm_process_pulse`)
 * - Microsecond pulse extraction for Roll, Pitch, Throttle, Yaw, and Aux channels (`syn_ppm_get_channel`)
 */

#include <stdbool.h>
#include <stdint.h>

#include "syntropic/input/syn_ppm.h"
#include "syntropic/port/syn_port_system.h"

static SYN_PPM_Decoder s_ppm;

static void stm32_ppm_init_timer(void)
{
    /* Hardware initialization for Timer Input Capture (e.g. TIM2 CH1 on PA0) */
}

int main_bare(void)
{
    stm32_ppm_init_timer();
    syn_ppm_init(&s_ppm);

    uint32_t last_log_ms = syn_port_get_tick_ms();

    /* Simulated PPM pulse train sequence: Frame Sync followed by 6 channels */
    const uint16_t sample_pulses[] = {
        4000, /* Sync gap (>2700 us) */
        1500, /* Ch 0: Roll 1500 us */
        1480, /* Ch 1: Pitch 1480 us */
        1100, /* Ch 2: Throttle 1100 us */
        1520, /* Ch 3: Yaw 1520 us */
        2000, /* Ch 4: Aux 1 2000 us */
        1000  /* Ch 5: Aux 2 1000 us */
    };
    const size_t sample_count = sizeof(sample_pulses) / sizeof(sample_pulses[0]);
    size_t pulse_idx = 0;

    while (1) {
        /* Simulate processing incoming pulse widths from Timer Input Capture ISR */
        uint16_t pulse_us = sample_pulses[pulse_idx];
        pulse_idx = (pulse_idx + 1) % sample_count;

        if (syn_ppm_process_pulse(&s_ppm, pulse_us) == SYN_OK) {
            /* Complete multi-channel frame decoded successfully */
            uint16_t roll     = syn_ppm_get_channel(&s_ppm, 0);
            uint16_t pitch    = syn_ppm_get_channel(&s_ppm, 1);
            uint16_t throttle = syn_ppm_get_channel(&s_ppm, 2);
            uint16_t yaw      = syn_ppm_get_channel(&s_ppm, 3);
            (void)roll;
            (void)pitch;
            (void)throttle;
            (void)yaw;
        }

        uint32_t now_ms = syn_port_get_tick_ms();
        if (now_ms - last_log_ms >= 50) {
            last_log_ms = now_ms;
        }
    }

    return 0;
}
