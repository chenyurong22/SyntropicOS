/**
 * @file main.c
 * @brief SyntropicOS NMEA 2000 Marine CAN Protocol STM32 HAL Integration Example.
 *
 * Demonstrates non-blocking, zero-malloc NMEA 2000 (IEC 61162-3) marine CAN bus
 * PGN encoding/decoding (Position, COG/SOG, Heading, Battery, Environment)
 * integrated directly with STM32 HAL CAN drivers (`HAL_CAN_...`).
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target MCU header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Hardware CAN handle instance */
extern CAN_HandleTypeDef hcan1;

/* N2K Node Source Address (Default SA = 35 for Marine Sensor / Navigation Node) */
static uint8_t n2k_sa = 35;
static uint8_t sequence_id = 0;

/* Static Fast-Packet RX Session Context (Zero Dynamic Allocation) */
static SYN_N2K_FastPacketRx fp_rx_session;

/**
 * @brief Initialize NMEA 2000 Marine Node parameters and Fast-Packet context.
 */
void n2k_app_init(void)
{
    n2k_sa = 35;
    sequence_id = 0;
    memset(&fp_rx_session, 0, sizeof(fp_rx_session));
}

/**
 * @brief Send a raw 29-bit Extended CAN frame via STM32 HAL CAN driver.
 */
static bool send_hal_can_frame(const SYN_CAN_Frame *frame)
{
    CAN_TxHeaderTypeDef tx_hdr = {0};
    tx_hdr.ExtId = frame->id;
    tx_hdr.IDE = CAN_ID_EXT; /* NMEA 2000 always uses 29-bit Extended CAN Identifiers */
    tx_hdr.RTR = CAN_RTR_DATA;
    tx_hdr.DLC = frame->dlc;

    uint32_t tx_mailbox;
    return HAL_CAN_AddTxMessage(&hcan1, &tx_hdr, (uint8_t *)frame->data, &tx_mailbox) == HAL_OK;
}

/**
 * @brief STM32 HAL CAN Rx FIFO 0 Interrupt Callback.
 *
 * Invoked by STM32 HAL whenever a raw 29-bit CAN frame arrives on FIFO 0.
 * Ingests and decodes NMEA 2000 PGNs.
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1) {
        CAN_RxHeaderTypeDef rx_hdr;
        SYN_CAN_Frame frame;

        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_hdr, frame.data) == HAL_OK) {
            frame.id = (rx_hdr.IDE == CAN_ID_EXT) ? rx_hdr.ExtId : rx_hdr.StdId;
            frame.dlc = (uint8_t)rx_hdr.DLC;

            /* Extract 18-bit J1939/N2K Header information */
            SYN_J1939_Header header;
            if (syn_j1939_id_unpack(frame.id, &header) == SYN_OK) {
                switch (header.pgn) {
                case SYN_N2K_PGN_POS_RAPID: {
                    SYN_N2K_PositionRapid pos;
                    if (syn_n2k_decode_position_rapid(&frame, &pos) == SYN_OK) {
                        /* Decoded Rapid Position: pos.latitude_1e7, pos.longitude_1e7 */
                    }
                    break;
                }
                case SYN_N2K_PGN_COG_SOG_RAPID: {
                    SYN_N2K_CogSogRapid cog_sog;
                    if (syn_n2k_decode_cog_sog_rapid(&frame, &cog_sog) == SYN_OK) {
                        /* Decoded COG & SOG: cog_sog.cog_rad_1e4, cog_sog.sog_m_s_1e2 */
                    }
                    break;
                }
                case SYN_N2K_PGN_BATTERY_STATUS: {
                    SYN_N2K_BatteryStatus bat;
                    if (syn_n2k_decode_battery(&frame, &bat) == SYN_OK) {
                        /* Decoded Battery Status: bat.voltage_1e2 (0.01V), bat.current_1e1 (0.1A) */
                    }
                    break;
                }
                default:
                    break;
                }
            }
        }
    }
}

/**
 * @brief Broadcast Rapid Update GNSS Position (PGN 129025 - 10 Hz rate).
 *
 * @param lat_deg Latitude in degrees (e.g. 37.774929)
 * @param lon_deg Longitude in degrees (e.g. -122.419416)
 */
