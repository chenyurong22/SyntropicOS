/**
 * @file main.c
 * @brief SyntropicOS CANopen to NMEA 2000 Marine CAN Gateway Example.
 *
 * Demonstrates a non-blocking, zero-malloc gateway on STM32 translating
 * CANopen DS301 Object Dictionary telemetry (battery voltage, current, SOC, alerts)
 * into standard NMEA 2000 (IEC 61162-3) marine CAN broadcast messages every 1500ms.
 *
 * Broadcast NMEA 2000 Messages (1500ms period):
 *   1. Battery Status       - CAN ID 0x19F214F3 (PGN 127508 / 0x1F214, Priority 6, SA 0xF3)
 *   2. DC Detailed Status   - CAN ID 0x19F212F3 (PGN 127506 / 0x1F212, Priority 6, SA 0xF3)
 *   3. DC Voltage / Current - CAN ID 0x19F307F3 (PGN 127751 / 0x1F307, Priority 6, SA 0xF3)
 *   4. Alert Status         - CAN ID 0x09F007F3 (PGN 126983 / 0x0F007, Priority 2, SA 0xF3)
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target MCU header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Hardware CAN handle instance */
extern CAN_HandleTypeDef hcan1;

/* ── Gateway CAN Configuration ───────────────────────────────────────────── */

#define CANOPEN_NODE_ID     0x20 /* CANopen Gateway Node-ID (32) */
#define N2K_SOURCE_ADDR     0xF3 /* NMEA 2000 Gateway Source Address (243 / 0xF3) */
#define N2K_BROADCAST_MS    1500 /* Periodic Broadcast Interval (1500 ms) */

/* ── Live Object Dictionary & Telemetry Variables ────────────────────────── */

static uint32_t od_device_type = 0x00000192;       /* CiA 402 / Motion / Power Device */
static uint8_t  od_error_register = 0x00;           /* Error Register */
static uint16_t od_battery_voltage_1e2 = 1380;     /* 13.80 V (0.01V units) */
static int16_t  od_battery_current_1e1 = 250;      /* 25.0 A (0.1A units) */
static uint16_t od_battery_temp_1e1 = 2982;        /* 298.2 K / 25.0 °C (0.1K units) */
static uint8_t  od_state_of_charge = 85;           /* 85 % SOC */
static uint8_t  od_state_of_health = 98;           /* 98 % SOH */
static uint16_t od_time_to_go_min = 720;           /* 720 min (12 hours) */
static uint16_t od_capacity_ah_1e1 = 2000;         /* 200.0 Ah (0.1Ah units) */
static uint8_t  od_alert_severity = 0;             /* 0 = Normal, 1 = Warning, 2 = Alarm */
static uint32_t od_alert_flags = 0x00000000;       /* Alert status bitmask */

/* ── CANopen Object Dictionary Table ─────────────────────────────────────── */

static const SYN_CANOpenODEntry od_table[] = {
    {0x1000, 0x00, SYN_CANOPEN_TYPE_U32, SYN_CANOPEN_ACCESS_RO, &od_device_type, sizeof(od_device_type)},
    {0x1001, 0x00, SYN_CANOPEN_TYPE_U8, SYN_CANOPEN_ACCESS_RO, &od_error_register, sizeof(od_error_register)},
    {0x6000, 0x01, SYN_CANOPEN_TYPE_U32, SYN_CANOPEN_ACCESS_RO, &od_alert_flags, sizeof(od_alert_flags)},
    {0x6000, 0x02, SYN_CANOPEN_TYPE_U8, SYN_CANOPEN_ACCESS_RO, &od_alert_severity, sizeof(od_alert_severity)},
    {0x6401, 0x01, SYN_CANOPEN_TYPE_U16, SYN_CANOPEN_ACCESS_RW, &od_battery_voltage_1e2, sizeof(od_battery_voltage_1e2)},
    {0x6401, 0x02, SYN_CANOPEN_TYPE_I16, SYN_CANOPEN_ACCESS_RW, &od_battery_current_1e1, sizeof(od_battery_current_1e1)},
    {0x6401, 0x03, SYN_CANOPEN_TYPE_U16, SYN_CANOPEN_ACCESS_RW, &od_battery_temp_1e1, sizeof(od_battery_temp_1e1)},
    {0x6402, 0x01, SYN_CANOPEN_TYPE_U8, SYN_CANOPEN_ACCESS_RW, &od_state_of_charge, sizeof(od_state_of_charge)},
    {0x6402, 0x02, SYN_CANOPEN_TYPE_U8, SYN_CANOPEN_ACCESS_RW, &od_state_of_health, sizeof(od_state_of_health)},
    {0x6402, 0x03, SYN_CANOPEN_TYPE_U16, SYN_CANOPEN_ACCESS_RW, &od_time_to_go_min, sizeof(od_time_to_go_min)},
};

