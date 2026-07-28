/**
 * @file main.c
 * @brief SyntropicOS SMBus 2.0 Smart Battery System (SBS 1.1) STM32 HAL Example.
 *
 * Demonstrates non-blocking SMBus transaction protocol encoding (`syn_smbus_encode_packet`),
 * Packet Error Checking (PEC) CRC-8 validation (`syn_smbus_calc_pec`), Smart Battery telemetry
 * reading (Voltage, Current, State of Charge, Temperature), and SMBus Alert Response Address (ARA 0x0C)
 * interrupt resolution using STM32 HAL I2C drivers (`HAL_I2C_...`).
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Hardware I2C handle instance for SMBus Smart Battery */
extern I2C_HandleTypeDef hi2c1;

/* Pin definitions for SMBALERT# interrupt line */
#define SMBALERT_PORT GPIOB
#define SMBALERT_PIN  GPIO_PIN_5

/* Smart Battery Data Specification (SBS 1.1) Command Codes */
#define SBS_CMD_TEMPERATURE            0x08 /* Temperature (0.1K units) */
#define SBS_CMD_VOLTAGE                0x09 /* Battery Voltage (mV) */
#define SBS_CMD_CURRENT                0x0A /* Battery Current (mA, signed) */
#define SBS_CMD_RELATIVE_STATE_OF_CHARGE 0x0D /* State of Charge (%) */
#define SBS_CMD_REMAINING_CAPACITY     0x0F /* Remaining Capacity (mAh) */
#define SBS_CMD_FULL_CHARGE_CAPACITY   0x10 /* Full Charge Capacity (mAh) */
#define SBS_CMD_CYCLE_COUNT            0x17 /* Cycle Count */
#define SBS_CMD_BATTERY_STATUS         0x16 /* Battery Status Word */

/* Decoded Smart Battery Telemetry Structure */
typedef struct {
    uint16_t voltage_mv;        /* Battery Voltage in millivolts */
    int16_t  current_ma;        /* Battery Current in milliamps (positive = charging, negative = discharging) */
    uint8_t  relative_soc_pct;  /* State of Charge percentage (0..100%) */
    float    temp_celsius;      /* Battery Temperature in °C */
    uint16_t remaining_mah;     /* Remaining Capacity in mAh */
    uint16_t full_capacity_mah; /* Full Charge Capacity in mAh */
    uint16_t cycle_count;       /* Battery Charge Cycle Count */
    uint16_t battery_status;    /* Status word (overtemp, alarm flags) */
    bool     alert_pending;     /* Set true when SMBALERT line triggers */
} SmartBattery_Telemetry;

static SmartBattery_Telemetry battery_data;

/**
 * @brief Execute an SMBus Read Word transaction with Packet Error Checking (PEC).
 * @param slave_addr 7-bit SMBus slave address (e.g., 0x16).
 * @param cmd        SMBus command code (e.g., 0x09).
 * @param out_val    Destination pointer for 16-bit word.
 * @return true on successful transaction and valid PEC.
 */
static bool smbus_read_word_pec(uint8_t slave_addr, uint8_t cmd, uint16_t *out_val)
{
    uint8_t rx_buf[3]; /* 2 data bytes + 1 PEC byte */
    uint16_t dev_addr = (uint16_t)(slave_addr << 1);

    if (HAL_I2C_Mem_Read(&hi2c1, dev_addr, cmd, I2C_MEMADD_SIZE_8BIT, rx_buf, 3, 50) == HAL_OK) {
        /* Compute expected PEC CRC-8 byte */
        uint8_t pec_bytes[4] = {dev_addr, cmd, (uint8_t)(dev_addr | 0x01), rx_buf[0]};
        uint8_t calc_pec = syn_smbus_calc_pec(0x00, pec_bytes, 4);
        calc_pec = syn_smbus_calc_pec(calc_pec, &rx_buf[1], 1);

        if (calc_pec == rx_buf[2]) {
            *out_val = (uint16_t)rx_buf[0] | ((uint16_t)rx_buf[1] << 8);
            return true;
        }
    }
    return false;
}

