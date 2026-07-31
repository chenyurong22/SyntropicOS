/**
 * @file main.c
 * @brief STM32 HAL CCP (CAN Calibration Protocol v2.1) Slave Example.
 */

#include "syntropic/proto/syn_ccp.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* STM32 HAL CAN Header Mock/Includes for standalone/embedded compile */
#if defined(STM32F4xx) || defined(STM32F7xx) || defined(STM32G4xx)
#include "stm32_hal.h"
#else
typedef struct {
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
    uint8_t TransmitGlobalTime;
} CAN_TxHeaderTypeDef;

typedef struct {
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
    uint32_t Timestamp;
    uint32_t FilterMatchIndex;
} CAN_RxHeaderTypeDef;

typedef struct {
    void *Instance;
} CAN_HandleTypeDef;

#define CAN_ID_STD 0x00000000U
#define CAN_RTR_DATA 0x00000000U
#define CAN_RX_FIFO0 0x00000000U
#define HAL_OK 0x00U
#endif

#define CCP_CRO_CAN_ID 0x600U /* Command Receive Object (Master -> Slave) */
#define CCP_DTO_CAN_ID 0x601U /* Data Transmission Object (Slave -> Master) */
#define CCP_STATION_ADDR 0x0001U

static SYN_CCP_Slave g_ccp_slave;
extern CAN_HandleTypeDef hcan1;

/* Target ECU calibration parameter example */
static uint32_t g_target_engine_rpm = 2500U;
static uint16_t g_target_coolant_temp = 85U;

void ccp_app_init(void)
{
    syn_ccp_init(&g_ccp_slave, CCP_STATION_ADDR);
    (void)g_target_engine_rpm;
    (void)g_target_coolant_temp;
}

void ccp_app_loop(uint32_t dt_ms)
{
    (void)dt_ms;

    /* Service periodic DAQ list sampling (e.g., event channel 0 triggered every 10ms) */
    uint8_t daq_dto[8];
    uint8_t list_idx = 0;
    uint8_t odt_idx = 0;

    if (syn_ccp_service_daq(&g_ccp_slave, 0U, daq_dto, &list_idx, &odt_idx)) {
        CAN_TxHeaderTypeDef tx_hdr = {0};
        tx_hdr.StdId = CCP_DTO_CAN_ID;
        tx_hdr.IDE = CAN_ID_STD;
        tx_hdr.RTR = CAN_RTR_DATA;
        tx_hdr.DLC = 8U;

        uint32_t tx_mailbox = 0;
        /* Transmission queue call (HAL_CAN_AddTxMessage) */
        (void)tx_hdr;
        (void)tx_mailbox;
    }
}

/**
 * @brief STM32 HAL CAN Rx FIFO 0 Interrupt Callback for CCP CRO frame ingestion.
 */
void HAL_CAN_RxFifo0MsgPendingCallback_CCP(CAN_HandleTypeDef *hcan)
{
    (void)hcan;
    CAN_RxHeaderTypeDef rx_hdr = {0};
    uint8_t rx_data[8] = {0};

    /* Simulating HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_hdr, rx_data) */
    if (rx_hdr.StdId == CCP_CRO_CAN_ID) {
        uint8_t dto_resp[8];
        if (syn_ccp_process_cro(&g_ccp_slave, rx_data, dto_resp)) {
            CAN_TxHeaderTypeDef tx_hdr = {0};
            tx_hdr.StdId = CCP_DTO_CAN_ID;
            tx_hdr.IDE = CAN_ID_STD;
            tx_hdr.RTR = CAN_RTR_DATA;
            tx_hdr.DLC = 8U;

            uint32_t tx_mailbox = 0;
            /* Transmit DTO response */
            (void)tx_hdr;
            (void)tx_mailbox;
        }
    }
}

int main(void)
{
    ccp_app_init();

    while (1) {
        ccp_app_loop(10U);
    }
    return 0;
}
