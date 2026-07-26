/**
 * @file main.c
 * @brief STM32 Embedded JSON Parsing & Serialization Example.
 *
 * Demonstrates zero-allocation JSON parsing and encoding on STM32 (Cortex-M) targets:
 * - Parsing incoming JSON strings in-place without dynamic heap allocation (malloc/free)
 * - Querying string, integer, boolean, and nested object fields using key-paths
 * - Formatting outgoing JSON telemetry payloads using syn_json_write
 * - Transmitting JSON responses via STM32 UART HAL
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "syntropic/util/syn_json_read.h"
#include "syntropic/util/syn_json_write.h"

/* ── STM32 Hardware Dummy UART Registers (Bare-Metal Stub) ────────────── */
#define USART1_BASE 0x40011000U

typedef struct {
    volatile uint32_t SR;   /* Status register           */
    volatile uint32_t DR;   /* Data register             */
    volatile uint32_t BRR;  /* Baud rate register        */
    volatile uint32_t CR1;  /* Control register 1        */
    volatile uint32_t CR2;  /* Control register 2        */
    volatile uint32_t CR3;  /* Control register 3        */
} STM32_USART_TypeDef;

#define STM32_USART1 ((STM32_USART_TypeDef *)USART1_BASE)
#define USART_SR_TXE (1U << 7) /* Transmit Data Register Empty */

static void stm32_uart_send_char(char c)
{
    /* On real STM32 hardware:
     * while (!(STM32_USART1->SR & USART_SR_TXE));
     * STM32_USART1->DR = (uint32_t)c;
     */
    (void)c;
}

static void stm32_uart_send_string(const char *str)
{
    while (*str) {
        stm32_uart_send_char(*str++);
    }
}

/* ── Example 1: In-Place Zero-Allocation JSON Parsing ──────────────────── */

static void demo_json_parsing(void)
{
    stm32_uart_send_string("\r\n=== 1. STM32 In-Place JSON Parsing Demo ===\r\n");

    /* Mutable buffer containing incoming JSON packet from UART/network */
    char rx_json[] = "{\r\n"
                     "  \"dev_id\": \"STM32-F407-Node1\",\r\n"
                     "  \"status\": {\r\n"
                     "    \"active\": true,\r\n"
                     "    \"uptime_s\": 3600\r\n"
                     "  },\r\n"
                     "  \"sensor\": {\r\n"
                     "    \"temp_c\": 24,\r\n"
                     "    \"humidity\": 58\r\n"
                     "  }\r\n"
                     "}";

    SYN_JsonReader reader;
    bool parsed = syn_json_parse(&reader, rx_json, strlen(rx_json));

    if (!parsed) {
        stm32_uart_send_string("ERROR: Failed to parse JSON document!\r\n");
        return;
    }

    stm32_uart_send_string("JSON successfully parsed in-place!\r\n");

    /* Extract top-level string field */
    char dev_id[32] = {0};
    if (syn_json_get_str(&reader, "dev_id", dev_id, sizeof(dev_id))) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Device ID: %s\r\n", dev_id);
        stm32_uart_send_string(msg);
    }

    /* Extract nested boolean field via dot notation */
    bool active = false;
    if (syn_json_get_bool(&reader, "status.active", &active)) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Status Active: %s\r\n", active ? "true" : "false");
        stm32_uart_send_string(msg);
    }

    /* Extract nested integer fields */
    int32_t temp = 0;
    int32_t humidity = 0;
    if (syn_json_get_int(&reader, "sensor.temp_c", &temp) &&
        syn_json_get_int(&reader, "sensor.humidity", &humidity)) {
        char msg[80];
        snprintf(msg, sizeof(msg), "Telemetry -> Temp: %ld C, Humidity: %ld%%\r\n", (long)temp, (long)humidity);
        stm32_uart_send_string(msg);
    }
}

/* ── Example 2: Zero-Allocation JSON Formatting & Encoding ─────────────── */

static void demo_json_serialization(void)
{
    stm32_uart_send_string("\r\n=== 2. STM32 JSON Serialization Demo ===\r\n");

    char tx_buffer[256];
    SYN_JsonWriter writer;
    syn_json_init(&writer, tx_buffer, sizeof(tx_buffer));

    /* Build outgoing telemetry JSON payload */
    syn_json_obj_open(&writer);
    syn_json_key_str(&writer, "node", "STM32F4-Gateway");
    syn_json_key_int(&writer, "vref_mv", 3300);
    syn_json_key_bool(&writer, "ready", true);

    syn_json_key(&writer, "metrics");
    syn_json_obj_open(&writer);
    syn_json_key_int(&writer, "cpu_load_pct", 12);
    syn_json_key_int(&writer, "free_ram_bytes", 48120);
    syn_json_obj_close(&writer);

    syn_json_obj_close(&writer);

    if (writer.overflow) {
        stm32_uart_send_string("ERROR: JSON buffer overflowed!\r\n");
        return;
    }

    stm32_uart_send_string("Generated JSON Payload:\r\n");
    stm32_uart_send_string(tx_buffer);
    stm32_uart_send_string("\r\n");
}

/* ── Main Entry Point ───────────────────────────────────────────────────── */

int main(void)
{
    stm32_uart_send_string("SyntropicOS STM32 Embedded JSON Example Started\r\n");

    /* Execute JSON Parsing & Serialization Demos */
    demo_json_parsing();
    demo_json_serialization();

    stm32_uart_send_string("\r\n=== STM32 JSON Demo Complete ===\r\n");

    while (1) {
        /* Application main loop */
    }

    return 0;
}
