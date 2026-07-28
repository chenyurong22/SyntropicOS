/**
 * @file    main.c
 * @brief   SyntropicOS Example — STM32 USART RX with ISR-Safe Ring Buffer (syn_ringbuf)
 *
 * Demonstrates non-blocking UART byte reception on STM32 using syn_ringbuf.
 * The UART interrupt handler pushes bytes to the ring buffer, and the main
 * loop consumes and echoes them back.
 *
 * Hardware:
 *   - Board: STM32F4 / STM32F1 Nucleo / Discovery
 *   - UART:  USART2 (115200 8N1)
 *   - Pins:  PA2 (TX), PA3 (RX)
 */

#include "stm32f4xx_hal.h"
#include "syntropic/syntropic.h"
#include "syntropic/util/syn_ringbuf.h"
#include "port/stm32_hal/port_stm32_hal.h"

#define RX_BUF_SIZE 128

/* Statically allocated backing memory and control block */
static uint8_t rx_backing_buf[RX_BUF_SIZE];
static SYN_RingBuf rx_ringbuf;

static UART_HandleTypeDef huart2;
static uint8_t rx_byte;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    /* 1. Initialize syn_ringbuf */
    syn_ringbuf_init(&rx_ringbuf, rx_backing_buf, sizeof(rx_backing_buf));

    /* 2. Register UART handle and start interrupt reception */
    syn_port_stm32_register_uart(0, &huart2);
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);

    /* Print welcome message */
    const char *msg = "SyntropicOS syn_ringbuf USART RX Example Ready!\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, (uint16_t)strlen(msg), 1000);

    uint8_t byte;
    uint8_t line_buf[64];
    size_t line_len = 0;

    /* 3. Non-blocking Main Loop */
    while (1)
    {
        /* Consume bytes from ring buffer */
        while (syn_ringbuf_get(&rx_ringbuf, &byte))
        {
            /* Echo character back */
            HAL_UART_Transmit(&huart2, &byte, 1, 10);

            /* Buffer until newline '\r' or '\n' */
            if (byte == '\r' || byte == '\n')
            {
                if (line_len > 0)
                {
                    line_buf[line_len] = '\0';
                    /* Process command line */
                    const char *prefix = "\r\n[Processed Line]: ";
                    HAL_UART_Transmit(&huart2, (uint8_t *)prefix, (uint16_t)strlen(prefix), 100);
                    HAL_UART_Transmit(&huart2, line_buf, (uint16_t)line_len, 100);
                    const char *newline = "\r\n";
                    HAL_UART_Transmit(&huart2, (uint8_t *)newline, (uint16_t)strlen(newline), 100);

                    line_len = 0;
                }
            }
            else if (line_len < sizeof(line_buf) - 1)
            {
                line_buf[line_len++] = byte;
            }
        }

        /* Non-blocking delay to let lower priority tasks run */
        HAL_Delay(5);
    }
}

/**
 * @brief UART RX Complete Callback (Runs in ISR context)
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        /* Push received byte into ring buffer (ISR producer) */
        syn_ringbuf_put(&rx_ringbuf, rx_byte);

        /* Re-arm interrupt for next byte */
        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}
