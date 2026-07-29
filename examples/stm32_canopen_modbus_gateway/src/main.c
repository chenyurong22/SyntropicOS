/**
 * @file main.c
 * @brief SyntropicOS CANopen to Modbus RTU/TCP Gateway Example.
 *
 * Demonstrates a non-blocking gateway on STM32 translating CANopen DS301/CiA 418
 * Object Dictionary entries into Modbus RTU/TCP holding registers (FC03/FC06/FC16).
 */

#include "stm32f4xx_hal.h"
#include "syntropic/syntropic.h"

/* Hardware CAN & USART handle instances */
extern CAN_HandleTypeDef hcan1;
extern UART_HandleTypeDef huart2;

/* ── Gateway Configuration ───────────────────────────────────────────────── */

#define CANOPEN_NODE_ID 0x20 /* CANopen Gateway Node-ID (32) */
#define MODBUS_SLAVE_ID 0x01 /* Modbus Slave ID (1) */
#define MODBUS_REG_COUNT 10  /* 10 Holding Registers (40001..40010) */

/* ── Live Telemetry & Object Dictionary Variables ────────────────────────── */

static uint32_t od_device_type = 0x00000192;   /* CiA 402 Device */
static uint8_t od_error_register = 0x00;       /* Error Register */
static uint16_t od_battery_voltage_1e2 = 1380; /* 13.80 V */
static int16_t od_battery_current_1e1 = 250;   /* 25.0 A */
static uint16_t od_battery_temp_1e1 = 2982;    /* 25.0 °C */
static uint8_t od_state_of_charge = 85;        /* 85 % SOC */
static uint8_t od_state_of_health = 98;        /* 98 % SOH */
static uint16_t od_time_to_go_min = 720;       /* 720 min */
static uint8_t od_alert_severity = 0;          /* Alert severity */
static uint32_t od_alert_flags = 0x00000000;   /* Alert bitmask */

/* ── CANopen Object Dictionary Table ─────────────────────────────────────── */

static const SYN_CANOpenODEntry od_table[] = {
    {0x1000, 0x00, SYN_CANOPEN_TYPE_U32, SYN_CANOPEN_ACCESS_RO, &od_device_type,
     sizeof(od_device_type)},
    {0x1001, 0x00, SYN_CANOPEN_TYPE_U8, SYN_CANOPEN_ACCESS_RO, &od_error_register,
     sizeof(od_error_register)},
    {0x6000, 0x01, SYN_CANOPEN_TYPE_U32, SYN_CANOPEN_ACCESS_RO, &od_alert_flags,
     sizeof(od_alert_flags)},
    {0x6000, 0x02, SYN_CANOPEN_TYPE_U8, SYN_CANOPEN_ACCESS_RO, &od_alert_severity,
     sizeof(od_alert_severity)},
    {0x6401, 0x01, SYN_CANOPEN_TYPE_U16, SYN_CANOPEN_ACCESS_RW, &od_battery_voltage_1e2,
     sizeof(od_battery_voltage_1e2)},
    {0x6401, 0x02, SYN_CANOPEN_TYPE_I16, SYN_CANOPEN_ACCESS_RW, &od_battery_current_1e1,
     sizeof(od_battery_current_1e1)},
    {0x6401, 0x03, SYN_CANOPEN_TYPE_U16, SYN_CANOPEN_ACCESS_RW, &od_battery_temp_1e1,
     sizeof(od_battery_temp_1e1)},
    {0x6402, 0x01, SYN_CANOPEN_TYPE_U8, SYN_CANOPEN_ACCESS_RW, &od_state_of_charge,
     sizeof(od_state_of_charge)},
    {0x6402, 0x02, SYN_CANOPEN_TYPE_U8, SYN_CANOPEN_ACCESS_RW, &od_state_of_health,
     sizeof(od_state_of_health)},
    {0x6402, 0x03, SYN_CANOPEN_TYPE_U16, SYN_CANOPEN_ACCESS_RW, &od_time_to_go_min,
     sizeof(od_time_to_go_min)},
};

/* Protocol Handles */
static SYN_CANOpenNode canopen_node;
static uint16_t modbus_regs[MODBUS_REG_COUNT];

/* ── Hardware Transmission Helpers ───────────────────────────────────────── */

static bool send_hal_can_std(uint32_t cob_id, const uint8_t *data, uint8_t len)
{
    CAN_TxHeaderTypeDef tx_hdr = {0};
    tx_hdr.StdId = cob_id;
    tx_hdr.IDE = CAN_ID_STD;
    tx_hdr.RTR = CAN_RTR_DATA;
    tx_hdr.DLC = len;

    uint32_t tx_mailbox;
    return HAL_CAN_AddTxMessage(&hcan1, &tx_hdr, (uint8_t *)data, &tx_mailbox) == HAL_OK;
}

/* ── Modbus Holding Register Sync ────────────────────────────────────────── */

/**
 * @brief Synchronize CANopen OD Telemetry into Modbus Holding Registers (40001..40010).
 */
static void sync_od_to_modbus_regs(void)
{
    modbus_regs[0] = od_battery_voltage_1e2;
    modbus_regs[1] = (uint16_t)od_battery_current_1e1;
    modbus_regs[2] = od_battery_temp_1e1;
    modbus_regs[3] = (uint16_t)od_state_of_charge;
    modbus_regs[4] = (uint16_t)od_state_of_health;
    modbus_regs[5] = od_time_to_go_min;
    modbus_regs[6] = (uint16_t)od_alert_severity;
    modbus_regs[7] = (uint16_t)(od_alert_flags & 0xFFFFU);
    modbus_regs[8] = (uint16_t)((od_alert_flags >> 16U) & 0xFFFFU);
    modbus_regs[9] = (uint16_t)od_error_register;
}

/* ── Gateway Application Init & Loop ─────────────────────────────────────── */

void gateway_app_init(void)
{
    SYN_CANOpenNodeConfig cfg = {0};
    cfg.node_id = CANOPEN_NODE_ID;
    cfg.heartbeat_ms = 1000;

    syn_canopen_init(&canopen_node, &cfg, od_table, sizeof(od_table) / sizeof(od_table[0]));
    sync_od_to_modbus_regs();
}

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

void gateway_app_process(uint32_t now_ms)
{
    uint8_t tx_buf[8];
    uint32_t tx_cob_id = 0;
    uint8_t tx_len = 0;

    if (syn_canopen_process_tx(&canopen_node, now_ms, &tx_cob_id, tx_buf, &tx_len)) {
        send_hal_can_std(tx_cob_id, tx_buf, tx_len);
    }

    sync_od_to_modbus_regs();
}

int main(void)
{
    HAL_Init();
    gateway_app_init();

    while (1) {
        uint32_t now_ms = HAL_GetTick();
        gateway_app_process(now_ms);
    }

    return 0;
}
