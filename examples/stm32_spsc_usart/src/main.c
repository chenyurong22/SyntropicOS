/**
 * @file    main.c
 * @brief   SyntropicOS Example — STM32 USART RX with Lock-Free SPSC Queue (syn_spsc_queue)
 *
 * Demonstrates lock-free structured message passing from a UART RX interrupt (producer)
 * to the main processing loop (consumer) using syn_spsc_queue.
 *
 * Hardware:
 *   - Board: STM32F4 / STM32F1 Nucleo / Discovery
 *   - UART:  USART2 (115200 8N1)
 *   - Pins:  PA2 (TX), PA3 (RX)
 */

#include "stm32f4xx_hal.h"
#include "syntropic/syntropic.h"
#include "syntropic/util/syn_spsc_queue.h"
#include "port/stm32_hal/port_stm32_hal.h"

#define QUEUE_CAPACITY 64

/** Structured UART Event Packet */
typedef struct {
    uint32_t timestamp_ms;
    uint8_t rx_byte;
} UART_RxEvent;

/* Statically allocated backing memory for SPSC queue */
static UART_RxEvent queue_backing_buf[QUEUE_CAPACITY];
static SYN_SPSC_Queue rx_spsc_queue;

static UART_HandleTypeDef huart2;
static uint8_t rx_byte_raw;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    /* 1. Initialize lock-free SPSC Queue */
    syn_spsc_queue_init(&rx_spsc_queue, queue_backing_buf, sizeof(UART_RxEvent), QUEUE_CAPACITY);

    /* 2. Register UART handle & start IT reception */
    syn_port_stm32_register_uart(0, &huart2);
    HAL_UART_Receive_IT(&huart2, &rx_byte_raw, 1);

    /* Print welcome message */
    const char *msg = "SyntropicOS syn_spsc_queue USART RX Example Ready!\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, (uint16_t)strlen(msg), 1000);

    UART_RxEvent event;

    /* 3. Non-blocking Main Processing Loop */
    while (1)
    {
        /* Pop structured event from lock-free queue */
        while (syn_spsc_queue_pop(&rx_spsc_queue, &event) == SYN_OK)
        {
            /* Echo event with timestamp */
            char out_str[64];
            int len = snprintf(out_str, sizeof(out_str), "[t=%lu ms] Recv: '%c' (0x%02X)\r\n",
                               (unsigned long)event.timestamp_ms,
                               (event.rx_byte >= 32 && event.rx_byte <= 126) ? event.rx_byte : '.',
                               event.rx_byte);
            if (len > 0)
            {
                HAL_UART_Transmit(&huart2, (uint8_t *)out_str, (uint16_t)len, 100);
            }
        }

        /* Non-blocking yield */
        HAL_Delay(5);
    }
}

/**
 * @brief UART RX Complete Callback (Runs in ISR context — Producer)
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        /* Build structured event */
        UART_RxEvent event = {
            .timestamp_ms = HAL_GetTick(),
            .rx_byte = rx_byte_raw
        };

        /* Lock-free push into SPSC Queue */
        syn_spsc_queue_push(&rx_spsc_queue, &event);

        /* Re-arm interrupt */
        HAL_UART_Receive_IT(&huart2, &rx_byte_raw, 1);
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
