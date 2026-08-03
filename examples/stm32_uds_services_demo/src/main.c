/**
 * @file main.c
 * @brief SyntropicOS ISO 14229-1 (UDS) Multi-Service Integration Example.
 *
 * Demonstrates complete ISO 14229-1 diagnostic server setup with callback handlers for:
 *  - 0x83 AccessTimingParameter (#95)
 *  - 0x84 SecuredDataTransmission (#101)
 *  - 0x31 RoutineControl (#104)
 *  - 0x2F InputOutputControlByIdentifier (#100)
 *  - 0x87 LinkControl (#103)
 *  - 0x86 ResponseOnEvent (#102)
 *  - 0x24 ReadScalingDataByIdentifier (#97)
 *  - 0x2A ReadDataByPeriodicIdentifier (#99)
 *  - 0x2C DynamicallyDefineDataIdentifier (#98)
 */

#include "syntropic/proto/syn_isotp.h"
#include "syntropic/proto/syn_uds.h"
#include "syntropic/syntropic.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Static UDS Server Instance */
static SYN_UDS_Server g_uds_server;

/* Shared Data Identifiers (DIDs) */
static uint8_t g_vin_data[17] = "SYNTROPICOS123456";
static uint8_t g_battery_volts[2] = {0x00, 0x78}; /* 12.0 Volts */

/* 1. Service 0x83 AccessTimingParameter Callback (#95) */
static bool on_access_timing(SYN_UDS_AccessTimingType timing_type, uint16_t *p2_max_ms,
                             uint16_t *p2_star_max_10ms, void *ctx)
{
    (void)ctx;
    switch (timing_type) {
    case SYN_UDS_TIMING_READ_EXTENDED:
    case SYN_UDS_TIMING_READ_ACTIVE:
        if (p2_max_ms != NULL) *p2_max_ms = 50U;
        if (p2_star_max_10ms != NULL) *p2_star_max_10ms = 500U;
        return true;
    case SYN_UDS_TIMING_SET_TO_DEFAULT:
        if (p2_max_ms != NULL) *p2_max_ms = 50U;
        if (p2_star_max_10ms != NULL) *p2_star_max_10ms = 500U;
        return true;
    case SYN_UDS_TIMING_SET_TO_GIVEN:
        return (p2_max_ms != NULL && *p2_max_ms >= 10U);
    default:
        return false;
    }
}

/* 2. Service 0x84 SecuredDataTransmission Callback (#101) */
static bool on_secured_data(const uint8_t *in_data, uint16_t in_len, uint8_t *out_buf,
                            uint16_t max_out_len, uint16_t *out_len, void *ctx)
{
    (void)ctx;
    if (in_len == 0 || max_out_len < in_len) {
        return false;
    }
    /* Basic payload decryption / processing */
    for (uint16_t i = 0; i < in_len; i++) {
        out_buf[i] = in_data[i] ^ 0xAA;
    }
    *out_len = in_len;
    return true;
}

/* 3. Service 0x31 RoutineControl Callback (#104) */
static bool on_routine_control(uint8_t subfunction, uint16_t routine_id, const uint8_t *in_data,
                               uint16_t in_len, uint8_t *out_buf, uint16_t max_out_len,
                               uint16_t *out_len, void *ctx)
{
    (void)in_data; (void)in_len; (void)ctx;
    if (routine_id == 0x0201) { /* Example #1-3: Wiring Harness Intermittent Test */
        if (subfunction == 0x01) { /* startRoutine */
            if (max_out_len < 1) return false;
            out_buf[0] = 0x32; /* routineStatusRecord: Started OK */
            *out_len = 1;
            return true;
        } else if (subfunction == 0x02) { /* stopRoutine */
            if (max_out_len < 1) return false;
            out_buf[0] = 0x30; /* routineStatusRecord: Stopped OK */
            *out_len = 1;
            return true;
        } else if (subfunction == 0x03) { /* requestRoutineResults */
            if (max_out_len < 2) return false;
            out_buf[0] = 0x30; /* routineStatusRecord: Test Finished */
            out_buf[1] = 0x33; /* Result code: All harness signals OK */
            *out_len = 2;
            return true;
        }
    } else if (routine_id == 0x0202) { /* Example #4: Transmission Gear Calibration */
        if (subfunction == 0x01) { /* startRoutine with routineControlOption */
            if (max_out_len < 1) return false;
            out_buf[0] = 0x00; /* Calibration started */
            *out_len = 1;
            return true;
        }
    }
    return false;
}

