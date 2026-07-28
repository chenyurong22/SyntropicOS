/**
 * @file main.c
 * @brief SyntropicOS Multi-Channel Software PWM STM32 HAL Example.
 *
 * Demonstrates software PWM pulse generation on standard GPIO pins (`syn_soft_pwm_service`),
 * 100-step resolution duty cycle control (`syn_soft_pwm_set_percent`), 10kHz timer ISR
 * integration (`TIM2_IRQHandler`), and smooth RGBW LED breathing effect using STM32 HAL drivers.
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Hardware Timer Handle */
extern TIM_HandleTypeDef htim2;

/* Pin Definitions for 4-Channel Software PWM (PA0..PA3) */
#define PWM_PORT GPIOA
#define CH_RED_PIN   GPIO_PIN_0
#define CH_GREEN_PIN GPIO_PIN_1
#define CH_BLUE_PIN  GPIO_PIN_2
#define CH_WHITE_PIN GPIO_PIN_3

#define NUM_PWM_CHANNELS 4

/* SyntropicOS Software PWM Channel Descriptors */
static SYN_SoftPWM soft_pwm_channels[NUM_PWM_CHANNELS];

/* Breathing Fader Application State */
typedef struct {
    uint8_t  duty_percent;
    int8_t   fade_direction;
    uint32_t last_fade_tick;
} Fader_State;

static Fader_State fader = {
    .duty_percent = 0,
    .fade_direction = 1,
    .last_fade_tick = 0
};

/**
 * @brief Hardware Timer 2 ISR (called at 10kHz / 100us interval).
 *
 * Services all 4 software PWM channels in a single pass.
 */
void TIM2_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_UPDATE) != RESET) {
        if (__HAL_TIM_GET_IT_SOURCE(&htim2, TIM_IT_UPDATE) != RESET) {
            __HAL_TIM_CLEAR_IT_FLAG(&htim2, TIM_FLAG_UPDATE);

            /* Service all 4 software PWM channels */
            syn_soft_pwm_service(soft_pwm_channels, NUM_PWM_CHANNELS);
        }
    }
}

/**
 * @brief Initialize Software PWM Channels and Hardware Timer.
 */
void soft_pwm_app_init(void)
{
    /* Initialize 4 Software PWM channels with 100-step resolution */
    syn_soft_pwm_init(&soft_pwm_channels[0], (SYN_GPIO_Pin)CH_RED_PIN,   100);
    syn_soft_pwm_init(&soft_pwm_channels[1], (SYN_GPIO_Pin)CH_GREEN_PIN, 100);
    syn_soft_pwm_init(&soft_pwm_channels[2], (SYN_GPIO_Pin)CH_BLUE_PIN,  100);
    syn_soft_pwm_init(&soft_pwm_channels[3], (SYN_GPIO_Pin)CH_WHITE_PIN, 100);

    /* Initial duty cycle values (Red = 100%, Green = 50%, Blue = 25%, White = 0%) */
    syn_soft_pwm_set_percent(&soft_pwm_channels[0], 100);
    syn_soft_pwm_set_percent(&soft_pwm_channels[1], 50);
    syn_soft_pwm_set_percent(&soft_pwm_channels[2], 25);
    syn_soft_pwm_set_percent(&soft_pwm_channels[3], 0);

    /* Start 10kHz Hardware Timer 2 Interrupt */
    HAL_TIM_Base_Start_IT(&htim2);
}

/**
 * @brief Periodic 20ms Breathing LED Fader Task (50Hz ramp update).
 */
void soft_pwm_app_task_20ms(void)
{
    uint32_t now = syn_port_get_tick_ms();

    if ((now - fader.last_fade_tick) >= 20U) {
        fader.last_fade_tick = now;

        /* Step breathing duty cycle up/down between 0% and 100% */
        if (fader.fade_direction > 0) {
            if (++fader.duty_percent >= 100) {
                fader.duty_percent = 100;
                fader.fade_direction = -1;
            }
        } else {
            if (fader.duty_percent == 0 || --fader.duty_percent == 0) {
                fader.duty_percent = 0;
                fader.fade_direction = 1;
            }
        }

        /* Update Red and White channels with complementary duty cycles */
        syn_soft_pwm_set_percent(&soft_pwm_channels[0], fader.duty_percent);
        syn_soft_pwm_set_percent(&soft_pwm_channels[3], 100 - fader.duty_percent);
    }
}
