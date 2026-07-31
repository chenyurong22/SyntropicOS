/**
 * @file main.c
 * @brief STM32 HAL SIM800 GPRS Cellular Modem + Native SyntropicOS MQTT Example.
 *
 * Demonstrates non-blocking UART stream parsing via syn_at_parser to establish
 * a GPRS IP context on SIM800 and run SyntropicOS's native syn_mqtt client engine.
 */

#include "syntropic/log/syn_log.h"
#include "syntropic/net/syn_mqtt.h"
#include "syntropic/sched/syn_sched.h"
#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Or stm32f1xx_hal.h / stm32g4xx_hal.h */

#include <stdio.h>
#include <string.h>

/* ── Hardware Instances ─────────────────────────────────────────────────── */
extern UART_HandleTypeDef huart2; /* Modem USART handle */

/* ── Buffers & Contexts ─────────────────────────────────────────────────── */
static uint8_t modem_rx_backing[256];
static SYN_Stream modem_rx_stream;

static char at_line_buf[128];
static SYN_AtParser at_parser;

static uint8_t mqtt_rx_buf[256];
static uint8_t mqtt_tx_buf[256];
static SYN_MqttClient mqtt_client;

/* SIM800 Connection State */
typedef enum {
    MODEM_STATE_POWER_ON,
    MODEM_STATE_CHECK_AT,
    MODEM_STATE_CHECK_SIM,
    MODEM_STATE_CHECK_CSQ,
    MODEM_STATE_ATTACH_GPRS,
    MODEM_STATE_SET_APN,
    MODEM_STATE_BRINGUP_WIRELESS,
    MODEM_STATE_GET_IP,
    MODEM_STATE_CONNECT_TCP,
    MODEM_STATE_READY,
    MODEM_STATE_FAILED,
} ModemState;

static ModemState g_modem_state = MODEM_STATE_POWER_ON;
static SYN_Socket g_sim800_sock = SYN_SOCKET_INVALID;

/* ── STM32 USART Interrupt Handler ──────────────────────────────────────── */

/**
 * @brief Ingest incoming byte from STM32 USART ISR into SyntropicOS Stream.
 */
void sim800_uart_rx_isr(uint8_t byte)
{
    syn_stream_put(&modem_rx_stream, byte);
}

/* ── SIM800 Non-blocking Protothread Task ───────────────────────────────── */

static SYN_PT_Status sim800_modem_task(SYN_PT *pt, SYN_Task *task)
{
    (void)task;
    (void)g_sim800_sock;
    static SYN_Timeout timer;
    static SYN_AtRespType resp;

    PT_BEGIN(pt);

    /* 1. Send initial AT command */
    g_modem_state = MODEM_STATE_CHECK_AT;
    HAL_UART_Transmit(&huart2, (const uint8_t *)"AT\r\n", 4, 100);
    syn_timeout_start(&timer, 2000);

    PT_WAIT_UNTIL(pt, (resp = syn_at_parser_feed_stream(&at_parser, &modem_rx_stream)) ==
                          SYN_AT_RESP_OK ||
                      syn_timeout_expired(&timer));

    if (resp != SYN_AT_RESP_OK) {
        g_modem_state = MODEM_STATE_FAILED;
        PT_EXIT(pt);
    }

    /* 2. Verify SIM card insertion (AT+CPIN?) */
    g_modem_state = MODEM_STATE_CHECK_SIM;
    HAL_UART_Transmit(&huart2, (const uint8_t *)"AT+CPIN?\r\n", 10, 100);
    syn_timeout_start(&timer, 3000);

    PT_WAIT_UNTIL(pt, (resp = syn_at_parser_feed_stream(&at_parser, &modem_rx_stream)) ==
                          SYN_AT_RESP_LINE ||
                      syn_timeout_expired(&timer));

    /* 3. Attach GPRS service (AT+CGATT=1) */
    g_modem_state = MODEM_STATE_ATTACH_GPRS;
    HAL_UART_Transmit(&huart2, (const uint8_t *)"AT+CGATT=1\r\n", 12, 100);
    syn_timeout_start(&timer, 5000);

    PT_WAIT_UNTIL(pt, (resp = syn_at_parser_feed_stream(&at_parser, &modem_rx_stream)) ==
                          SYN_AT_RESP_OK ||
                      syn_timeout_expired(&timer));

    /* 4. Configure APN (AT+CSTT="CMNET") */
    g_modem_state = MODEM_STATE_SET_APN;
    HAL_UART_Transmit(&huart2, (const uint8_t *)"AT+CSTT=\"CMNET\"\r\n", 17, 100);
    syn_timeout_start(&timer, 3000);

    PT_WAIT_UNTIL(pt, (resp = syn_at_parser_feed_stream(&at_parser, &modem_rx_stream)) ==
                          SYN_AT_RESP_OK ||
                      syn_timeout_expired(&timer));

    /* 5. Bring up wireless connection (AT+CIICR) */
    g_modem_state = MODEM_STATE_BRINGUP_WIRELESS;
    HAL_UART_Transmit(&huart2, (const uint8_t *)"AT+CIICR\r\n", 10, 100);
    syn_timeout_start(&timer, 10000);

    PT_WAIT_UNTIL(pt, (resp = syn_at_parser_feed_stream(&at_parser, &modem_rx_stream)) ==
                          SYN_AT_RESP_OK ||
                      syn_timeout_expired(&timer));

    /* 6. Network connected! Set modem state to ready */
    g_modem_state = MODEM_STATE_READY;
    SYN_LOG_I("SIM800", "SIM800 GPRS Wireless Connection Established");

    PT_END(pt);
}

/* ── MQTT Callbacks ─────────────────────────────────────────────────────── */

static void on_mqtt_message(const char *topic, const uint8_t *payload, size_t len, void *ctx)
{
    (void)ctx;
    SYN_LOG_I("SIM800", "MQTT Rx [%s]: %.*s", topic, (int)len, (const char *)payload);
}

/* ── Application Main ───────────────────────────────────────────────────── */

int main(void)
{
    /* Initialize STM32 HAL Hardware Peripherals */
    HAL_Init();

    /* Initialize SyntropicOS Stream & Parser */
    syn_stream_init(&modem_rx_stream, modem_rx_backing, sizeof(modem_rx_backing));
    syn_at_parser_init(&at_parser, at_line_buf, sizeof(at_line_buf));

    /* Initialize SyntropicOS Native MQTT Client */
    syn_mqtt_init(&mqtt_client, "broker.hivemq.com", 1883, "stm32_sim800_client",
                  NULL, NULL, 60, mqtt_rx_buf, sizeof(mqtt_rx_buf),
                  mqtt_tx_buf, sizeof(mqtt_tx_buf));
    mqtt_client.on_message = on_mqtt_message;

    /* Create SyntropicOS Tasks & Scheduler */
    static SYN_Task tasks[2];
    static SYN_Sched sched;

    syn_task_create(&tasks[0], "sim800", sim800_modem_task, 0, NULL);
    syn_task_create(&tasks[1], "mqtt", syn_mqtt_task, 1, &mqtt_client);

    syn_sched_init(&sched, tasks, 2);

    /* Run Cooperative Scheduler Loop */
    for (;;) {
        syn_sched_run(&sched);
    }
}