/* 4. Service 0x2F InputOutputControlByIdentifier Callback (#100) */
static bool on_io_control(uint16_t did, uint8_t control_opt, const uint8_t *in_data,
                          uint16_t in_len, uint8_t *out_buf, uint16_t max_out_len,
                          uint16_t *out_len, void *ctx)
{
    (void)ctx;
    if (did == 0x9B00) { /* Example #1: Air Inlet Door Position */
        if (control_opt == 0x03 && in_len >= 1 && max_out_len >= 2) { /* shortTermAdjustment */
            out_buf[0] = control_opt;
            out_buf[1] = in_data[0]; /* Apply requested door position % (e.g. 0x0A = 10%) */
            *out_len = 2;
            return true;
        } else if (control_opt == 0x00 && max_out_len >= 2) { /* returnControlToECU */
            out_buf[0] = control_opt;
            out_buf[1] = 0x50; /* ECU normal automatic mode (50%) */
            *out_len = 2;
            return true;
        } else if (control_opt == 0x01 && max_out_len >= 2) { /* resetToDefault */
            out_buf[0] = control_opt;
            out_buf[1] = 0x00; /* Default position 0% */
            *out_len = 2;
            return true;
        } else if (control_opt == 0x02 && max_out_len >= 2) { /* freezeCurrentState */
            out_buf[0] = control_opt;
            out_buf[1] = 0x1E; /* Frozen state 30% */
            *out_len = 2;
            return true;
        }
    } else if (did == 0x0155) { /* Example #2: EGR & IAC Composite Actuator Control */
        if (control_opt == 0x03 && in_len >= 5 && max_out_len >= 6) { /* shortTermAdjustment */
            out_buf[0] = control_opt;
            memcpy(&out_buf[1], in_data, 5); /* IAC Pintle, RPM, Pedal A/B, EGR Duty Cycle */
            *out_len = 6;
            return true;
        } else if (control_opt == 0x00 && max_out_len >= 6) { /* returnControlToECU */
            out_buf[0] = control_opt;
            out_buf[1] = 0x0A;
            out_buf[2] = 0x07;
            out_buf[3] = 0xD0;
            out_buf[4] = 0x11;
            out_buf[5] = 0x20;
            *out_len = 6;
            return true;
        }
    }
    return false;
}

/* 5. Service 0x87 LinkControl Callback (#103) */
static bool on_link_control(uint8_t subfunction, const uint8_t *in_data, uint16_t in_len,
                            uint8_t *out_buf, uint16_t max_out_len, uint16_t *out_len, void *ctx)
{
    (void)in_data; (void)in_len; (void)ctx;
    if (max_out_len < 1) return false;
    out_buf[0] = subfunction;
    *out_len = 1;

    if (subfunction == 0x01) { /* verifyModeTransitionWithFixedParameter (e.g. 0x05 = 115200 Baud) */
        return true;
    } else if (subfunction == 0x02) { /* verifyModeTransitionWithSpecificParameter (3-byte custom baud rate) */
        return true;
    } else if (subfunction == 0x03) { /* transitionMode: apply baud rate switch to hardware */
        return true;
    }
    return false;
}

/* 6. Service 0x86 ResponseOnEvent Callback (#102) */
static bool on_response_on_event(uint8_t subfunction, const uint8_t *in_data, uint16_t in_len,
                                 uint8_t *out_buf, uint16_t max_out_len, uint16_t *out_len,
                                 void *ctx)
{
    (void)ctx;
    if (max_out_len < 2 + in_len) return false;
    out_buf[0] = 0x01; /* numberOfIdentifiedEvents = 1 */
    out_buf[1] = (in_len >= 1) ? in_data[0] : 0x02; /* eventWindowTime (e.g. 0x08 = 80s or 0x02 = infinite) */
    if (in_len > 1) {
        memcpy(&out_buf[2], &in_data[1], in_len - 1); /* Echo eventTypeRecord & serviceToRespondToRecord */
        *out_len = 1 + in_len;
    } else {
        *out_len = 2;
    }
    (void)subfunction;
    return true;
}

/* 7. Service 0x24 ReadScalingDataByIdentifier Callback (#97) */
static bool on_scaling_data(uint16_t did, uint8_t *out_buf, uint16_t max_out_len,
                            uint16_t *out_len, void *ctx)
{
    (void)ctx;
    if (did == 0xF190 && max_out_len >= 2) { /* Example #1: VIN (0xF190) ASCII scaling */
        out_buf[0] = 0x6F; /* ASCII, 15 data bytes */
        out_buf[1] = 0x62; /* ASCII, 2 data bytes */
        *out_len = 2;
        return true;
    } else if (did == 0x0105 && max_out_len >= 9) { /* Example #2: Vehicle Speed (0x0105) Formula */
        out_buf[0] = 0x01; /* Unsigned numeric, 1 data byte */
        out_buf[1] = 0x95; /* Formula, 5 data bytes */
        out_buf[2] = 0x00; /* FormulaIdentifier: C0 * x + C1 */
        out_buf[3] = 0xE0; /* C0 high byte (75 * 10^-2) */
        out_buf[4] = 0x4B; /* C0 low byte */
        out_buf[5] = 0x00; /* C1 high byte (30 * 10^0) */
        out_buf[6] = 0x1E; /* C1 low byte */
        out_buf[7] = 0xA1; /* Unit/Format, 1 data byte */
        out_buf[8] = 0x30; /* Unit ID: km/h */
        *out_len = 9;
        return true;
    } else if (did == 0x0967 && max_out_len >= 3) { /* Example #3: Bit-mapped record (0x0967) */
        out_buf[0] = 0x22; /* BitMappedReportedWithoutMask, 2 data bytes */
        out_buf[1] = 0x03; /* DataRecord#1 validity mask */
        out_buf[2] = 0x43; /* DataRecord#2 validity mask */
        *out_len = 3;
        return true;
    }
    return false;
}