/* CANopen Node Handle */
static SYN_CANOpenNode canopen_node;
static uint8_t n2k_sid = 0; /* NMEA 2000 Sequence ID counter */

/* ── Hardware CAN Transmission Helpers ───────────────────────────────────── */

/**
 * @brief Send 11-bit Standard CAN frame (CANopen COB-ID).
 */
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

/**
 * @brief Send 29-bit Extended CAN frame (NMEA 2000 / J1939 PGN).
 */
static bool send_hal_can_ext(uint32_t ext_id, const uint8_t *data, uint8_t len)
{
    CAN_TxHeaderTypeDef tx_hdr = {0};
    tx_hdr.ExtId = ext_id;
    tx_hdr.IDE = CAN_ID_EXT;
    tx_hdr.RTR = CAN_RTR_DATA;
    tx_hdr.DLC = len;

    uint32_t tx_mailbox;
    return HAL_CAN_AddTxMessage(&hcan1, &tx_hdr, (uint8_t *)data, &tx_mailbox) == HAL_OK;
}

/* ── NMEA 2000 Broadcast Encoders ────────────────────────────────────────── */

/**
 * @brief Broadcast Battery Status (0x19F214F3 - PGN 127508, Priority 6, SA 0xF3, 1500ms).
 */
static void broadcast_n2k_battery_status(void)
{
    SYN_CAN_Frame frame;
    SYN_N2K_BatteryStatus bat = {
        .sid = n2k_sid,
        .instance = 0,
        .voltage_1e2 = od_battery_voltage_1e2,
        .current_1e1 = od_battery_current_1e1,
        .temperature_1e1 = od_battery_temp_1e1
    };

    if (syn_n2k_encode_battery(N2K_SOURCE_ADDR, &bat, &frame) == SYN_OK) {
        /* Force exact CAN ID 0x19F214F3 specified by N2K architecture */
        frame.id = 0x19F214F3;
        send_hal_can_ext(frame.id, frame.data, frame.dlc);
    }
}

/**
 * @brief Broadcast DC Detailed Status (0x19F212F3 - PGN 127506, Priority 6, SA 0xF3, 1500ms).
 */
static void broadcast_n2k_dc_detailed_status(void)
{
    SYN_CAN_Frame frame;
    SYN_N2K_DcDetailedStatus dc = {
        .sid = n2k_sid,
        .instance = 0,
        .dc_type = 0, /* 0 = Battery */
        .state_of_charge = od_state_of_charge,
        .state_of_health = od_state_of_health,
        .time_to_go_min = od_time_to_go_min,
        .capacity_ah_1e1 = (uint16_t)(od_capacity_ah_1e1 / 10)
    };

    if (syn_n2k_encode_dc_detailed(N2K_SOURCE_ADDR, &dc, &frame) == SYN_OK) {
        /* Force exact CAN ID 0x19F212F3 specified by N2K architecture */
        frame.id = 0x19F212F3;
        send_hal_can_ext(frame.id, frame.data, frame.dlc);
    }
}

/**
 * @brief Broadcast DC Voltage / Current (0x19F307F3 - PGN 127751, Priority 6, SA 0xF3, 1500ms).
 */
static void broadcast_n2k_dc_voltage_current(void)
{
    uint8_t payload[8];
    size_t idx = 0;

    syn_pack_u8(payload, &idx, n2k_sid);                                /* Byte 0: Sequence ID */
    syn_pack_u8(payload, &idx, 0x00);                                   /* Byte 1: DC Bus Instance (0) */
    syn_pack_u16_le(payload, &idx, od_battery_voltage_1e2);             /* Bytes 2-3: DC Voltage (0.01V) */
    syn_pack_u16_le(payload, &idx, (uint16_t)od_battery_current_1e1);   /* Bytes 4-5: DC Current (0.1A) */
    syn_pack_u16_le(payload, &idx, 0xFFFF);                             /* Bytes 6-7: Ripple Voltage / Reserved */

    /* Extended 29-bit CAN ID: 0x19F307F3 (Priority 6, PGN 127751, SA 0xF3) */
    send_hal_can_ext(0x19F307F3, payload, sizeof(payload));
}

/**
 * @brief Broadcast Alert Status (0x09F007F3 - PGN 126983, Priority 2, SA 0xF3, 1500ms).
 */
