/**
 * @file main.c
 * @brief SyntropicOS Stepper Motor Motion Control STM32 HAL Example.
 *
 * Demonstrates non-blocking stepper motor motion control (`syn_stepper_init`),
 * speed and acceleration ramping (`syn_stepper_set_speed`), absolute position moves
 * (`syn_stepper_move_to`), and enable pin control using STM32 HAL drivers.
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Hardware Timer Handle */
extern TIM_HandleTypeDef htim2;

/* Pin Definitions for STEP, DIR, and EN Pins */
#define STEPPER_PORT GPIOA
#define STEP_PIN GPIO_PIN_0
#define DIR_PIN  GPIO_PIN_1
#define EN_PIN   GPIO_PIN_2

/* SyntropicOS Stepper Motor Driver Handle */
static SYN_Stepper stepper;

/* Application Motion Sequencing State */
typedef struct {
    int32_t target_positions[4];
    uint8_t current_target_idx;
    uint32_t dwell_start_tick;
    bool dwelling;
} Motion_State;

static Motion_State motion = {
    .target_positions = { 3200, 0, 6400, -3200 }, /* Target step positions */
    .current_target_idx = 0,
    .dwell_start_tick = 0,
    .dwelling = false
};

/**
 * @brief Hardware Timer 2 ISR (called at 10kHz / 100us interval).
 *
 * Steps the SyntropicOS Stepper Motor motion profile.
 */
void TIM2_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_UPDATE) != RESET) {
        if (__HAL_TIM_GET_IT_SOURCE(&htim2, TIM_IT_UPDATE) != RESET) {
            __HAL_TIM_CLEAR_IT_FLAG(&htim2, TIM_FLAG_UPDATE);

            /* Step Stepper Motion State Machine & update outputs */
            syn_stepper_tick(&stepper);
        }
    }
}

/**
 * @brief Initialize Stepper Motor Driver and Hardware Timer.
 */
void stepper_app_init(void)
{
    /* Initialize Stepper on STEP (PA0) and DIR (PA1) */
    syn_stepper_init(&stepper, (SYN_GPIO_Pin)STEP_PIN, (SYN_GPIO_Pin)DIR_PIN);

    /* Set EN pin on PA2 (Active Low) */
    syn_stepper_set_enable_pin(&stepper, (SYN_GPIO_Pin)EN_PIN, true);
    syn_stepper_enable(&stepper, true);

    /* Set maximum speed (2000 SPS) and acceleration (1000 SPS^2) */
    syn_stepper_set_speed(&stepper, 2000, 1000);

    /* Start initial move to first target position (3,200 steps = 1 full revolution) */
    syn_stepper_move_to(&stepper, motion.target_positions[0]);

    /* Start 10kHz Hardware Timer 2 Interrupt */
    HAL_TIM_Base_Start_IT(&htim2);
}

/**
 * @brief Periodic Motion Sequence Controller Task (called every 20ms).
 */
void stepper_app_task_20ms(void)
{
    uint32_t now = syn_port_get_tick_ms();

    /* Check if current move completed */
    if (!syn_stepper_is_moving(&stepper)) {
        if (!motion.dwelling) {
            /* Start 500ms dwell delay at target position */
            motion.dwelling = true;
            motion.dwell_start_tick = now;
        } else if ((now - motion.dwell_start_tick) >= 500U) {
            motion.dwelling = false;

            /* Advance to next target position */
            motion.current_target_idx = (motion.current_target_idx + 1) % 4;
            int32_t next_pos = motion.target_positions[motion.current_target_idx];

            syn_stepper_move_to(&stepper, next_pos);
        }
    }
}
