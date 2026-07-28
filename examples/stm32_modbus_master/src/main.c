/**
 * @file    main.c
 * @brief   SyntropicOS Example — Bare-Metal STM32 Modbus RTU Master (RS485)
 *
 * Demonstrates a non-blocking Modbus RTU Master querying RS485 slave devices:
 *   1. Periodic Read Holding Registers (FC 0x03).
 *   2. Write Single Register (FC 0x06).
 *   3. RS485 Driver Enable (DE) pin control.
 *   4. Timeout & CRC error handling.
 *
 * Hardware:
 *   - Board: STM32F4 / STM32F1 Nucleo
 *   - UART:  USART2 (PA2=TX, PA3=RX, PA1=RS485 DE) @ 115200 8N1
 */

#include "stm32f4xx_hal.h"
#include "syntropic/syntropic.h"
#include "syntropic/proto/syn_modbus_master.h"
#include "port/stm32_hal/port_stm32_hal.h"

#define SLAVE_ADDR 1
#define RS485_DE_PIN SYN_PORT_STM32_PIN(GPIOA, GPIO_PIN_1)

static SYN_ModbusMaster mb_master;
static UART_HandleTypeDef huart2;
static uint8_t rx_byte;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void set_rs485_de(bool tx_enable);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    /* Set RS485 to Receive mode (DE = LOW) */
    set_rs485_de(false);

    /* 1. Initialize Modbus Master engine (500 ms timeout) */
    syn_modbus_master_init(&mb_master, 500);

    /* 2. Register UART handle & start IT reception */
    syn_port_stm32_register_uart(0, &huart2);
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);

    uint32_t last_poll_tick = 0;

    /* 3. Main Loop */
    while (1)
    {
        uint32_t now = HAL_GetTick();

        /* Process Master state machine */
        SYN_ModbusMaster_State state = syn_modbus_master_process(&mb_master, now);

        if (state == SYN_MB_MASTER_STATE_COMPLETE)
        {
            /* Transaction succeeded */
            if (mb_master.func_code == SYN_MODBUS_FC_READ_HOLDING)
            {
                /* Read completed: values stored in mb_master.read_data */
                // mb_master.read_data[0..count-1] contain register values
            }
        }
        else if (state == SYN_MB_MASTER_STATE_TIMEOUT)
        {
            /* Slave response timed out */
        }
        else if (state == SYN_MB_MASTER_STATE_ERROR)
        {
            /* Modbus exception or CRC error */
        }

        /* Issue Read Holding Registers query every 1000 ms if Master is idle */
        if (state == SYN_MB_MASTER_STATE_IDLE && (now - last_poll_tick) >= 1000)
        {
            last_poll_tick = now;

            /* Enable RS485 TX direction */
            set_rs485_de(true);

            /* Issue FC 0x03: Read 4 holding registers starting at address 0 from Slave 1 */
            if (syn_modbus_master_read_holding(&mb_master, SLAVE_ADDR, 0, 4) == SYN_OK)
            {
                /* Transmit request frame over UART */
                HAL_UART_Transmit(&huart2, mb_master.buf, mb_master.tx_len, 100);
            }

            /* Revert RS485 back to RX direction */
            set_rs485_de(false);
        }

        HAL_Delay(5);
    }
}

static void set_rs485_de(bool tx_enable)
{
    syn_gpio_write(RS485_DE_PIN, tx_enable ? SYN_GPIO_HIGH : SYN_GPIO_LOW);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        /* Feed received byte to Modbus Master engine */
        syn_modbus_master_feed(&mb_master, rx_byte);

        /* Re-arm interrupt */
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

    /* Configure RS485 DE Pin (PA1) */
    syn_gpio_init(RS485_DE_PIN, SYN_GPIO_OUTPUT);

    /* Configure UART2 Pins (PA2=TX, PA3=RX) */
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