void n2k_broadcast_position_rapid(double lat_deg, double lon_deg)
{
    SYN_N2K_PositionRapid pos;
    pos.latitude_1e7 = (int32_t)(lat_deg * 1e7);
    pos.longitude_1e7 = (int32_t)(lon_deg * 1e7);

    SYN_CAN_Frame frame;
    if (syn_n2k_encode_position_rapid(n2k_sa, &pos, &frame) == SYN_OK) {
        send_hal_can_frame(&frame);
    }
}

/**
 * @brief Broadcast Rapid Update Course Over Ground & Speed Over Ground (PGN 129026 - 4 Hz rate).
 *
 * @param cog_deg Course Over Ground in degrees (0.0 to 360.0)
 * @param sog_knots Speed Over Ground in knots (1 knot = 0.514444 m/s)
 */
void n2k_broadcast_cog_sog_rapid(float cog_deg, float sog_knots)
{
    SYN_N2K_CogSogRapid cog_sog;
    cog_sog.sid = sequence_id++;
    cog_sog.cog_ref = 0; /* 0 = True, 1 = Magnetic */

    /* Convert cog_deg (0-360) to radians in 1e-4 units */
    float cog_rad = cog_deg * (3.14159265f / 180.0f);
    cog_sog.cog_rad_1e4 = (uint16_t)(cog_rad * 10000.0f);

    /* Convert knots to m/s in 1e-2 units */
    float sog_m_s = sog_knots * 0.514444f;
    cog_sog.sog_m_s_1e2 = (uint16_t)(sog_m_s * 100.0f);

    SYN_CAN_Frame frame;
    if (syn_n2k_encode_cog_sog_rapid(n2k_sa, &cog_sog, &frame) == SYN_OK) {
        send_hal_can_frame(&frame);
    }
}

/**
 * @brief Broadcast Vessel Heading (PGN 127250 - 10 Hz rate).
 *
 * @param heading_deg Heading in degrees (0.0 to 360.0)
 */
void n2k_broadcast_vessel_heading(float heading_deg)
{
    SYN_N2K_VesselHeading heading;
    heading.sid = sequence_id++;
    heading.heading_ref = 0; /* 0 = True, 1 = Magnetic */

    float heading_rad = heading_deg * (3.14159265f / 180.0f);
    heading.heading_rad_1e4 = (uint16_t)(heading_rad * 10000.0f);
    heading.deviation_rad_1e4 = 0;
    heading.variation_rad_1e4 = 0;

    SYN_CAN_Frame frame;
    if (syn_n2k_encode_heading(n2k_sa, &heading, &frame) == SYN_OK) {
        send_hal_can_frame(&frame);
    }
}

/**
 * @brief Broadcast House Battery Bank Telemetry (PGN 127508 - 1 Hz rate).
 *
 * @param voltage_v Battery Voltage in Volts (e.g. 13.62 V)
 * @param current_a Battery Current in Amperes (e.g. 24.5 A)
 */
void n2k_broadcast_battery_status(float voltage_v, float current_a)
{
    SYN_N2K_BatteryStatus bat;
    bat.sid = sequence_id++;
    bat.instance = 1; /* Battery Bank #1 */
    bat.voltage_1e2 = (uint16_t)(voltage_v * 100.0f);
    bat.current_1e1 = (int16_t)(current_a * 10.0f);
    bat.temperature_1e1 = 2982; /* 298.2 Kelvin (25.0 °C) */

    SYN_CAN_Frame frame;
    if (syn_n2k_encode_battery(n2k_sa, &bat, &frame) == SYN_OK) {
        send_hal_can_frame(&frame);
    }
}

/**
 * @brief Periodic 100ms NMEA 2000 application task routine.
 */
void n2k_app_task_100ms(void)
{
    /* 1. Broadcast Rapid Position (10 Hz) */
    n2k_broadcast_position_rapid(37.774929, -122.419416);

    /* 2. Broadcast Vessel Heading (10 Hz) */
    n2k_broadcast_vessel_heading(145.2f);

    /* 3. Broadcast COG/SOG (4 Hz / every 250ms) */
    static uint8_t cog_ticks = 0;
    if (++cog_ticks >= 2) {
        cog_ticks = 0;
        n2k_broadcast_cog_sog_rapid(146.5f, 12.4f);
    }

    /* 4. Broadcast Battery Telemetry (1 Hz / every 1000ms) */
    static uint8_t bat_ticks = 0;
    if (++bat_ticks >= 10) {
        bat_ticks = 0;
        n2k_broadcast_battery_status(13.62f, 18.5f);
    }
}
