/**
 * @file main.c
 * @brief SyntropicOS CANopen DS301 Slave STM32 HAL Integration Example.
 *
 * Demonstrates non-blocking, zero-malloc CANopen DS301 slave protocol stack
 * integration (Object Dictionary, SDO server, TPDO/RPDO, NMT, Heartbeat, EMCY)
 * with STM32 HAL CAN drivers (`HAL_CAN_...`).
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target MCU header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Hardware CAN handle instance */
extern CAN_HandleTypeDef hcan1;

/* Node Configuration (Node-ID = 0x20 / 32) */
#define CANOPEN_NODE_ID 0x20

/* Live Object Dictionary Variables */
static uint32_t od_device_type = 0x00000192; /* 0x192 = 402 / Motion Control Profile */
static uint8_t od_error_register = 0x00;
static uint16_t od_heartbeat_period_ms = 1000;
static char od_device_name[] = "SyntropicOS CANopen Node";
static uint8_t od_digital_inputs = 0x0F;
static uint8_t od_digital_outputs = 0x00;
static uint16_t od_analog_input_ch1 = 2048; /* 12-bit ADC raw reading */

/* Object Dictionary Table */
static const SYN_CANOpenODEntry od_table[] = {
    {0x1000, 0x00, SYN_CANOPEN_TYPE_U32, SYN_CANOPEN_ACCESS_RO, &od_device_type, sizeof(od_device_type)},
    {0x1001, 0x00, SYN_CANOPEN_TYPE_U8, SYN_CANOPEN_ACCESS_RO, &od_error_register, sizeof(od_error_register)},
    {0x1008, 0x00, SYN_CANOPEN_TYPE_U8, SYN_CANOPEN_ACCESS_RO, od_device_name, sizeof(od_device_name) - 1},
    {0x1017, 0x00, SYN_CANOPEN_TYPE_U16, SYN_CANOPEN_ACCESS_RW, &od_heartbeat_period_ms, sizeof(od_heartbeat_period_ms)},
    {0x6000, 0x01, SYN_CANOPEN_TYPE_U8, SYN_CANOPEN_ACCESS_RO, &od_digital_inputs, sizeof(od_digital_inputs)},
    {0x6200, 0x01, SYN_CANOPEN_TYPE_U8, SYN_CANOPEN_ACCESS_RW, &od_digital_outputs, sizeof(od_digital_outputs)},
    {0x6401, 0x01, SYN_CANOPEN_TYPE_U16, SYN_CANOPEN_ACCESS_RO, &od_analog_input_ch1, sizeof(od_analog_input_ch1)},
};

/* CANopen Node Handle & Configuration */
static SYN_CANOpenNode canopen_node;

/**
 * @brief Transmit a raw 11-bit Standard CAN message via STM32 HAL CAN.
 */
static bool send_hal_can_msg(uint32_t cob_id, const uint8_t *data, uint8_t len)
{
    CAN_TxHeaderTypeDef tx_hdr = {0};
    tx_hdr.StdId = cob_id;
    tx_hdr.IDE = CAN_ID_STD; /* CANopen DS301 uses 11-bit Standard CAN COB-IDs */
    tx_hdr.RTR = CAN_RTR_DATA;
    tx_hdr.DLC = len;

    uint32_t tx_mailbox;
    return HAL_CAN_AddTxMessage(&hcan1, &tx_hdr, (uint8_t *)data, &tx_mailbox) == HAL_OK;
}

/**
 * @brief Initialize CANopen DS301 Slave Node.
 */
void canopen_app_init(void)
{
    SYN_CANOpenNodeConfig cfg = {0};
    cfg.node_id = CANOPEN_NODE_ID;
    cfg.heartbeat_ms = 1000; /* 1 Hz Heartbeat producer */

    /* Configure TPDO 1 mapping: COB-ID 0x180 + NodeID -> OD 0x6401 sub 1 (Analog Input Ch 1) */
    cfg.tpdo[0].enabled = 1;
    cfg.tpdo[0].cob_id = 0x180 + CANOPEN_NODE_ID;
    cfg.tpdo[0].od_index = 0x6401;
    cfg.tpdo[0].od_subindex = 0x01;

    /* Configure RPDO 1 mapping: COB-ID 0x200 + NodeID -> OD 0x6200 sub 1 (Digital Outputs) */
    cfg.rpdo[0].enabled = 1;
    cfg.rpdo[0].cob_id = 0x200 + CANOPEN_NODE_ID;
    cfg.rpdo[0].od_index = 0x6200;
    cfg.rpdo[0].od_subindex = 0x01;

    syn_canopen_init(&canopen_node, &cfg, od_table, sizeof(od_table) / sizeof(od_table[0]));
}

/**
 * @brief STM32 HAL CAN Rx FIFO 0 Interrupt Callback.
 *
 * Invoked by STM32 HAL whenever a raw 11-bit CAN frame arrives on FIFO 0.
 * Ingests frame into CANopen DS301 slave engine.
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1) {
        CAN_RxHeaderTypeDef rx_hdr;
        uint8_t rx_data[8];

        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_hdr, rx_data) == HAL_OK) {
            if (rx_hdr.IDE == CAN_ID_STD) {
                syn_canopen_process_rx(&canopen_node, rx_hdr.StdId, rx_data, (uint8_t)rx_hdr.DLC);
            }
        }
    }
}

/**
 * @brief Drain pending transmit frames from CANopen node and send via HAL CAN.
 */
static void drain_canopen_tx_queue(void)
{
    uint32_t cob_id;
    uint8_t data[8];
    uint8_t len;

    while (syn_canopen_get_tx(&canopen_node, &cob_id, data, &len)) {
        send_hal_can_msg(cob_id, data, len);
    }
}

/**
 * @brief Periodic Jitter/Tick Task (call every 1ms or 10ms from main loop).
 *
 * @param dt_ms Milliseconds elapsed since last call.
 */
void canopen_app_loop(uint32_t dt_ms)
{
    /* 1. Step CANopen internal timers (Heartbeat, SDO timeouts) */
    syn_canopen_update(&canopen_node, dt_ms);

    /* 2. Flush any generated response frames (SDO responses, Heartbeat, TPDOs) */
    drain_canopen_tx_queue();
}

/**
 * @brief Trigger periodic TPDO telemetry transmission (e.g. every 100ms).
 */
void canopen_app_task_100ms(void)
{
    /* Simulate ADC sensor reading update */
    od_analog_input_ch1 += 10;
    if (od_analog_input_ch1 > 4095)
        od_analog_input_ch1 = 0;

    /* Trigger TPDO 1 transmission if node is Operational */
    if (canopen_node.nmt_state == SYN_CANOPEN_NMT_STATE_OPERATIONAL) {
        syn_canopen_tpdo_trigger(&canopen_node, 1);
        drain_canopen_tx_queue();
    }
}

/**
 * @brief Trigger Emergency alarm message (EMCY).
 *
 * @param error_code 16-bit CANopen Emergency code (e.g. 0x3110 for Mains Voltage Error)
 */
void canopen_trigger_alarm(uint16_t error_code)
{
    syn_canopen_send_emcy(&canopen_node, error_code, 0x01);
    drain_canopen_tx_queue();
}
