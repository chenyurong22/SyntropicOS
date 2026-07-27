/**
 * @file main.c
 * @brief SyntropicOS SAE J1939 Heavy-Duty Vehicle CAN Protocol STM32 HAL Example.
 *
 * Demonstrates non-blocking, zero-malloc SAE J1939 protocol stack integration
 * with STM32 HAL CAN drivers (`HAL_CAN_...`), including 29-bit CAN ID parsing,
 * Address Claiming (J1939-81), BAM Transport Protocol multi-packet frame transmission
 * and reception, and Active Diagnostic Trouble Code (DM1 J1939-73) broadcasts.
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Hardware CAN handle instance */
extern CAN_HandleTypeDef hcan1;

/* Static J1939 Node & Communication Buffers (Zero Heap Allocation) */
static SYN_J1939_Node j1939_node;
static SYN_J1939_DTC active_dtcs[4];
static size_t dtc_count = 0;

/**
 * @brief Initialize J1939 Node parameters and Address Claiming (J1939-81).
 */
void j1939_app_init(void)
{
    /* Configure 64-bit J1939 Device NAME */
    SYN_J1939_Name name = {
        .identity_number = 123456,              /* 21-bit unique serial number */
        .manufacturer_code = 0x07F,             /* 11-bit SAE manufacturer code */
        .ecu_instance = 0,                      /* First ECU instance */
        .function_instance = 0,                 /* First Function instance */
        .function = 0x1A,                       /* Engine Controller function */
        .vehicle_system = 0x00,                 /* Tractor / Vehicle System */
        .vehicle_system_inst = 0,               /* System Instance */
        .industry_group = 1,                    /* On-Highway Equipment */
        .arbitrary_addr_cap = true,             /* Capable of address claim arbitration */
    };

    /* Initialize node with preferred Source Address (SA = 0x00 for Engine #1) */
    syn_j1939_node_init(&j1939_node, 0x00, &name);

    /* Setup initial active Diagnostic Trouble Code (DM1 SPN 110, FMI 3: Engine Coolant Temp High) */
    active_dtcs[0].spn = 110;
    active_dtcs[0].fmi = 3;
    active_dtcs[0].occurrence_count = 1;
    active_dtcs[0].conversion_method = 0;
    dtc_count = 1;
}

/**
 * @brief STM32 HAL CAN Rx FIFO 0 Interrupt Callback.
 *
 * Invoked by STM32 HAL whenever a raw 29-bit CAN frame arrives on FIFO 0.
 * Passes raw frame into SyntropicOS J1939 node state machine.
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1) {
        CAN_RxHeaderTypeDef rx_hdr;
        SYN_CAN_Frame frame;

        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_hdr, frame.data) == HAL_OK) {
            frame.id = (rx_hdr.IDE == CAN_ID_EXT) ? rx_hdr.ExtId : rx_hdr.StdId;
            frame.dlc = (uint8_t)rx_hdr.DLC;

            /* Ingest J1939 frame */
            uint32_t rx_pgn;
            const uint8_t *rx_payload;
            size_t rx_len;

            SYN_Status status = syn_j1939_process_rx(&j1939_node, &frame, &rx_pgn, &rx_payload, &rx_len);
            if (status == SYN_OK) {
                /* Complete J1939 message received (single frame or reassembled BAM/RTS-CTS payload) */
                if (rx_pgn == SYN_J1939_PGN_REQUEST && rx_len >= 3) {
                    /* Handle PGN Request */
                    uint32_t requested_pgn = (uint32_t)rx_payload[0] |
                                             ((uint32_t)rx_payload[1] << 8) |
                                             ((uint32_t)rx_payload[2] << 16);
                    (void)requested_pgn;
                }
            }
        }
    }
}

/**
 * @brief Send a raw 29-bit CAN frame via STM32 HAL CAN driver.
 */
static bool send_hal_can_frame(uint32_t can_id_29bit, const uint8_t *data, uint8_t dlc)
{
    CAN_TxHeaderTypeDef tx_hdr = {0};
    tx_hdr.ExtId = can_id_29bit;
    tx_hdr.IDE = CAN_ID_EXT; /* J1939 always uses 29-bit Extended CAN Identifiers */
    tx_hdr.RTR = CAN_RTR_DATA;
    tx_hdr.DLC = dlc;

    uint32_t tx_mailbox;
    return HAL_CAN_AddTxMessage(&hcan1, &tx_hdr, (uint8_t *)data, &tx_mailbox) == HAL_OK;
}

/**
 * @brief Transmit J1939 Address Claim frame (J1939-81).
 */
void j1939_broadcast_address_claim(void)
{
    SYN_CAN_Frame claim_frame;
    if (syn_j1939_build_address_claim(&j1939_node, &claim_frame) == SYN_OK) {
        send_hal_can_frame(claim_frame.id, claim_frame.data, claim_frame.dlc);
        j1939_node.state = SYN_J1939_STATE_CLAIMED;
    }
}

/**
 * @brief Broadcast Active Diagnostic Trouble Codes (DM1 J1939-73) via BAM Transport Protocol.
 */
void j1939_broadcast_dm1_dtcs(void)
{
    if (j1939_node.state != SYN_J1939_STATE_CLAIMED)
        return;

    uint8_t dm1_payload[64];
    size_t dm1_len = syn_j1939_encode_dm1(dm1_payload, sizeof(dm1_payload), active_dtcs, dtc_count, 0x00);

    if (dm1_len <= 8) {
        /* Single frame transmission */
        uint32_t can_id = syn_j1939_id_pack(6, SYN_J1939_PGN_DM1, j1939_node.sa, SYN_J1939_ADDR_GLOBAL);
        send_hal_can_frame(can_id, dm1_payload, (uint8_t)dm1_len);
    } else {
        /* Multi-packet Transport Protocol Broadcast Announce Message (BAM) */
        SYN_CAN_Frame bam_cm_frame;
        if (syn_j1939_build_tp_bam(j1939_node.sa, SYN_J1939_PGN_DM1, (uint16_t)dm1_len, &bam_cm_frame) == SYN_OK) {
            send_hal_can_frame(bam_cm_frame.id, bam_cm_frame.data, bam_cm_frame.dlc);

            /* Transmit TP.DT payload packets */
            uint8_t total_packets = (uint8_t)((dm1_len + 6) / 7);
            for (uint8_t seq = 1; seq <= total_packets; seq++) {
                size_t offset = (seq - 1) * 7;
                size_t chunk_len = (dm1_len - offset > 7) ? 7 : (dm1_len - offset);

                SYN_CAN_Frame dt_frame;
                if (syn_j1939_build_tp_dt(j1939_node.sa, seq, &dm1_payload[offset], chunk_len, &dt_frame) == SYN_OK) {
                    send_hal_can_frame(dt_frame.id, dt_frame.data, dt_frame.dlc);
                }
            }
        }
    }
}

/**
 * @brief Periodic 100ms J1939 application task.
 */
void j1939_app_task_100ms(void)
{
    if (j1939_node.state == SYN_J1939_STATE_UNCLAIMED) {
        j1939_broadcast_address_claim();
    }

    /* Broadcast DM1 diagnostic trouble codes periodically (1000ms rate per J1939-73) */
    static uint8_t ticks_100ms = 0;
    if (++ticks_100ms >= 10) {
        ticks_100ms = 0;
        j1939_broadcast_dm1_dtcs();
    }
}
