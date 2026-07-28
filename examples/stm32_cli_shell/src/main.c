/**
 * @file main.c
 * @brief SyntropicOS Embedded Interactive CLI Shell STM32 HAL Example.
 *
 * Demonstrates character-by-character USART RX interrupt ingestion (`HAL_UART_RxCpltCallback`),
 * interactive line editing (`syn_cli_process_char`), statically registered custom shell commands
 * (`led`, `status`, `temp`), line buffering, prompt rendering (`stm32> `), and zero-malloc command
 * dispatch using STM32 HAL USART drivers (`HAL_UART_...`).
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

#include <stdio.h>
#include <string.h>

/* Hardware USART handle instance for console CLI */
extern UART_HandleTypeDef huart1;

/* Onboard LED GPIO Port & Pin */
#define LED_GPIO_PORT GPIOA
#define LED_GPIO_PIN  GPIO_PIN_5

/* Single byte interrupt buffer for USART RX */
static uint8_t cli_rx_byte;

/* SyntropicOS CLI Engine Context */
static SYN_CLI cli_instance;

/* Command Handler Functions Forward Declarations */
static int cmd_led(int argc, char *argv[]);
static int cmd_status(int argc, char *argv[]);
static int cmd_temp(int argc, char *argv[]);

/* Statically Registered Command Table */
static const SYN_CLI_Command shell_commands[] = {
    {"led",    "led <on|off|toggle> - Control onboard LED state", cmd_led},
    {"status", "status - Show MCU uptime, memory, and task status", cmd_status},
    {"temp",   "temp - Read internal CPU temperature telemetry", cmd_temp},
};

/**
 * @brief Command handler for `led <on|off|toggle>`.
 */
static int cmd_led(int argc, char *argv[])
{
    if (argc < 2) {
        syn_cli_printf(&cli_instance, "Usage: led <on|off|toggle>\r\n");
        return 1;
    }

    if (strcmp(argv[1], "on") == 0) {
        HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_SET);
        syn_cli_printf(&cli_instance, "LED turned ON\r\n");
    } else if (strcmp(argv[1], "off") == 0) {
        HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_RESET);
        syn_cli_printf(&cli_instance, "LED turned OFF\r\n");
    } else if (strcmp(argv[1], "toggle") == 0) {
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
        syn_cli_printf(&cli_instance, "LED toggled\r\n");
    } else {
        syn_cli_printf(&cli_instance, "Unknown argument '%s'. Use on, off, or toggle.\r\n", argv[1]);
        return 1;
    }

    return 0;
}

/**
 * @brief Command handler for `status`.
 */
static int cmd_status(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    uint32_t uptime_ms = syn_port_get_tick_ms();
    uint32_t uptime_sec = uptime_ms / 1000U;

    syn_cli_printf(&cli_instance, "\r\n=== STM32 System Status ===\r\n");
    syn_cli_printf(&cli_instance, "Kernel: SyntropicOS Bare-Metal C99\r\n");
    syn_cli_printf(&cli_instance, "Uptime: %lu seconds (%lu ms)\r\n", uptime_sec, uptime_ms);
    syn_cli_printf(&cli_instance, "LED State: %s\r\n",
                   (HAL_GPIO_ReadPin(LED_GPIO_PORT, LED_GPIO_PIN) == GPIO_PIN_SET) ? "ON" : "OFF");
    syn_cli_printf(&cli_instance, "===========================\r\n\r\n");

    return 0;
}

/**
 * @brief Command handler for `temp`.
 */
static int cmd_temp(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    /* Simulated MCU CPU die temperature reading (25.0 °C) */
    float temp_c = 25.4f;

    syn_cli_printf(&cli_instance, "CPU Temperature: %.2f degC\r\n", temp_c);
    return 0;
}

/**
 * @brief Initialize USART CLI application and arm RX interrupts.
 */
void cli_app_init(void)
{
    /* Initialize SyntropicOS CLI with command table and prompt */
    syn_cli_init(&cli_instance, shell_commands,
                 sizeof(shell_commands) / sizeof(shell_commands[0]), "stm32> ");

    syn_cli_set_echo(&cli_instance, true);

    /* Print welcome banner and initial prompt */
    syn_cli_printf(&cli_instance, "\r\n=========================================\r\n");
    syn_cli_printf(&cli_instance, " SyntropicOS Embedded USART CLI Shell\r\n");
    syn_cli_printf(&cli_instance, " Type 'help' for a list of available commands\r\n");
    syn_cli_printf(&cli_instance, "=========================================\r\n\r\n");

    syn_cli_print_prompt(&cli_instance);

    /* Arm USART1 interrupt for 1-byte character reception */
    HAL_UART_Receive_IT(&huart1, &cli_rx_byte, 1);
}

/**
 * @brief STM32 HAL USART Rx Interrupt Callback.
 *
 * Feeds each incoming character byte into SyntropicOS CLI process engine.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        /* Process character in CLI state machine */
        syn_cli_process_char(&cli_instance, (char)cli_rx_byte);

        /* Re-arm USART interrupt for next character */
        HAL_UART_Receive_IT(&huart1, &cli_rx_byte, 1);
    }
}
