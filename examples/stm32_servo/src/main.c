/**
 * @file main.c
 * @brief SyntropicOS RC Servo Motor STM32 HAL Example.
 *
 * Demonstrates non-blocking RC servo motor initialization (`syn_servo_init`), angle setting
 * (`syn_servo_set_angle`), smooth timed position ramping (`syn_servo_move_to`), and 50Hz PWM
 * timer register output (`TIM2->CCR1`) using STM32 HAL drivers.
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Hardware Timer Handle for 50Hz PWM Output */
extern TIM_HandleTypeDef htim2;

/* Servo Calibration Parameters */
#define SERVO_PULSE_MIN_US 1000 /* 1000us for 0 degrees */
#define SERVO_PULSE_MAX_US 2000 /* 2000us for 180 degrees */
#define SERVO_ANGLE_RANGE  180  /* 180 degree mechanical range */

/* SyntropicOS Servo Driver Handle */
static SYN_Servo servo;

/* Application Servo Sweep State Machine */
typedef struct {
    uint16_t target_angles[4];
    uint8_t  target_idx;
    uint32_t dwell_start_tick;
    bool     dwelling;
    uint32_t last_update_tick;
} Servo_State;

static Servo_State servo_state = {
    .target_angles = { 0, 90, 180, 90 }, /* Target angle sequence */
    .target_idx = 0,
    .dwell_start_tick = 0,
    .dwelling = false,
    .last_update_tick = 0
};

/**
 * @brief Update physical Timer PWM CCR1 register with microsecond pulse width.
 */
static void update_hardware_pwm_pulse(uint16_t pulse_us)
{
    /* Set Timer 2 Compare Register 1 (TIM2->CCR1) for 50Hz PWM pulse width */
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulse_us);
}

/**
 * @brief Initialize RC Servo Motor Driver and Hardware Timer PWM.
 */
void servo_app_init(void)
{
    /* Initialize Servo: 1000us min, 2000us max, 180 degrees */
    syn_servo_init(&servo, SERVO_PULSE_MIN_US, SERVO_PULSE_MAX_US, SERVO_ANGLE_RANGE);

    /* Start at 0 degrees */
    syn_servo_set_angle(&servo, 0);
    update_hardware_pwm_pulse(syn_servo_get_pulse_us(&servo));

    /* Start Hardware Timer 2 PWM Channel 1 Output */
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

    /* Start first smooth move to 90 degrees over 1000ms */
    syn_servo_move_to(&servo, servo_state.target_angles[0], 1000);
}

/**
 * @brief Periodic 20ms Servo Motion Control Task (50Hz frame update).
 */
void servo_app_task_20ms(void)
{
    uint32_t now = syn_port_get_tick_ms();

    /* 1. Step SyntropicOS Servo Motion Ramping Engine at 50Hz (20ms interval) */
    if ((now - servo_state.last_update_tick) >= 20U) {
        servo_state.last_update_tick = now;

        syn_servo_update(&servo);

        /* Update Timer PWM register with latest microsecond pulse width */
        uint16_t pulse_us = syn_servo_get_pulse_us(&servo);
        update_hardware_pwm_pulse(pulse_us);
    }

    /* 2. Sequence Next Target Angle when Servo reaches current target */
    if (syn_servo_at_target(&servo)) {
        if (!servo_state.dwelling) {
            /* Start 500ms dwell delay at target angle */
            servo_state.dwelling = true;
            servo_state.dwell_start_tick = now;
        } else if ((now - servo_state.dwell_start_tick) >= 500U) {
            servo_state.dwelling = false;

            /* Advance to next target angle in sequence */
            servo_state.target_idx = (servo_state.target_idx + 1) % 4;
            uint16_t next_angle = servo_state.target_angles[servo_state.target_idx];

            /* Move to next angle smoothly over 1000ms */
            syn_servo_move_to(&servo, next_angle, 1000);
        }
    }
}