/* 8. Service 0x2A ReadDataByPeriodicIdentifier Callback (#99) */
static bool on_periodic_data(uint8_t mode, const uint8_t *in_data, uint16_t in_len,
                             uint8_t *out_buf, uint16_t max_out_len, uint16_t *out_len,
                             void *ctx)
{
    (void)ctx;
    if (mode == 0x04) { /* stopSending */
        *out_len = 0;
        return true;
    }
    /* Periodic transmission modes: 0x01 = Slow, 0x02 = Medium, 0x03 = Fast */
    if (mode >= 0x01 && mode <= 0x03 && in_len >= 1) {
        uint8_t pdid = in_data[0];
        if (pdid == 0xE3 && max_out_len >= 5) { /* Example #1: Periodic DID 0xE3 (Engine parameters) */
            out_buf[0] = 0xA6; /* Engine Coolant Temp (ECT) */
            out_buf[1] = 0x66; /* Throttle Position (TP) */
            out_buf[2] = 0x07; /* Engine Speed RPM high */
            out_buf[3] = 0x50; /* Engine Speed RPM low */
            out_buf[4] = 0x00; /* Vehicle Speed Sensor (VSS) */
            *out_len = 5;
            return true;
        } else if (pdid == 0x24 && max_out_len >= 5) { /* Example #2: Periodic DID 0x24 (Electrical/Pressure) */
            out_buf[0] = 0x78; /* Battery Positive Voltage */
            out_buf[1] = 0x64; /* Manifold Absolute Pressure (MAP) */
            out_buf[2] = 0x12; /* Mass Air Flow (MAF) */
            out_buf[3] = 0x60; /* Barometric Pressure */
            out_buf[4] = 0x32; /* Calculated Load */
            *out_len = 5;
            return true;
        }
    }
    *out_len = 0;
    return true;
}

/* 9. Service 0x2C DynamicallyDefineDataIdentifier Callback (#98) */
static bool on_dynamic_did(uint8_t subfunction, uint16_t dyn_did, const uint8_t *in_data,
                           uint16_t in_len, uint8_t *out_buf, uint16_t max_out_len,
                           uint16_t *out_len, void *ctx)
{
    (void)in_data; (void)in_len; (void)ctx;
    if (max_out_len < 3) return false;
    out_buf[0] = subfunction;
    out_buf[1] = (uint8_t)(dyn_did >> 8);
    out_buf[2] = (uint8_t)(dyn_did & 0xFF);
    *out_len = 3;

    if (subfunction == 0x01) { /* defineByIdentifier: DIDs 0x010A, 0x050B, 0x1234, 0x5678, 0x9ABC -> DDDDI 0xF301 */
        return (dyn_did == 0xF301 || dyn_did == 0xF302);
    } else if (subfunction == 0x02) { /* defineByMemoryAddress -> DDDDI 0xF302 */
        return (dyn_did == 0xF302);
    } else if (subfunction == 0x03) { /* clearDynamicallyDefinedDataIdentifier -> DDDDI 0xF303 */
        return (dyn_did == 0xF303 || dyn_did == 0xF301 || dyn_did == 0xF302);
    }
    return false;
}

void stm32_uds_services_demo_init(void)
{
    /* Initialize UDS Server */
    syn_uds_init(&g_uds_server);

    /* Register DIDs */
    syn_uds_register_did(&g_uds_server, 0xF190, g_vin_data, sizeof(g_vin_data), false);
    syn_uds_register_did(&g_uds_server, 0x0100, g_battery_volts, sizeof(g_battery_volts), true);
    syn_uds_register_did(&g_uds_server, 0x9B00, g_battery_volts, sizeof(g_battery_volts), true);

    /* Register ISO 14229 Service Callbacks */
    syn_uds_register_access_timing(&g_uds_server, on_access_timing, NULL);
    syn_uds_register_secured_data(&g_uds_server, on_secured_data, NULL);
    syn_uds_register_routine_control(&g_uds_server, on_routine_control, NULL);
    syn_uds_register_io_control(&g_uds_server, on_io_control, NULL);
    syn_uds_register_link_control(&g_uds_server, on_link_control, NULL);
    syn_uds_register_roe_handler(&g_uds_server, on_response_on_event, NULL);
    syn_uds_register_scaling_data_handler(&g_uds_server, on_scaling_data, NULL);
    syn_uds_register_periodic_data_handler(&g_uds_server, on_periodic_data, NULL);
    syn_uds_register_dynamic_did_handler(&g_uds_server, on_dynamic_did, NULL);
}

int main(void)
{
    stm32_uds_services_demo_init();
    for (;;) {
        /* Application main loop servicing diagnostic requests */
    }
    return 0;
}
