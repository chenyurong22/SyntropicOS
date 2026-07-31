/**
 * @file    main.c
 * @brief   SyntropicOS Example — Bare-Metal STM32 BACnet MS/TP Smart Sensor Node
 *
 * Demonstrates:
 *   1. BACnet MS/TP Node & Object Database initialization.
 *   2. RS485 Half-Duplex Direction Control.
 *   3. Responding to Who-Is / ReadProperty BACnet APDU queries.
 *
 * Hardware:
 *   - Board: STM32F4 / STM32F1 Nucleo
 *   - UART:  USART2 (PA2=TX, PA3=RX, PA1=RS485 DE) @ 38400 8N1
 */

#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "syntropic/syntropic.h"
#include "syntropic/proto/syn_bacnet.h"
#include "port/stm32_hal/port_stm32_hal.h"

#define RS485_DE_PIN SYN_PORT_STM32_PIN(GPIOA, GPIO_PIN_1)
#define LOCAL_MAC_ADDR 12
#define DEVICE_INSTANCE_ID 123456

static SYN_BACnet_Node bacnet_node;
static UART_HandleTypeDef huart2;
static uint8_t rx_raw_buf[512];
static size_t rx_raw_len = 0;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void set_rs485_de(bool tx_enable);

int main(void)
{
    (void)rx_raw_buf;
    (void)rx_raw_len;
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    set_rs485_de(false);

    /* 1. Initialize BACnet MS/TP Node & Object Database */
    syn_bacnet_node_init(&bacnet_node, LOCAL_MAC_ADDR, DEVICE_INSTANCE_ID);

    /* Add Temperature & Humidity Analog Input objects */
    syn_bacnet_add_object(&bacnet_node, SYN_BACNET_OBJ_ANALOG_INPUT, 1, 22.5f, "Room Temperature");
    syn_bacnet_add_object(&bacnet_node, SYN_BACNET_OBJ_ANALOG_INPUT, 2, 45.0f, "Relative Humidity");

    syn_port_stm32_register_uart(0, &huart2);

    /* Print Welcome Banner over Console */
    const char *banner = "\r\nSyntropicOS STM32 BACnet MS/TP Sensor Node Initialized!\r\n"
                         "MAC Address: 12 | Device ID: 123456\r\n"
                         "Objects: Device (123456), AI:1 (Temp), AI:2 (Humidity)\r\n\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t *)banner, (uint16_t)strlen(banner), 1000);

    /* 2. Main Loop */
    while (1)
    {
        /* Simulate receiving a BACnet MS/TP Who-Is request */
        uint8_t simulated_who_is_payload[] = {0x10, SYN_BACNET_SERVICE_UNCONFIRMED_WHO_IS};
        uint8_t req_buf[64];
        size_t req_len = syn_bacnet_mstp_encode_frame(SYN_BACNET_MSTP_FRAME_DATA_NOT_EXPECTING_REPLY,
                                                      SYN_BACNET_BROADCAST_MAC, 1,
                                                      simulated_who_is_payload, sizeof(simulated_who_is_payload),
                                                      req_buf);

        SYN_BACnet_MSTP_Frame rx_frame;
        if (syn_bacnet_mstp_decode_frame(req_buf, req_len, &rx_frame))
        {
            SYN_BACnet_MSTP_Frame tx_frame;
            bool has_tx = false;

            if (syn_bacnet_node_process(&bacnet_node, &rx_frame, &tx_frame, &has_tx) == SYN_OK && has_tx)
            {
                uint8_t tx_buf[128];
                size_t tx_len = syn_bacnet_mstp_encode_frame(tx_frame.frame_type,
                                                             tx_frame.destination_mac,
                                                             tx_frame.source_mac,
                                                             tx_frame.payload,
                                                             tx_frame.data_len,
                                                             tx_buf);

                set_rs485_de(true);
                HAL_UART_Transmit(&huart2, tx_buf, (uint16_t)tx_len, 100);
                set_rs485_de(false);

                char log[128];
                int log_len = snprintf(log, sizeof(log), "[BACnet TX] Sent I-Am Response to MAC %u (%zu bytes)\r\n",
                                       tx_frame.destination_mac, tx_len);
                HAL_UART_Transmit(&huart2, (uint8_t *)log, (uint16_t)log_len, 100);
            }
        }

        HAL_Delay(2000);
    }
}

static void set_rs485_de(bool tx_enable)
{
    syn_gpio_write(RS485_DE_PIN, tx_enable ? SYN_GPIO_HIGH : SYN_GPIO_LOW);
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
    huart2.Init.BaudRate = 38400;
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

    syn_gpio_init(RS485_DE_PIN, SYN_GPIO_OUTPUT);

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