static void broadcast_n2k_alert_status(void)
{
    uint8_t payload[8];
    size_t idx = 0;

    syn_pack_u8(payload, &idx, n2k_sid);                     /* Byte 0: Sequence ID */
    syn_pack_u8(payload, &idx, 0x00);                        /* Byte 1: Alert Instance / Bank */
    syn_pack_u8(payload, &idx, 0x01);                        /* Byte 2: Alert System (Power / DC Electrical) */
    syn_pack_u8(payload, &idx, od_alert_severity);           /* Byte 3: Severity (0=Normal, 1=Warning, 2=Alarm) */
    syn_pack_u32_le(payload, &idx, od_alert_flags);          /* Bytes 4-7: Active Alert Bitmask Flags */

    /* Extended 29-bit CAN ID: 0x09F007F3 (Priority 2, PGN 126983, SA 0xF3) */
    send_hal_can_ext(0x09F007F3, payload, sizeof(payload));
}

/* ── Gateway Application Init & Loop ─────────────────────────────────────── */

/**
 * @brief Initialize Gateway Protocol Stacks.
 */
void gateway_app_init(void)
{
    SYN_CANOpenNodeConfig cfg = {0};
    cfg.node_id = CANOPEN_NODE_ID;
    cfg.heartbeat_ms = 1000;

    /* Initialize CANopen DS301 Slave Engine */
    syn_canopen_init(&canopen_node, &cfg, od_table, sizeof(od_table) / sizeof(od_table[0]));

    n2k_sid = 0;
}

/**
 * @brief STM32 HAL CAN RX Interrupt Callback.
 * Distinguishes 11-bit CANopen frames vs 29-bit NMEA 2000 frames.
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1) {
        CAN_RxHeaderTypeDef rx_hdr;
        uint8_t rx_data[8];

        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_hdr, rx_data) == HAL_OK) {
            if (rx_hdr.IDE == CAN_ID_STD) {
                /* Ingest 11-bit Standard CANopen Frame (SDO / RPDO / NMT) */
                syn_canopen_process_rx(&canopen_node, rx_hdr.StdId, rx_data, (uint8_t)rx_hdr.DLC);
            } else if (rx_hdr.IDE == CAN_ID_EXT) {
                /* Ingest 29-bit Extended NMEA 2000 Frame */
                SYN_J1939_Header n2k_hdr;
                if (syn_j1939_id_unpack(rx_hdr.ExtId, &n2k_hdr) == SYN_OK) {
                    /* Process incoming N2K queries or requests if needed */
                }
            }
        }
    }
}

/**
 * @brief Gateway Main Cooperative Task.
 * Executes CANopen housekeeping and broadcasts 4 NMEA 2000 messages every 1500ms.
 */
void gateway_app_process(uint32_t now_ms)
{
    static uint32_t last_n2k_tx_ms = 0;

    /* 1. CANopen DS301 Node Housekeeping (Heartbeat & SDO timers) */
    static uint32_t last_canopen_ms = 0;
    uint32_t delta_ms = now_ms - last_canopen_ms;
    last_canopen_ms = now_ms;

    syn_canopen_update(&canopen_node, delta_ms);

    uint8_t tx_buf[8];
    uint32_t tx_cob_id = 0;
    uint8_t tx_len = 0;
    while (syn_canopen_get_tx(&canopen_node, &tx_cob_id, tx_buf, &tx_len)) {
        send_hal_can_std(tx_cob_id, tx_buf, tx_len);
    }

    /* 2. NMEA 2000 Periodic Broadcast Scheduler (Every 1500 ms) */
    if ((now_ms - last_n2k_tx_ms) >= N2K_BROADCAST_MS) {
        last_n2k_tx_ms = now_ms;

        /* Broadcast all 4 NMEA 2000 Messages */
        broadcast_n2k_battery_status();     /* 0x19F214F3 - 1500ms */
        broadcast_n2k_dc_detailed_status(); /* 0x19F212F3 - 1500ms */
        broadcast_n2k_dc_voltage_current(); /* 0x19F307F3 - 1500ms */
        broadcast_n2k_alert_status();      /* 0x09F007F3 - 1500ms */

        n2k_sid++; /* Increment sequence counter */
    }
}

/**
 * @brief Application Entry Point.
 */
int main(void)
{
    /* Initialize STM32 HAL, System Clocks & CAN1 Peripheral */
    HAL_Init();
    /* SystemClock_Config(); */
    /* MX_CAN1_Init(); */

    gateway_app_init();

    while (1) {
        uint32_t now_ms = HAL_GetTick();
        gateway_app_process(now_ms);
    }

    return 0;
}
