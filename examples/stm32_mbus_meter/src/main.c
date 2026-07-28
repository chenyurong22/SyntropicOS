/**
 * @file main.c
 * @brief SyntropicOS M-Bus (EN 13757-2 / EN 13757-3) Utility Meter Reader STM32 HAL Example.
 *
 * Demonstrates non-blocking M-Bus master frame formatting (`syn_mbus_encode_short`),
 * streaming byte-by-byte USART interrupt ingestion (`syn_mbus_decoder_feed`),
 * link initialization (`SND_NKE`), data class 2 requests (`REQ_UD2`), and response frame
 * decoding (`RSP_UD`) using STM32 HAL USART drivers (`HAL_UART_...`).
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Hardware USART handle instance for M-Bus level converter (TSS721A) */
extern UART_HandleTypeDef huart2;

/* Single byte interrupt buffer for USART RX */
static uint8_t mbus_rx_byte;

/* SyntropicOS M-Bus Decoder and Context */
static SYN_MBUS_Decoder mbus_decoder;
static SYN_MBUS_Frame last_rx_frame;
static bool new_frame_received = false;

/* Target Meter Primary Address (0x01..0xFA, 0xFE = broadcast with reply) */
#define METER_PRIMARY_ADDR 0x01

/**
 * @brief Frame completion callback invoked by SyntropicOS M-Bus streaming decoder.
 * @param frame Pointer to decoded M-Bus frame structure.
 * @param ctx   User context pointer.
 */
static void on_mbus_frame_received(const SYN_MBUS_Frame *frame, void *ctx)
{
    (void)ctx;
    if (frame != NULL && frame->checksum_valid) {
        last_rx_frame = *frame;
        new_frame_received = true;
    }
}

/**
 * @brief Initialize M-Bus protocol stack and USART interrupt reception.
 */
void mbus_app_init(void)
{
    /* Initialize M-Bus streaming state machine decoder */
    syn_mbus_decoder_init(&mbus_decoder, on_mbus_frame_received, NULL);

    /* Arm USART2 interrupt for 1-byte reception */
    HAL_UART_Receive_IT(&huart2, &mbus_rx_byte, 1);
}

/**
 * @brief STM32 HAL USART Rx Interrupt Callback.
 *
 * Feeds incoming bytes from M-Bus transceiver into SyntropicOS decoder.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        /* Feed byte into M-Bus state machine decoder */
        syn_mbus_decoder_feed(&mbus_decoder, mbus_rx_byte);

        /* Re-arm USART interrupt */
        HAL_UART_Receive_IT(&huart2, &mbus_rx_byte, 1);
    }
}

/**
 * @brief Send an M-Bus Short Frame (SND_NKE or REQ_UD2) to target meter.
 * @param c_field Control Field (SYN_MBUS_C_SND_NKE or SYN_MBUS_C_REQ_UD2).
 * @param a_field Primary Address (0x01..0xFA).
 */
static bool send_mbus_short_frame(uint8_t c_field, uint8_t a_field)
{
    uint8_t tx_buf[8];
    size_t tx_len = 0;

    /* Encode 5-byte M-Bus Short Frame (0x10 | C | A | Checksum | 0x16) */
    SYN_Status status = syn_mbus_encode_short(c_field, a_field, tx_buf, sizeof(tx_buf), &tx_len);
    if (status != SYN_OK) {
        return false;
    }

    return HAL_UART_Transmit(&huart2, tx_buf, (uint16_t)tx_len, 100) == HAL_OK;
}

/**
 * @brief Perform M-Bus Link Initialization (SND_NKE C=0x40).
 */
bool mbus_send_link_reset(uint8_t address)
{
    return send_mbus_short_frame(SYN_MBUS_C_SND_NKE, address);
}

/**
 * @brief Request User Data Class 2 (REQ_UD2 C=0x5B) from target meter.
 */
bool mbus_request_meter_data(uint8_t address)
{
    return send_mbus_short_frame(SYN_MBUS_C_REQ_UD2, address);
}

/**
 * @brief Process decoded M-Bus response frame payload (EN 13757-3 data records).
 * @param frame Pointer to received M-Bus frame.
 */
static void process_meter_response(const SYN_MBUS_Frame *frame)
{
    if (frame->type == SYN_MBUS_FRAME_TYPE_SINGLE_ACK) {
        /* Meter sent single 0xE5 ACK (successful Link Reset) */
        return;
    }

    if (frame->type == SYN_MBUS_FRAME_TYPE_LONG || frame->type == SYN_MBUS_FRAME_TYPE_CONTROL) {
        if (frame->c_field == SYN_MBUS_C_RSP_UD) {
            /* Response User Data (RSP_UD) payload from water/heat/electricity meter */
            uint8_t ci = frame->ci_field;
            (void)ci;

            /* Extract meter serial number, energy/volume readings from payload */
            if (frame->payload_len >= 12) {
                const uint8_t *data_records = &frame->payload[12]; /* Skip 12-byte fixed header */
                size_t records_len = frame->payload_len - 12;
                (void)data_records;
                (void)records_len;
            }
        }
    }
}

/**
 * @brief Periodic 1000ms M-Bus polling task.
 */
void mbus_app_task_1000ms(void)
{
    static uint8_t state = 0;

    switch (state) {
    case 0:
        /* Step 1: Send Link Reset (SND_NKE) to Meter Address 0x01 */
        mbus_send_link_reset(METER_PRIMARY_ADDR);
        state = 1;
        break;

    case 1:
        /* Step 2: Request Data Class 2 (REQ_UD2) to read meter telemetry */
        mbus_request_meter_data(METER_PRIMARY_ADDR);
        state = 2;
        break;

    case 2:
        /* Step 3: Check if response frame was received and processed */
        if (new_frame_received) {
            new_frame_received = false;
            process_meter_response(&last_rx_frame);
        }
        state = 1; /* Loop data requests */
        break;

    default:
        state = 0;
        break;
    }
}
