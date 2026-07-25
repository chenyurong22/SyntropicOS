/**
 * @file main.c
 * @brief SyntropicOS DMX512-A Slave Receiver STM32 HAL Example.
 *
 * Demonstrates how to handle DMX512 Break framing errors and data reception
 * on STM32 USART peripherals using HAL callbacks.
 */

#include "syntropic/proto/syn_dmx512.h"
#include "stm32f7xx_hal.h"  /* Or stm32f4xx_hal.h / stm32g4xx_hal.h */

extern UART_HandleTypeDef huart3;

static SYN_DMX512_Slave dmx_slave;
static uint8_t uart_rx_byte;

/**
 * @brief Initialize DMX512 Slave Receiver and start UART reception.
 */
void dmx_app_init(void)
{
    /* Initialize DMX512 slave starting at DMX Address 1, footprint 4 channels */
    syn_dmx512_slave_init(&dmx_slave, 1, 4);

    /* Start 1-byte interrupt-driven UART reception */
    HAL_UART_Receive_IT(&huart3, &uart_rx_byte, 1);
}

/**
 * @brief STM32 HAL UART Error Callback.
 *
 * CRITICAL: When a DMX512 Break signal arrives, the line is held LOW for >88us,
 * triggering a Framing Error (HAL_UART_ERROR_FE). STM32 HAL invokes ErrorCallback
 * and STOPS reception! We must call rx_break, clear error flags, and re-arm Receive_IT.
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) {
        uint32_t err = HAL_UART_GetError(huart);

        if (err & HAL_UART_ERROR_FE) {
            /* Framing Error = DMX512 BREAK Condition! */
            syn_dmx512_slave_rx_break(&dmx_slave);
        }

        /* Clear all USART error flags */
        __HAL_UART_CLEAR_PEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_OREFLAG(huart);

        /* Re-enable 1-byte IT reception */
        HAL_UART_Receive_IT(huart, &uart_rx_byte, 1);
    }
}

/**
 * @brief STM32 HAL UART Rx Complete Callback.
 *
 * Called for each valid incoming DMX channel data byte.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) {
        /* Process byte in DMX slave FSM */
        syn_dmx512_slave_rx_byte(&dmx_slave, uart_rx_byte);

        /* Check if new full frame payload is available */
        if (syn_dmx512_slave_is_updated(&dmx_slave)) {
            uint8_t red   = syn_dmx512_slave_get_channel(&dmx_slave, 0);
            uint8_t green = syn_dmx512_slave_get_channel(&dmx_slave, 1);
            uint8_t blue  = syn_dmx512_slave_get_channel(&dmx_slave, 2);
            uint8_t dim   = syn_dmx512_slave_get_channel(&dmx_slave, 3);

            (void)red; (void)green; (void)blue; (void)dim;
        }

        /* Re-enable 1-byte IT reception */
        HAL_UART_Receive_IT(huart, &uart_rx_byte, 1);
    }
}