/**
 * @brief Execute an SMBus Alert Response Address (ARA 0x0C) query to identify alerting device.
 * @param out_alerting_addr Pointer to receive 7-bit address of device that pulled SMBALERT low.
 * @return true if an alerting device responded.
 */
bool smbus_query_alert_response_address(uint8_t *out_alerting_addr)
{
    uint8_t ara_addr = (uint8_t)(SYN_SMBUS_ADDR_ALERT_RESPONSE << 1);
    uint8_t rx_byte = 0;

    if (HAL_I2C_Master_Receive(&hi2c1, ara_addr | 0x01, &rx_byte, 1, 50) == HAL_OK) {
        if (out_alerting_addr != NULL) {
            *out_alerting_addr = (uint8_t)(rx_byte >> 1); /* Extract 7-bit address */
        }
        return true;
    }
    return false;
}

/**
 * @brief STM32 HAL GPIO External Interrupt Callback (SMBALERT pin).
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == SMBALERT_PIN) {
        battery_data.alert_pending = true;
    }
}

/**
 * @brief Poll and update Smart Battery telemetry parameters.
 */
void smbus_battery_update(void)
{
    uint16_t val16 = 0;

    /* Read Voltage (0x09) */
    if (smbus_read_word_pec(SYN_SMBUS_ADDR_SMART_BATTERY, SBS_CMD_VOLTAGE, &val16)) {
        battery_data.voltage_mv = val16;
    }

    /* Read Current (0x0A) */
    if (smbus_read_word_pec(SYN_SMBUS_ADDR_SMART_BATTERY, SBS_CMD_CURRENT, &val16)) {
        battery_data.current_ma = (int16_t)val16;
    }

    /* Read Relative State of Charge (0x0D) */
    if (smbus_read_word_pec(SYN_SMBUS_ADDR_SMART_BATTERY, SBS_CMD_RELATIVE_STATE_OF_CHARGE, &val16)) {
        battery_data.relative_soc_pct = (uint8_t)val16;
    }

    /* Read Temperature (0x08) -> convert 0.1 Kelvin to °C */
    if (smbus_read_word_pec(SYN_SMBUS_ADDR_SMART_BATTERY, SBS_CMD_TEMPERATURE, &val16)) {
        battery_data.temp_celsius = ((float)val16 * 0.1f) - 273.15f;
    }

    /* Read Remaining Capacity (0x0F) */
    if (smbus_read_word_pec(SYN_SMBUS_ADDR_SMART_BATTERY, SBS_CMD_REMAINING_CAPACITY, &val16)) {
        battery_data.remaining_mah = val16;
    }

    /* Read Full Charge Capacity (0x10) */
    if (smbus_read_word_pec(SYN_SMBUS_ADDR_SMART_BATTERY, SBS_CMD_FULL_CHARGE_CAPACITY, &val16)) {
        battery_data.full_capacity_mah = val16;
    }

    /* Read Cycle Count (0x17) */
    if (smbus_read_word_pec(SYN_SMBUS_ADDR_SMART_BATTERY, SBS_CMD_CYCLE_COUNT, &val16)) {
        battery_data.cycle_count = val16;
    }
}

/**
 * @brief Periodic 1000ms SMBus Smart Battery polling task.
 */
void smbus_battery_task_1000ms(void)
{
    /* Handle SMBALERT interrupt if line was pulled low */
    if (battery_data.alert_pending) {
        uint8_t alerting_addr = 0;
        if (smbus_query_alert_response_address(&alerting_addr)) {
            /* Successfully cleared alert from device at alerting_addr */
            battery_data.alert_pending = false;
        }
    }

    /* Update Smart Battery telemetry values */
    smbus_battery_update();
}
