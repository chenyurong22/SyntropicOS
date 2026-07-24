/**
 * @file main.c
 * @brief SyntropicOS IR Remote Control (syn_ir) STM32 HAL Example.
 *
 * Demonstrates non-blocking IR decoding and encoding with STM32 HAL Timers:
 *   - Receiver: TIM Input Capture measures pulse durations in microseconds.
 *   - Transmitter: TIM PWM generates carrier pulses.
 */

#include "syntropic/proto/syn_ir.h"
#include "stm32f4xx_hal.h"  /* Or stm32f1xx_hal.h / stm32g4xx_hal.h */

extern TIM_HandleTypeDef htim2;  /* Input Capture Timer for IR Receiver */
extern TIM_HandleTypeDef htim3;  /* PWM Timer for IR Transmitter (38kHz) */

static SYN_IR_Decoder ir_decoder;

/**
 * @brief Initialize IR remote decoder.
 */
void ir_app_init(void)
{
    syn_ir_decoder_init(&ir_decoder);
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
}

/**
 * @brief STM32 HAL Timer Input Capture Interrupt Callback.
 *
 * Triggered on both rising and falling edges of IR Receiver GPIO (e.g. VS1838B).
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        static uint32_t last_capture = 0;
        uint32_t current_capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        
        uint32_t duration_us = (current_capture >= last_capture) ?
            (current_capture - last_capture) :
            (0xFFFFU - last_capture + current_capture + 1U);
        
        last_capture = current_capture;

        /* Check pin state: active low IR receiver pin */
        GPIO_PinState state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
        bool is_mark = (state == GPIO_PIN_RESET); /* LOW = Mark (Carrier ON) */

        SYN_IR_Frame rx_frame;
        if (syn_ir_decode_pulse(&ir_decoder, (uint16_t)duration_us, is_mark, &rx_frame)) {
            /* Successfully decoded IR frame! */
            (void)rx_frame;
        }
    }
}

/**
 * @brief Periodic main loop task.
 */
void ir_app_loop(void)
{
    SYN_IR_Frame rx_frame;
    if (syn_ir_decode_timeout(&ir_decoder, &rx_frame)) {
        /* Timeout decoded frame */
        (void)rx_frame;
    }
}

/**
 * @brief Transmit IR command using STM32 HAL PWM.
 */
void send_ir_remote_command(SYN_IR_Protocol proto, uint32_t addr, uint32_t cmd)
{
    SYN_IR_Frame tx_frame = {
        .protocol = proto,
        .address = addr,
        .command = cmd
    };

    SYN_IR_Pulse pulses[100];
    size_t pulse_count = 0;

    if (syn_ir_encode_frame(&tx_frame, pulses, 100, &pulse_count) == SYN_OK) {
        for (size_t i = 0; i < pulse_count; i++) {
            if (pulses[i].is_mark) {
                HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
            } else {
                HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
            }
            /* Delay in microseconds using timer or DWT */
            uint32_t start = DWT->CYCCNT;
            uint32_t ticks = pulses[i].duration_us * (SystemCoreClock / 1000000U);
            while ((DWT->CYCCNT - start) < ticks);
        }
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
    }
}
