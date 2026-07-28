/**
 * @file main.c
 * @brief SyntropicOS 4-Channel Capacitive Touch Sensing STM32 HAL Example.
 *
 * Demonstrates non-blocking capacitive touch button sampling (`syn_touch_feed_sample`),
 * baseline noise floor calibration (`syn_touch_calibrate`), threshold delta detection,
 * and touch indicator LED output using STM32 HAL ADC drivers (`HAL_ADC_...`).
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Hardware Handles for ADC and GPIO */
extern ADC_HandleTypeDef hadc1;

/* Pin Definitions for Touch Indicator LED */
#define LED_PORT GPIOB
#define LED_PIN  GPIO_PIN_0

/* Capacitive Touch Threshold Configuration */
#define TOUCH_THRESHOLD_DELTA 30 /* ADC delta threshold for touch press */
#define TOUCH_NUM_KEYS 4

/* SyntropicOS Touch Context Handles (4 Touch Keys) */
static SYN_Touch touch_keys[TOUCH_NUM_KEYS];

/* Active Touch Status */
typedef struct {
    bool key_pressed[TOUCH_NUM_KEYS];
    uint16_t raw_adc[TOUCH_NUM_KEYS];
    uint16_t baselines[TOUCH_NUM_KEYS];
} Touch_Status;

static Touch_Status touch_status;

/**
 * @brief Initialize 4 Capacitive Touch Key channels and calibrate baseline.
 */
void touch_app_init(void)
{
    /* Initialize 4 touch keys on ADC Channels PA0..PA3 */
    for (uint8_t i = 0; i < TOUCH_NUM_KEYS; i++) {
        syn_touch_init(&touch_keys[i], (SYN_GPIO_Pin)i, TOUCH_THRESHOLD_DELTA);

        /* Set initial baseline capacitance reading (100 raw ADC counts) */
        syn_touch_calibrate(&touch_keys[i], 100);
    }
}

/**
 * @brief Read 4 ADC channels corresponding to touch pads.
 */
static bool sample_touch_adc_channels(uint16_t *raw_buf)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    uint32_t channels[4] = {ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3};

    for (uint8_t i = 0; i < 4; i++) {
        sConfig.Channel = channels[i];
        sConfig.Rank = 1;
        sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;

        if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) return false;

        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) return false;

        raw_buf[i] = (uint16_t)HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
    }
    return true;
}

/**
 * @brief Periodic 10ms Touch Scanning Task (100Hz scan rate).
 */
void touch_app_task_10ms(void)
{
    uint16_t raw_samples[TOUCH_NUM_KEYS];
    bool any_key_active = false;

    if (sample_touch_adc_channels(raw_samples)) {
        for (uint8_t i = 0; i < TOUCH_NUM_KEYS; i++) {
            /* Feed raw ADC charge / voltage reading into SyntropicOS touch engine */
            syn_touch_feed_sample(&touch_keys[i], raw_samples[i]);

            /* Query touch press state */
            bool is_pressed = syn_touch_is_pressed(&touch_keys[i]);
            touch_status.key_pressed[i] = is_pressed;
            touch_status.raw_adc[i]     = raw_samples[i];
            touch_status.baselines[i]   = touch_keys[i].baseline;

            if (is_pressed) {
                any_key_active = true;
            }
        }

        /* Control Touch Indicator LED */
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, any_key_active ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

/**
 * @brief Periodic 5000ms Environmental Baseline Calibration Task.
 *
 * Slowly updates baseline capacitance to account for temperature & humidity drift.
 */
void touch_baseline_calibration_task_5s(void)
{
    for (uint8_t i = 0; i < TOUCH_NUM_KEYS; i++) {
        /* Only update baseline if key is not currently pressed */
        if (!syn_touch_is_pressed(&touch_keys[i])) {
            /* EMA baseline drift adjustment: baseline = (9 * baseline + raw) / 10 */
            uint16_t new_baseline = (uint16_t)(((uint32_t)touch_keys[i].baseline * 9 + touch_status.raw_adc[i]) / 10);
            syn_touch_calibrate(&touch_keys[i], new_baseline);
        }
    }
}
