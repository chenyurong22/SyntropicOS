/**
 * @file main_bare.c
 * @brief STM32 6-Step BLDC Motor Commutation Example (Bare-Metal while(1) Polling Loop Variant).
 *
 * Demonstrates 6-step trapezoidal commutation for 3-phase BLDC motor:
 * - Direct EXTI Hall sensor pin sampling and commutation update
 * - Direct speed calculation & PWM duty update inside while(1) loop
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "syntropic/motor/syn_bldc_6step.h"
#include "syntropic/port/syn_port_system.h"

static SYN_BLDC_6Step s_bldc;

static void stm32_apply_phase_gates(const SYN_BLDC_PhaseOutputs *gates)
{
    /* On real STM32 hardware (TIM1/TIM8 Break and Dead-Time BDTR):
     * Phase U: High-Side TIM1_CH1 (PA8), Low-Side TIM1_CH1N (PB13)
     * Phase V: High-Side TIM1_CH2 (PA9), Low-Side TIM1_CH2N (PB14)
     * Phase W: High-Side TIM1_CH3 (PA10), Low-Side TIM1_CH3N (PB15)
     */
    (void)gates;
}

static uint8_t stm32_read_hall_sensors(void)
{
    /* Read GPIOC Pins PC6(H1), PC7(H2), PC8(H3) -> 3-bit value (1..6) */
    return 0b101; /* Simulated Hall sector 1 */
}

int main_bare(void)
{
    SYN_BLDC_Config cfg = {
        .pole_pairs    = 4,
        .pwm_frequency = 20000
    };
    syn_bldc_6step_init(&s_bldc, &cfg);
    syn_bldc_6step_set_duty(&s_bldc, 500); /* 50.0% PWM */
    syn_bldc_6step_start(&s_bldc);

    uint32_t last_100ms = syn_port_get_tick_ms();

    while (1) {
        /* Step A: Read Hall sensors & commute gates */
        uint8_t hall = stm32_read_hall_sensors();
        SYN_BLDC_PhaseOutputs gates;
        if (syn_bldc_6step_set_hall(&s_bldc, hall, &gates) == SYN_OK) {
            stm32_apply_phase_gates(&gates);
        }

        /* Step B: Periodically calculate RPM speed (every 100ms) */
        uint32_t now_ms = syn_port_get_tick_ms();
        if (now_ms - last_100ms >= 100) {
            last_100ms = now_ms;
            syn_bldc_6step_update_speed(&s_bldc, now_ms, 1500);
        }
    }

    return 0;
}
