/**
 * @file main.c
 * @brief STM32 HAL XCP (Universal Measurement and Calibration Protocol v1.x) over USART Example.
 */

#include "syntropic/proto/syn_xcp.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* STM32 HAL USART Header Mock/Includes for standalone compile */
#if defined(STM32F4xx) || defined(STM32F7xx) || defined(STM32G4xx)
#include "stm32_hal.h"
#else
typedef struct {
    void *Instance;
} UART_HandleTypeDef;

#define HAL_OK 0x00U
#endif

#define XCP_STATION_ID 0x0001U
#define XCP_UART_BUF_SIZE 256U

static SYN_XCP_Slave g_xcp_slave;
extern UART_HandleTypeDef huart1;

static uint8_t g_rx_byte;
static uint8_t g_cto_buf[8];
static uint8_t g_cto_idx = 0;

void xcp_usart_init(void)
{
    syn_xcp_init(&g_xcp_slave, XCP_STATION_ID);
}

void xcp_usart_loop(uint32_t dt_ms)
{
    (void)dt_ms;

    /* Service periodic XCP DAQ sampling event over USART */
    uint8_t daq_dto[8];
    uint8_t list_idx = 0;
    uint8_t odt_idx = 0;

    if (syn_xcp_service_daq(&g_xcp_slave, 0U, daq_dto, &list_idx, &odt_idx)) {
        /* Transmit 8-byte DAQ packet over UART */
        /* HAL_UART_Transmit(&huart1, daq_dto, 8U, 10U); */
    }
}

/**
 * @brief STM32 HAL UART Rx Interrupt Callback for XCP CTO frame ingestion.
 */
void HAL_UART_RxCpltCallback_XCP(UART_HandleTypeDef *huart)
{
    (void)huart;

    g_cto_buf[g_cto_idx++] = g_rx_byte;

    if (g_cto_idx >= 8U) {
        g_cto_idx = 0U;

        uint8_t dto_resp[8];
        if (syn_xcp_process_cto(&g_xcp_slave, g_cto_buf, dto_resp)) {
            /* Transmit 8-byte CTO response packet over UART */
            /* HAL_UART_Transmit(&huart1, dto_resp, 8U, 10U); */
        }
    }

    /* Re-arm single-byte UART Rx interrupt */
    /* HAL_UART_Receive_IT(&huart1, &g_rx_byte, 1U); */
}

int main(void)
{
    xcp_usart_init();

    while (1) {
        xcp_usart_loop(10U);
    }
    return 0;
}
