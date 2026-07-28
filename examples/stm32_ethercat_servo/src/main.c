/**
 * @file main.c
 * @brief SyntropicOS EtherCAT (IEEE 802.3 EtherType 0x88A4) Slave & CoE Drive STM32 HAL Example.
 *
 * Demonstrates L2 raw Ethernet EtherCAT frame ingestion (`HAL_ETH_RxCpltCallback`),
 * EtherCAT State Machine (ESM INIT -> PREOP -> SAFEOP -> OP) state processing,
 * CoE (CAN Application Protocol over EtherCAT) Object Dictionary binding, Working Counter (WKC)
 * accounting, and 1kHz cyclic process data exchange (RxPDO / TxPDO) using STM32 HAL ETH drivers.
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32f7xx_hal.h) */

/* Hardware Ethernet Handle instance */
extern ETH_HandleTypeDef heth;

/* Station Address assigned by EtherCAT Master during SCAN */
#define ETHERCAT_STATION_ADDR 0x1001

/* Static EtherCAT Node & CANopen Object Dictionary Bindings */
static SYN_EcatNode ecat_node;
static SYN_CANOpenNode canopen_od;
static SYN_CANOpenDictEntry od_entries[16];

/* CiA 402 Servo Drive Object Dictionary Variables */
static uint16_t control_word = 0;   /* Index 0x6040 Controlword */
static uint16_t status_word  = 0x0270; /* Index 0x6041 Statusword (Operation Enabled) */
static int32_t  target_position = 0; /* Index 0x607A Target Position */
static int32_t  actual_position = 0; /* Index 0x6064 Position Actual Value */
static int16_t  actual_torque   = 0; /* Index 0x6077 Torque Actual Value */

/**
 * @brief Initialize Object Dictionary entries for CiA 402 Drive Profile CoE.
 */
static void init_drive_object_dictionary(void)
{
    syn_canopen_init(&canopen_od, 0x01, od_entries, sizeof(od_entries) / sizeof(od_entries[0]));

    /* Add CiA 402 Controlword (0x6040:00) */
    syn_canopen_add_entry(&canopen_od, 0x6040, 0x00, SYN_CANOPEN_TYPE_UINT16,
                         SYN_CANOPEN_ATTR_RW, &control_word);

    /* Add CiA 402 Statusword (0x6041:00) */
    syn_canopen_add_entry(&canopen_od, 0x6041, 0x00, SYN_CANOPEN_TYPE_UINT16,
                         SYN_CANOPEN_ATTR_RO, &status_word);

    /* Add Target Position (0x607A:00) */
    syn_canopen_add_entry(&canopen_od, 0x607A, 0x00, SYN_CANOPEN_TYPE_INT32,
                         SYN_CANOPEN_ATTR_RW, &target_position);

    /* Add Actual Position (0x6064:00) */
    syn_canopen_add_entry(&canopen_od, 0x6064, 0x00, SYN_CANOPEN_TYPE_INT32,
                         SYN_CANOPEN_ATTR_RO, &actual_position);
}

/**
 * @brief Initialize EtherCAT Node protocol stack.
 */
void ethercat_app_init(void)
{
    /* 1. Setup Object Dictionary for CoE */
    init_drive_object_dictionary();

    /* 2. Initialize EtherCAT Node with Station Address 0x1001 */
    syn_ecat_init(&ecat_node, ETHERCAT_STATION_ADDR, &canopen_od);

    /* 3. Start EtherCAT State Machine in INIT state */
    syn_ecat_set_state(&ecat_node, SYN_ECAT_STATE_INIT);
}

/**
 * @brief STM32 HAL Ethernet Rx Frame Interrupt Callback.
 *
 * Ingests L2 raw Ethernet frame (EtherType 0x88A4) directly into SyntropicOS EtherCAT stack.
 */
