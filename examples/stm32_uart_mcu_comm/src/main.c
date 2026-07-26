/**
 * @file main.c
 * @brief STM32 MCU-to-MCU UART Communication Example (Single-byte RX Interrupt, No DMA).
 *
 * Demonstrates bi-directional framed communication between a Master MCU (Node 0x01)
 * and a Slave MCU (Node 0x02) over USART TTL / RS232 using SyntropicOS syn_cobs
 * (COBS framing with 0x00 delimiter) and syn_router (addressed packet dispatch with ACKs).
 */

#include "stm32f4xx_hal.h" /* Adjust header for target (stm32f7xx_hal.h, stm32g4xx_hal.h, etc.) */
#include "syntropic/syntropic.h"
#include "syntropic/net/syn_router.h"
#include "syntropic/proto/syn_cobs.h"

/* ── Node IDs & Application Message Types ──────────────────────────────── */

#define MASTER_NODE_ID 0x01
#define SLAVE_NODE_ID 0x02

#define MSG_TYPE_SET_LED 0x10
#define MSG_TYPE_STATUS 0x11

/* Compile-time flag: set to 1 for Master MCU, 0 for Slave MCU */
#ifndef IS_MASTER_MCU
#define IS_MASTER_MCU 1
#endif

extern UART_HandleTypeDef huart2;

static uint8_t rx_byte;           /* 1-byte UART RX interrupt buffer */
static SYN_COBS_Decoder cobs_dec; /* Streaming COBS decoder */
static uint8_t cobs_buf[128];     /* COBS packet assembly buffer */

static SYN_Router router;
static SYN_RouterHandler handlers[4];

/* ── 1. Custom Transport (UART Transmit) ────────────────────────────────── */

static SYN_Status uart_send_pkt(const uint8_t *data, size_t len, void *ctx)
{
    (void)ctx;

    /* Encode payload with COBS and append trailing 0x00 delimiter */
    uint8_t enc_buf[140];
    size_t enc_len = syn_cobs_encode(data, len, enc_buf);
    enc_buf[enc_len++] = 0x00; /* Framing delimiter */

    /* Transmit framed packet via HAL UART */
    HAL_UART_Transmit(&huart2, enc_buf, (uint16_t)enc_len, 100);
    return SYN_OK;
}

static SYN_Transport uart_transport = {
    .send = uart_send_pkt,
    .ctx = NULL,
};

/* ── 2. COBS Decoder Callback ───────────────────────────────────────────── */

static void on_cobs_frame_received(const uint8_t *data, size_t len, void *ctx)
{
    (void)ctx;
    /* Feed assembled frame into router for packet parsing & dispatch */
    syn_router_feed(&router, data, len);
}

/* ── 3. Single-Byte UART RX Interrupt (No DMA) ──────────────────────────── */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        /* 1. Feed single byte into COBS streaming state machine */
        syn_cobs_decoder_feed(&cobs_dec, rx_byte);

        /* 2. Re-arm single-byte UART RX interrupt for next byte */
        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
    }
}

/* ── 4. Application Message Callbacks ────────────────────────────────────── */

#if !IS_MASTER_MCU
/* SLAVE HANDLER: Process LED command from Master */
static void on_set_led_command(const SYN_Packet *pkt, void *ctx)
{
    (void)ctx;

    if (pkt->len > 0 && pkt->payload[0] == 0x01) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET); /* LED ON */
    } else {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET); /* LED OFF */
    }

    /* Reply to Master with Slave status packet */
    uint8_t resp_data[] = {0xAA, 0x55};
    syn_router_send(&router, MASTER_NODE_ID, MSG_TYPE_STATUS, resp_data, sizeof(resp_data), false);
}
#endif

#if IS_MASTER_MCU
/* MASTER HANDLER: Process Status response from Slave */
static void on_slave_status_received(const SYN_Packet *pkt, void *ctx)
{
    (void)ctx;
    /* Status packet received from Slave (pkt->src == SLAVE_NODE_ID) */
    (void)pkt;
}
#endif

/* ── 5. System Setup & Application Main Loop ───────────────────────────── */

void app_init(void)
{
    /* Initialize COBS streaming decoder state machine */
    syn_cobs_decoder_init(&cobs_dec, cobs_buf, sizeof(cobs_buf), on_cobs_frame_received, NULL);

#if IS_MASTER_MCU
    /* Initialize Router as MASTER (Node ID 0x01) */
    syn_router_init(&router, MASTER_NODE_ID, &uart_transport, handlers, 4);
    syn_router_register(&router, MSG_TYPE_STATUS, on_slave_status_received, NULL);
#else
    /* Initialize Router as SLAVE (Node ID 0x02) */
    syn_router_init(&router, SLAVE_NODE_ID, &uart_transport, handlers, 4);
    syn_router_register(&router, MSG_TYPE_SET_LED, on_set_led_command, NULL);
#endif

    /* Start initial single-byte UART RX interrupt */
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}

void app_loop(void)
{
    /* Poll router for background tasks (ACK retries, timeouts) */
    syn_router_poll(&router);

#if IS_MASTER_MCU
    /* Master sends LED toggle command to Slave every 1 second */
    static uint32_t last_tick = 0;
    if (HAL_GetTick() - last_tick >= 1000) {
        last_tick = HAL_GetTick();

        static bool led_on = false;
        led_on = !led_on;

        uint8_t cmd_payload[] = {led_on ? 0x01 : 0x00};
        syn_router_send(&router, SLAVE_NODE_ID, MSG_TYPE_SET_LED, cmd_payload, sizeof(cmd_payload),
                        true);
    }
#endif
}

int main(void)
{
    HAL_Init();
    /* Configure System Clock, GPIOs, and USART2... */

    app_init();

    while (1) {
        app_loop();
    }
}
