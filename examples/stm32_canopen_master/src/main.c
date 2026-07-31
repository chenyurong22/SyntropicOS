/**
 * @file main.c
 * @brief SyntropicOS CANopen CiA 302 Master / Network Manager STM32 HAL Example.
 *
 * Demonstrates non-blocking, zero-malloc CANopen Master engine integration
 * (NMT Master control, SDO Client read/write, Node Heartbeat monitoring, and TPDO ingestion)
 * with STM32 HAL CAN drivers (`HAL_CAN_...`).
 */

#include "syntropic/proto/syn_canopen_mgr.h"
#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target MCU header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Hardware CAN handle instance */
extern CAN_HandleTypeDef hcan1;

/* Target Slave Node Configuration (Node-ID = 0x20 / 32) */
#define TARGET_SLAVE_NODE_ID 0x20

/* Master Handle */
static SYN_CANOpenManager canopen_mgr;

/**
 * @brief Transmit a raw 11-bit Standard CAN message frame via STM32 HAL CAN.
 * @param frame Pointer to SyntropicOS CAN frame structure.
 * @return true if added to CAN Tx mailbox successfully, false otherwise.
 */
static bool send_hal_can_frame(const SYN_CAN_Frame *frame)
{
    if (frame == NULL) {
        return false;
    }

    CAN_TxHeaderTypeDef tx_hdr = {0};
    tx_hdr.StdId = frame->id;
    tx_hdr.IDE = CAN_ID_STD; /* CANopen uses 11-bit Standard CAN COB-IDs */
    tx_hdr.RTR = CAN_RTR_DATA;
    tx_hdr.DLC = frame->dlc;

    uint32_t tx_mailbox;
    return HAL_CAN_AddTxMessage(&hcan1, &tx_hdr, (uint8_t *)frame->data, &tx_mailbox) == HAL_OK;
}

/**
 * @brief Initialize CANopen Master Manager.
 */
void canopen_master_app_init(void)
{
    /* Initialize manager state structure */
    syn_canopen_mgr_init(&canopen_mgr);

    /* Send NMT Start Node command (0x01) to put slave into Operational state */
    SYN_CAN_Frame nmt_frame;
    syn_canopen_mgr_build_nmt(&nmt_frame, TARGET_SLAVE_NODE_ID, SYN_CANOPEN_NMT_CMD_START);
    send_hal_can_frame(&nmt_frame);
}

/**
 * @brief Initiate an SDO Read request for Device Type (OD 0x1000:00).
 */
bool canopen_master_read_device_type(uint8_t target_node)
{
    SYN_CAN_Frame req_frame;
    SYN_Status st = syn_canopen_mgr_sdo_read_init(&canopen_mgr, &req_frame, target_node, 0x1000, 0x00);
    if (st == SYN_OK) {
        return send_hal_can_frame(&req_frame);
    }
    return false;
}

/**
 * @brief Initiate an SDO Write request for Producer Heartbeat Time (OD 0x1017:00 = 1000 ms).
 */
bool canopen_master_write_heartbeat(uint8_t target_node, uint16_t heartbeat_ms)
{
    SYN_CAN_Frame req_frame;
    SYN_Status st = syn_canopen_mgr_sdo_write_init(&canopen_mgr, &req_frame, target_node, 0x1017, 0x00,
                                                  &heartbeat_ms, sizeof(heartbeat_ms));
    if (st == SYN_OK) {
        return send_hal_can_frame(&req_frame);
    }
    return false;
}

/**
 * @brief STM32 HAL CAN Rx FIFO 0 Interrupt Callback.
 *
 * Invoked by STM32 HAL whenever a raw 11-bit CAN frame arrives on FIFO 0.
 * Ingests incoming SDO responses, Heartbeat (0x780+NodeID), and TPDOs into manager engine.
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1) {
        CAN_RxHeaderTypeDef rx_hdr;
        uint8_t rx_data[8];

        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_hdr, rx_data) == HAL_OK) {
            if (rx_hdr.IDE == CAN_ID_STD) {
                SYN_CAN_Frame rx_frame;
                rx_frame.id = rx_hdr.StdId;
                rx_frame.dlc = (uint8_t)rx_hdr.DLC;
                rx_frame.extended = false;
                rx_frame.rtr = (rx_hdr.RTR == CAN_RTR_REMOTE);
                (void)memcpy(rx_frame.data, rx_data, rx_frame.dlc);

                syn_canopen_mgr_process_frame(&canopen_mgr, &rx_frame);
            }
        }
    }
}

/**
 * @brief Periodic master application task (call from main loop every tick).
 * @param delta_ms Elapsed time since last tick in milliseconds.
 */
void canopen_master_app_step(uint16_t delta_ms)
{
    /* Step manager timers (SDO client timeouts, node heartbeat monitoring) */
    syn_canopen_mgr_step(&canopen_mgr, delta_ms);

    /* Check status of target slave node heartbeat monitoring */
    if (TARGET_SLAVE_NODE_ID < 128) {
        bool slave_online = canopen_mgr.nodes[TARGET_SLAVE_NODE_ID].online;
        uint8_t slave_nmt_state = canopen_mgr.nodes[TARGET_SLAVE_NODE_ID].nmt_state;
        (void)slave_online;
        (void)slave_nmt_state;
    }

    /* Check SDO client state */
    if (canopen_mgr.sdo_client.state == SYN_SDO_CLIENT_STATE_SUCCESS) {
        /* SDO operation completed successfully — data is in canopen_mgr.sdo_client.data */
    } else if (canopen_mgr.sdo_client.state == SYN_SDO_CLIENT_STATE_ERROR) {
        /* SDO operation failed — abort code in canopen_mgr.sdo_client.abort_code */
    }
}