void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth_ptr)
{
    if (heth_ptr->Instance == heth.Instance) {
        uint8_t rx_buf[1518];
        uint32_t length = 0;

        /* Extract raw Ethernet frame from STM32 HAL DMA descriptors */
        if (HAL_ETH_ReadData(heth_ptr, (void **)rx_buf) == HAL_OK) {
            length = heth_ptr->RxDesc->BackupAddr0; /* Frame length */

            if (length >= 14) {
                uint16_t ethertype = ((uint16_t)rx_buf[12] << 8) | rx_buf[13];

                /* Verify EtherType 0x88A4 (EtherCAT) */
                if (ethertype == SYN_ETHERCAT_ETHERTYPE) {
                    uint16_t wkc = 0;

                    /* Parse datagrams starting after 14-byte MAC header */
                    SYN_Status status = syn_ecat_parse_frame(&ecat_node, &rx_buf[14], (size_t)(length - 14), &wkc);
                    if (status == SYN_OK) {
                        ecat_node.wkc_last = wkc;
                    }
                }
            }
        }

        /* Re-arm Ethernet MAC reception */
        HAL_ETH_BuildRxDescriptors(heth_ptr);
    }
}

/**
 * @brief Transmit an EtherCAT response frame via STM32 HAL Ethernet MAC DMA.
 */
static bool send_ethercat_raw_frame(const uint8_t *frame, size_t len)
{
    uint8_t eth_frame[1518];

    /* Broadcast MAC Dest (FF:FF:FF:FF:FF:FF) + STM32 Src MAC */
    for (int i = 0; i < 6; i++) {
        eth_frame[i] = 0xFF;
    }
    eth_frame[6] = 0x02; eth_frame[7] = 0x00; eth_frame[8] = 0x00;
    eth_frame[9] = 0x00; eth_frame[10] = 0x00; eth_frame[11] = 0x01;

    /* Set EtherType 0x88A4 */
    eth_frame[12] = 0x88;
    eth_frame[13] = 0xA4;

    /* Copy EtherCAT PDU payload */
    for (size_t i = 0; i < len && i < 1500; i++) {
        eth_frame[14 + i] = frame[i];
    }

    return HAL_ETH_TransmitFrame(&heth, 14 + len) == HAL_OK;
}

/**
 * @brief 1kHz Cyclic EtherCAT Servo Control Loop (called from SYSTICK or TIM ISR).
 */
void ethercat_cyclic_task_1ms(void)
{
    /* Step EtherCAT State Machine */
    syn_ecat_update(&ecat_node);

    /* Process Cyclic Data Exchange when ESM is in OP (Operational) state */
    if (ecat_node.state == SYN_ECAT_STATE_OP) {
        /* 1. Update actual position feedback */
        if (actual_position < target_position) {
            actual_position += 10;
        } else if (actual_position > target_position) {
            actual_position -= 10;
        }

        /* 2. Build cyclic TxPDO datagram frame (FPRD / LRW) */
        SYN_EcatDatagram datagram = {
            .cmd = SYN_ECAT_CMD_FPRW,
            .idx = 1,
            .addr = ETHERCAT_STATION_ADDR,
            .len = 8,
            .m = 0,
            .wkc = 0
        };

        uint8_t pdo_data[8];
        pdo_data[0] = (uint8_t)(status_word & 0xFF);
        pdo_data[1] = (uint8_t)(status_word >> 8);
        pdo_data[2] = (uint8_t)(actual_position & 0xFF);
        pdo_data[3] = (uint8_t)((actual_position >> 8) & 0xFF);
        pdo_data[4] = (uint8_t)((actual_position >> 16) & 0xFF);
        pdo_data[5] = (uint8_t)((actual_position >> 24) & 0xFF);
        pdo_data[6] = (uint8_t)(actual_torque & 0xFF);
        pdo_data[7] = (uint8_t)(actual_torque >> 8);

        uint8_t tx_frame[256];
        size_t tx_len = syn_ecat_build_datagram_frame(tx_frame, sizeof(tx_frame), &datagram, pdo_data, 8);

        if (tx_len > 0) {
            send_ethercat_raw_frame(tx_frame, tx_len);
            ecat_node.tx_pdos++;
        }
    }
}
