/**
 * @file main.c
 * @brief STM32 Field-Oriented Control (FOC) BLDC Motor Example.
 *
 * Demonstrates fixed-point Field-Oriented Control (FOC) for 3-phase BLDC/PMSM motors:
 * - 10 kHz Hardware ADC/Timer ISR: 3-phase current sampling -> Clarke/Park -> Id/Iq PID -> Inv Park/Clarke -> SVPWM
 * - Sensorless Sliding Mode Observer (SMO) for rotor angle and speed estimation
 * - Background 100 Hz outer velocity control loop
 */

#include <stdbool.h>
#include <stdint.h>

#include "syntropic/control/syn_pid.h"
#include "syntropic/motor/syn_foc.h"
#include "syntropic/motor/syn_foc_observer.h"
#include "syntropic/port/syn_port_system.h"
#include "syntropic/util/syn_qmath.h"

static SYN_PID          s_id_pid;
static SYN_PID          s_iq_pid;
static SYN_PID          s_vel_pid;
static SYN_FOC_Observer s_observer;
static volatile q16_t   s_target_iq = 0;

static void stm32_sample_currents(SYN_FOC_ABC *currents)
{
    currents->a = Q16_FROM_FLOAT(0.5f);
    currents->b = Q16_FROM_FLOAT(-0.25f);
    currents->c = Q16_FROM_FLOAT(-0.25f);
}

static void stm32_apply_svpwm_duty(const SYN_FOC_ABC *duty)
{
    (void)duty;
}

/**
 * @brief 10 kHz Hardware Timer/ADC Interrupt Service Routine.
 * Executes hard real-time vector current control (100 us execution budget).
 */
void stm32_foc_current_loop_isr(void)
{
    /* Step 1: Read 3-phase currents */
    SYN_FOC_ABC phase_currents;
    stm32_sample_currents(&phase_currents);

    /* Step 2: Forward Clarke Transform (3-phase ABC -> 2-phase stationary AB) */
    SYN_FOC_AB i_ab;
    syn_foc_clarke(&phase_currents, &i_ab);

    /* Step 3: Estimate rotor angle theta via Sliding Mode Observer */
    q16_t theta = syn_foc_observer_update(&s_observer, &i_ab, &i_ab, 1);

    /* Step 4: Forward Park Transform (Stationary AB -> Rotating DQ) */
    SYN_FOC_DQ i_dq;
    syn_foc_park(&i_ab, theta, &i_dq);

    /* Step 5: Closed-loop Id and Iq PID current regulation */
    int32_t vd_out = syn_pid_update(&s_id_pid, 0, i_dq.d, 1);
    int32_t vq_out = syn_pid_update(&s_iq_pid, s_target_iq, i_dq.q, 1);

    SYN_FOC_DQ v_dq = {
        .d = (q16_t)vd_out,
        .q = (q16_t)vq_out
    };

    /* Step 6: Inverse Park Transform (Rotating DQ -> Stationary AB) */
    SYN_FOC_AB v_ab;
    syn_foc_inv_park(&v_dq, theta, &v_ab);

    /* Step 7: Inverse Clarke / Space Vector PWM (SVPWM) */
    SYN_FOC_ABC pwm_duty;
    syn_foc_inv_clarke(&v_ab, &pwm_duty);

    stm32_apply_svpwm_duty(&pwm_duty);
}

int main(void)
{
    /* Initialize Id, Iq, and velocity PID controllers */
    SYN_PID_Config id_cfg  = SYN_PID_GAINS(1.2f, 0.5f, 0.0f, 100, -Q16_ONE, Q16_ONE);
    SYN_PID_Config iq_cfg  = SYN_PID_GAINS(1.2f, 0.5f, 0.0f, 100, -Q16_ONE, Q16_ONE);
    SYN_PID_Config vel_cfg = SYN_PID_GAINS(0.5f, 0.1f, 0.0f, 100, -Q16_FROM_FLOAT(5.0f), Q16_FROM_FLOAT(5.0f));

    syn_pid_init(&s_id_pid, &id_cfg);
    syn_pid_init(&s_iq_pid, &iq_cfg);
    syn_pid_init(&s_vel_pid, &vel_cfg);

    /* Initialize Sliding Mode Observer */
    SYN_FOC_ObserverConfig obs_cfg = {
        .r                = Q16_FROM_FLOAT(0.5f),
        .l                = Q16_FROM_FLOAT(0.002f),
        .k_slide          = Q16_FROM_FLOAT(5.0f),
        .filter_cutoff_hz = Q16_FROM_FLOAT(200.0f)
    };
    syn_foc_observer_init(&s_observer, &obs_cfg);

    uint32_t last_10ms = syn_port_get_tick_ms();

    while (1) {
        /* Run 10 kHz FOC current loop */
        stm32_foc_current_loop_isr();

        /* Background 100 Hz Outer Velocity Loop (10 ms) */
        uint32_t now_ms = syn_port_get_tick_ms();
        if (now_ms - last_10ms >= 10) {
            last_10ms = now_ms;
            q16_t measured_speed = syn_foc_observer_get_speed(&s_observer);
            q16_t target_speed   = Q16_FROM_FLOAT(1500.0f); /* 1500 RPM target */

            int32_t iq_cmd = syn_pid_update(&s_vel_pid, target_speed, measured_speed, 10);
            s_target_iq    = (q16_t)iq_cmd;
        }
    }

    return 0;
}
