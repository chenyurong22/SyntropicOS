/**
 * @file main.c
 * @brief SyntropicOS Real-Time Clock (RTC) & Calendar STM32 HAL Example.
 *
 * Demonstrates non-blocking Real-Time Clock date and time management (`syn_rtc`),
 * 1ms SysTick time accumulation, UTC Unix epoch timestamp conversions (`syn_rtc_to_epoch` /
 * `syn_rtc_from_epoch`), date-time validation (`syn_rtc_is_valid`), and USART command parsing.
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

#include <stdio.h>
#include <string.h>

/* USART Handle for Debug & Protocol Interface */
extern UART_HandleTypeDef huart1;

/* RTC & Serial Protocol State Structure */
typedef struct {
    uint32_t current_epoch_sec; /* Active UTC Unix epoch timestamp in seconds */
    uint32_t last_tick_ms;      /* Millisecond tick tracking */
    uint32_t subsecond_ms;      /* Sub-second millisecond accumulator */
    char rx_buffer[64];         /* Non-blocking USART RX buffer */
    uint8_t rx_idx;
} RTC_State;

static RTC_State rtc_app = {
    .current_epoch_sec = 1785153600U, /* Default starting epoch: 2026-07-28 12:00:00 UTC */
    .last_tick_ms = 0,
    .subsecond_ms = 0,
    .rx_idx = 0
};

/**
 * @brief Transmit string via USART1.
 */
static void uart_send_string(const char *str)
{
    HAL_UART_Transmit(&huart1, (const uint8_t *)str, (uint16_t)strlen(str), 100);
}

/**
 * @brief Process serial commands to read or set date and time parameters.
 */
static void process_serial_command(const char *cmd)
{
    if (strncmp(cmd, "GET_TIME", 8) == 0) {
        SYN_RTC_DateTime dt;
        syn_rtc_from_epoch(rtc_app.current_epoch_sec, &dt);

        char resp[64];
        snprintf(resp, sizeof(resp), "TIME=%04u-%02u-%02u %02u:%02u:%02u\r\n",
                 dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
        uart_send_string(resp);
    } else if (strncmp(cmd, "SET_TIME=", 9) == 0) {
        uint32_t y, m, d, hh, mm, ss;
        if (sscanf(cmd + 9, "%lu-%lu-%lu %lu:%lu:%lu", &y, &m, &d, &hh, &mm, &ss) == 6) {
            SYN_RTC_DateTime dt;
            dt.year   = (uint16_t)y;
            dt.month  = (uint8_t)m;
            dt.day    = (uint8_t)d;
            dt.hour   = (uint8_t)hh;
            dt.minute = (uint8_t)mm;
            dt.second = (uint8_t)ss;
            dt.subsecond = 0;

            if (syn_rtc_is_valid(&dt)) {
                rtc_app.current_epoch_sec = syn_rtc_to_epoch(&dt);
                rtc_app.subsecond_ms = 0;

                char resp[64];
                snprintf(resp, sizeof(resp), "OK TIME=%04u-%02u-%02u %02u:%02u:%02u\r\n",
                         dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
                uart_send_string(resp);
            } else {
                uart_send_string("ERROR INVALID_DATE_RANGE\r\n");
            }
        } else {
            uart_send_string("ERROR INVALID_FORMAT\r\n");
        }
    } else {
        uart_send_string("ERROR UNKNOWN_COMMAND\r\n");
    }
}

/**
 * @brief Initialize RTC Application.
 */
void rtc_app_init(void)
{
    rtc_app.last_tick_ms = syn_port_get_tick_ms();
    uart_send_string("SyntropicOS RTC & Perpetual Calendar Initialized.\r\n");
}

/**
 * @brief Periodic 1ms Task: Accumulates 1000ms SysTick ticks into seconds.
 */
void rtc_app_task_1ms(void)
{
    uint32_t now = syn_port_get_tick_ms();
    uint32_t dt = now - rtc_app.last_tick_ms;
    rtc_app.last_tick_ms = now;

    rtc_app.subsecond_ms += dt;

    /* Accumulate 1000ms -> increment 1 UTC epoch second */
    while (rtc_app.subsecond_ms >= 1000U) {
        rtc_app.subsecond_ms -= 1000U;
        rtc_app.current_epoch_sec++;
    }
}

/**
 * @brief Non-blocking USART RX poll task.
 */
void rtc_app_usart_rx_task(void)
{
    uint8_t rx_byte = 0;

    if (HAL_UART_Receive(&huart1, &rx_byte, 1, 0) == HAL_OK) {
        if (rx_byte == '\r' || rx_byte == '\n') {
            if (rtc_app.rx_idx > 0) {
                rtc_app.rx_buffer[rtc_app.rx_idx] = '\0';
                process_serial_command(rtc_app.rx_buffer);
                rtc_app.rx_idx = 0;
            }
        } else {
            if (rtc_app.rx_idx < sizeof(rtc_app.rx_buffer) - 1) {
                rtc_app.rx_buffer[rtc_app.rx_idx++] = (char)rx_byte;
            }
        }
    }
}
