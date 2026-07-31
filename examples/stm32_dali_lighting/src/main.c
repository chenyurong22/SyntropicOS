/**
 * @file    main.c
 * @brief   SyntropicOS Example — STM32 DALI (IEC 62386) Lighting Control Gear
 *
 * Demonstrates:
 *   1. DALI Control Gear (Slave Node) state initialization.
 *   2. Decoding 16-bit DALI Forward Frames (Short, Group, Broadcast).
 *   3. Direct Arc Power (DAPC) dimming & Standard DALI Commands.
 *   4. Formulating 8-bit DALI Backward Frame responses.
 *
 * Hardware:
 *   - Board: STM32F4 / STM32F1 Nucleo
 *   - LED Dimmer PWM: PA0 (TIM2_CH1)
 *   - UART Console:   USART2 (PA2/PA3 @ 115200 8N1)
 */

#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "syntropic/syntropic.h"
#include "syntropic/proto/syn_dali.h"
#include "port/stm32_hal/port_stm32_hal.h"

static SYN_DALI_SlaveState dali_slave;
static UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void update_led_pwm_level(uint8_t arc_level);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    /* 1. Configure DALI Control Gear Slave Parameters */
    SYN_DALI_SlaveConfig cfg = {
        .short_address = 5,          /* DALI Short Address 5 */
        .group_mask = (1U << 2),     /* Assigned to Group 2 */
        .min_level = 1,              /* Min physical level 1 */
        .max_level = 254,            /* Max physical level 254 */
        .power_on_level = 254,       /* Power-on level 100% */
        .system_failure_level = 254, /* System failure level 100% */
        .fade_rate = 7,
        .fade_time = 0
    };

    syn_dali_slave_init(&dali_slave, &cfg);

    /* Print Welcome Banner */
    const char *banner = "\r\nSyntropicOS STM32 DALI (IEC 62386) Control Gear Initialized!\r\n"
                         "DALI Short Address: 5 | Group: 2\r\n"
                         "Simulating DALI Forward Frame Execution...\r\n\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t *)banner, (uint16_t)strlen(banner), 1000);

    /* Set initial hardware LED PWM level */
    update_led_pwm_level(dali_slave.actual_level);

    /* 2. Simulate Sequence of DALI Commands from Master */
    uint16_t simulated_frames[] = {
        /* Direct Arc Power Control: Set Short Address 5 to Level 200 */
        syn_dali_encode_forward((5 << 1) | 0x00, 200),

        /* Direct Arc Power Control: Set Broadcast to Level 50 */
        syn_dali_encode_forward(0xFE, 50),

        /* Standard Command: STEP DOWN on Group 2 */
        syn_dali_encode_forward(((8 + 2) << 1) | 0x01, SYN_DALI_CMD_STEP_DOWN),

        /* Query Command: QUERY ACTUAL LEVEL on Short Address 5 */
        syn_dali_encode_forward((5 << 1) | 0x01, SYN_DALI_CMD_QUERY_ACTUAL_LEVEL),

        /* Standard Command: RECALL MAX on Broadcast */
        syn_dali_encode_forward(0xFF, SYN_DALI_CMD_RECALL_MAX),

        /* Standard Command: OFF on Short Address 5 */
        syn_dali_encode_forward((5 << 1) | 0x01, SYN_DALI_CMD_OFF)
    };

    size_t frame_count = sizeof(simulated_frames) / sizeof(simulated_frames[0]);

    /* 3. Main Loop */
    size_t frame_idx = 0;
    while (1)
    {
        if (frame_idx < frame_count)
        {
            uint16_t raw_16 = simulated_frames[frame_idx++];

            SYN_DALI_ForwardFrame req;
            if (syn_dali_decode_forward(raw_16, &req))
            {
                uint8_t resp_data = 0;
                bool has_resp = false;

                SYN_Status st = syn_dali_slave_process(&dali_slave, &req, &resp_data, &has_resp);
                if (st == SYN_OK)
                {
                    /* Update physical LED PWM hardware output */
                    update_led_pwm_level(dali_slave.actual_level);

                    char buf[128];
                    int len = snprintf(buf, sizeof(buf),
                                       "[DALI RX] Frame: 0x%04X | Direct: %d | Addr: %d | Arc Level: %u",
                                       raw_16, req.is_direct, req.address, dali_slave.actual_level);
                    HAL_UART_Transmit(&huart2, (uint8_t *)buf, (uint16_t)len, 200);

                    if (has_resp)
                    {
                        uint8_t back_frame = syn_dali_encode_backward(resp_data);
                        len = snprintf(buf, sizeof(buf), " -> [DALI TX Response]: 0x%02X (%u)", back_frame, resp_data);
                        HAL_UART_Transmit(&huart2, (uint8_t *)buf, (uint16_t)len, 200);
                    }

                    const char *nl = "\r\n";
                    HAL_UART_Transmit(&huart2, (uint8_t *)nl, (uint16_t)strlen(nl), 100);
                }
            }

            HAL_Delay(1000);
        }
        else
        {
            HAL_Delay(100);
        }
    }
}

/**
 * @brief Update hardware PWM output for LED dimming control
 */
static void update_led_pwm_level(uint8_t arc_level)
{
    /* Map DALI Arc Level (0..254) to PWM duty cycle (0..1000) */
    uint32_t pwm_duty = ((uint32_t)arc_level * 1000U) / 254U;
    (void)pwm_duty;
    /* In hardware: __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pwm_duty); */
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
