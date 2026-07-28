/**
 * @file main.c
 * @brief SyntropicOS LIN 2.1 Automotive Bus Master & Slave STM32 HAL Example.
 *
 * Demonstrates non-blocking LIN 2.0 / 2.1 Master schedule table execution
 * and Slave state machine byte stream processing using STM32 HAL USART
 * drivers (`HAL_UART_...`). Handles Break generation (`HAL_UART_SendBreak`),
 * Sync byte (0x55), Protected Identifier (PID) parity, Classic/Enhanced checksums,
 * and LIN transceiver Enable/Sleep GPIO control.
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Hardware USART handle instance for LIN transceiver */
extern UART_HandleTypeDef huart1;

/* Pin definitions for LIN Transceiver (TJA1021 / MCP2003) Sleep / Enable pin */
#define LIN_NSLP_PORT GPIOA
#define LIN_NSLP_PIN  GPIO_PIN_8

/* LIN Frame IDs used in this application */
#define LIN_ID_CMD_SLAVE1   0x10 /* Master publishes control command to Slave 1 */
#define LIN_ID_STATUS_SLAVE1 0x11 /* Master subscribes to status telemetry from Slave 1 */
#define LIN_ID_AMBIENT_TEMP 0x22 /* Master subscribes to ambient temperature sensor */

/* Master Schedule Table Definition */
static const SYN_LIN_ScheduleSlot master_schedule[] = {
    {
        .id = LIN_ID_CMD_SLAVE1,
        .len = 4,
        .checksum_mode = SYN_LIN_CHECKSUM_ENHANCED,
        .dir = SYN_LIN_SLOT_PUBLISH,
        .data = {0x01, 0x80, 0x00, 0xFF},
        .slot_delay_ms = 20, /* 20ms slot window */
    },
    {
        .id = LIN_ID_STATUS_SLAVE1,
        .len = 2,
        .checksum_mode = SYN_LIN_CHECKSUM_ENHANCED,
        .dir = SYN_LIN_SLOT_SUBSCRIBE,
        .data = {0},
        .slot_delay_ms = 20,
    },
    {
        .id = LIN_ID_AMBIENT_TEMP,
        .len = 2,
        .checksum_mode = SYN_LIN_CHECKSUM_CLASSIC,
        .dir = SYN_LIN_SLOT_SUBSCRIBE,
        .data = {0},
        .slot_delay_ms = 30,
    },
};

/* LIN Engine Context Handles */
static SYN_LIN_Master lin_master;
static SYN_LIN_Slave lin_slave;

/**
 * @brief Initialize LIN Transceiver GPIO and SyntropicOS LIN Protocol Engines.
 */
void lin_app_init(void)
{
    /* Wake up LIN transceiver (Set NSLP / EN pin HIGH) */
    HAL_GPIO_WritePin(LIN_NSLP_PORT, LIN_NSLP_PIN, GPIO_PIN_SET);

    /* Initialize Master engine with 3-slot schedule table */
    syn_lin_master_init(&lin_master, master_schedule, sizeof(master_schedule) / sizeof(master_schedule[0]));

    /* Initialize Slave engine with NAD 0x02 */
    syn_lin_slave_init(&lin_slave, 0x02);

    /* Register Slave frame filters */
    syn_lin_slave_add_frame(&lin_slave, LIN_ID_CMD_SLAVE1, 4, SYN_LIN_SLOT_SUBSCRIBE, SYN_LIN_CHECKSUM_ENHANCED);
    syn_lin_slave_add_frame(&lin_slave, LIN_ID_STATUS_SLAVE1, 2, SYN_LIN_SLOT_PUBLISH, SYN_LIN_CHECKSUM_ENHANCED);

    /* Load payload bytes for Slave published frame (STATUS_SLAVE1) */
    uint8_t slave_status_data[2] = {0x42, 0x00};
    syn_lin_slave_set_publish_data(&lin_slave, LIN_ID_STATUS_SLAVE1, slave_status_data, sizeof(slave_status_data));
}

/**
 * @brief Transmit a complete LIN Header (Break + Sync + PID) as Master.
 * @param frame_id 6-bit LIN Frame ID (0..63).
 */
static void lin_master_send_header(uint8_t frame_id)
{
    /* 1. Generate Break Field (>= 13 dominant bits low) */
    HAL_UART_SendBreak(&huart1);

    /* 2. Compute Protected Identifier (PID) byte */
    uint8_t pid = syn_lin_calc_pid(frame_id);

    /* 3. Transmit Sync Byte (0x55) followed by PID */
    uint8_t header_buf[2] = {SYN_LIN_SYNC_BYTE, pid};
    HAL_UART_Transmit(&huart1, header_buf, 2, 10);
}

/**
 * @brief Transmit LIN Response data payload and calculated checksum.
 */
static void lin_send_response(uint8_t pid, const uint8_t *data, uint8_t len, SYN_LIN_ChecksumMode mode)
{
    uint8_t tx_buf[9];
    for (uint8_t i = 0; i < len; i++) {
        tx_buf[i] = data[i];
    }

    /* Append Classic or Enhanced Checksum byte */
    tx_buf[len] = syn_lin_calc_checksum(pid, data, len, mode);

    HAL_UART_Transmit(&huart1, tx_buf, len + 1, 10);
}

/**
 * @brief STM32 HAL USART Rx Interrupt Callback.
 *
 * Invoked by STM32 HAL whenever a byte arrives from the LIN bus.
 * Feeds byte into SyntropicOS LIN Slave parser.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        extern uint8_t lin_rx_byte;
        SYN_LIN_Frame rx_frame;

        if (syn_lin_slave_process_byte(&lin_slave, lin_rx_byte, &rx_frame)) {
            /* Valid LIN frame received by Slave */
            if (rx_frame.id == LIN_ID_CMD_SLAVE1) {
                /* Command frame received from Master -> Update local actuator state */
                uint8_t cmd_param = rx_frame.data[0];
                (void)cmd_param;
            }
        }

        /* Re-arm USART interrupt reception for next byte */
        HAL_UART_Receive_IT(&huart1, &lin_rx_byte, 1);
    }
}

/**
 * @brief Periodic LIN Master task (called every 10ms from scheduler).
 * @param dt_ms Time step since last execution (e.g., 10ms).
 */
void lin_master_task(uint32_t dt_ms)
{
    const SYN_LIN_ScheduleSlot *active_slot = NULL;

    /* Step schedule table timer */
    if (syn_lin_master_step(&lin_master, dt_ms, &active_slot)) {
        if (active_slot != NULL) {
            /* Transmit LIN Master Header */
            lin_master_send_header(active_slot->id);

            uint8_t pid = syn_lin_calc_pid(active_slot->id);

            if (active_slot->dir == SYN_LIN_SLOT_PUBLISH) {
                /* Master publishes payload data to LIN bus */
                lin_send_response(pid, active_slot->data, active_slot->len, active_slot->checksum_mode);
            } else {
                /* Master subscribes -> Arm USART DMA/IT to receive slave response */
                static uint8_t slave_rx_buf[9];
                HAL_UART_Receive_IT(&huart1, slave_rx_buf, active_slot->len + 1);
            }
        }
    }
}
