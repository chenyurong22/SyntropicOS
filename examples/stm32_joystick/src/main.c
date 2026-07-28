/**
 * @file main.c
 * @brief SyntropicOS Dual-Axis Analog Joystick STM32 HAL Example.
 *
 * Demonstrates 12-bit ADC dual-channel sampling (VRX on PA0, VRY on PA1),
 * deadband noise suppression, normalized -100%..+100% position scaling,
 * 8-way directional classification, and integrated push-button handling using STM32 HAL ADC drivers.
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Hardware Handles for ADC and GPIO */
extern ADC_HandleTypeDef hadc1;

/* Pin Definitions */
#define JOY_SW_PORT GPIOA
#define JOY_SW_PIN  GPIO_PIN_2

/* ADC Parameters (12-bit ADC: 0..4095) */
#define ADC_FULL_SCALE 4095
#define ADC_CENTER     2048
#define ADC_DEADBAND   150

/* SyntropicOS Joystick Engine Context */
static SYN_Joystick joystick;

/* Active Telemetry Values */
typedef struct {
    int16_t x_pct;          /* X-Axis Percentage (-100% to +100%) */
    int16_t y_pct;          /* Y-Axis Percentage (-100% to +100%) */
    SYN_JoystickDir dir;    /* 8-Way Directional Enum */
    bool button_pressed;    /* Digital Push Button State */
} Joystick_Telemetry;

static Joystick_Telemetry joy_status;

/**
 * @brief Initialize Joystick Driver parameters.
 */
void joystick_app_init(void)
{
    /* Initialize joystick with 2048 center, 4095 max ADC, and 150 deadband */
    syn_joystick_init(&joystick, ADC_CENTER, ADC_CENTER, ADC_FULL_SCALE, ADC_DEADBAND);
}

/**
 * @brief Sample ADC channels for X (VRX) and Y (VRY) axes.
 * @param out_x Raw 12-bit ADC value for X axis.
 * @param out_y Raw 12-bit ADC value for Y axis.
 */
static bool sample_joystick_adc(uint16_t *out_x, uint16_t *out_y)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    /* 1. Read Channel 0 (PA0 - VRX) */
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) return false;

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) return false;
    *out_x = (uint16_t)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    /* 2. Read Channel 1 (PA1 - VRY) */
    sConfig.Channel = ADC_CHANNEL_1;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) return false;

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) return false;
    *out_y = (uint16_t)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    return true;
}

/**
 * @brief Periodic 20ms Joystick Processing Task (50Hz sample rate).
 */
void joystick_app_task_20ms(void)
{
    uint16_t raw_x = 0;
    uint16_t raw_y = 0;

    if (sample_joystick_adc(&raw_x, &raw_y)) {
        /* Read digital button pin (Active Low with internal pull-up) */
        bool btn_pressed = (HAL_GPIO_ReadPin(JOY_SW_PORT, JOY_SW_PIN) == GPIO_PIN_RESET);

        /* Feed raw ADC samples into SyntropicOS Joystick state machine */
        syn_joystick_feed_adc(&joystick, raw_x, raw_y, btn_pressed);

        /* Extract normalized percentage scaling & directional classification */
        joy_status.x_pct = syn_joystick_get_x_pct(&joystick);
        joy_status.y_pct = syn_joystick_get_y_pct(&joystick);
        joy_status.dir   = syn_joystick_get_dir(&joystick);
        joy_status.button_pressed = joystick.button_pressed;
    }
}
