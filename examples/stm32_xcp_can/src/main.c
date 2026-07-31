/**
 * @file main.c
 * @brief STM32 HAL XCP (Universal Measurement and Calibration Protocol v1.x) over CAN Example.
 */

#include "syntropic/proto/syn_xcp.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* STM32 HAL CAN Header Mock/Includes for standalone compile */
#if defined(STM32F4xx) || defined(STM32F7xx) || defined(STM32G4xx)
#include "stm32_hal.h"
#else
typedef struct {
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
} CAN_TxHeaderTypeDef;

typedef struct {
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
} CAN_RxHeaderTypeDef;

typedef struct {
    void *Instance;
} CAN_HandleTypeDef;

#define CAN_ID_STD 0x00000000U
#define CAN_RTR_DATA 0x00000000U
#define CAN_RX_FIFO0 0x00000000U
#endif

#define XCP_CTO_CAN_ID 0x555U /* Master -> Slave Command */
#define XCP_DTO_CAN_ID 0x556U /* Slave -> Master Response/DAQ */
#define XCP_STATION_ID 0x0001U

static SYN_XCP_Slave g_xcp_slave;
extern CAN_HandleTypeDef hcan1;

void xcp_app_init(void)
{
    syn_xcp_init(&g_xcp_slave, XCP_STATION_ID);
}

void xcp_app_loop(uint32_t dt_ms)
{
    (void)dt_ms;

    /* Service periodic XCP DAQ sampling event (event channel 0 triggered every 10ms) */
    uint8_t daq_dto[8];
    uint8_t list_idx = 0;
    uint8_t odt_idx = 0;

    if (syn_xcp_service_daq(&g_xcp_slave, 0U, daq_dto, &list_idx, &odt_idx)) {
        CAN_TxHeaderTypeDef tx_hdr = {0};
        tx_hdr.StdId = XCP_DTO_CAN_ID;
        tx_hdr.IDE = CAN_ID_STD;
        tx_hdr.RTR = CAN_RTR_DATA;
        tx_hdr.DLC = 8U;

        uint32_t tx_mailbox = 0;
        /* Transmission queue call (HAL_CAN_AddTxMessage) */
        (void)tx_mailbox;
    }
}

/**
 * @brief STM32 HAL CAN Rx FIFO 0 Interrupt Callback for XCP CTO frame ingestion.
 */
void HAL_CAN_RxFifo0MsgPendingCallback_XCP(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_hdr;
    uint8_t rx_data[8];

    if (rx_hdr.StdId == XCP_CTO_CAN_ID) {
        uint8_t dto_resp[8];
        if (syn_xcp_process_cto(&g_xcp_slave, rx_data, dto_resp)) {
            CAN_TxHeaderTypeDef tx_hdr = {0};
            tx_hdr.StdId = XCP_DTO_CAN_ID;
            tx_hdr.IDE = CAN_ID_STD;
            tx_hdr.RTR = CAN_RTR_DATA;
            tx_hdr.DLC = 8U;

            uint32_t tx_mailbox = 0;
            /* Transmit DTO response */
            (void)tx_mailbox;
        }
    }
}

int main(void)
{
    xcp_app_init();

    while (1) {
        xcp_app_loop(10U);
    }
    return 0;
}
