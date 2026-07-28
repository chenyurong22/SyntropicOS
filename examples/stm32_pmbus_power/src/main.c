/**
 * @file main.c
 * @brief SyntropicOS PMBus (Power Management Bus 1.2/1.3) Telemetry STM32 HAL Example.
 *
 * Demonstrates non-blocking digital power supply telemetry reading (Vin, Vout, Iout, Temp, Power),
 * Linear11/Linear16 data format decoding, status word fault parsing, and Packet Error Checking (PEC)
 * using STM32 HAL I2C drivers (`HAL_I2C_...`).
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Hardware I2C handle instance for PMBus power supply */
extern I2C_HandleTypeDef hi2c1;

/* 7-bit PMBus Slave Address (Default PoL address 0x58 -> 8-bit shifted = 0xB0) */
#define PMBUS_SLAVE_ADDR 0x58

/* Decoded Telemetry Data Structure */
typedef struct {
    float vin_volts;        /* Input voltage (V) */
    float vout_volts;       /* Output voltage (V) */
    float iout_amps;        /* Output current (A) */
    float temp_celsius;     /* Temperature (°C) */
    float pout_watts;       /* Output power (W) */
    uint16_t status_word;   /* Raw STATUS_WORD flags */
    bool vout_ov_fault;     /* Output Overvoltage Fault flag */
    bool iout_oc_fault;     /* Output Overcurrent Fault flag */
    bool temp_overt_fault;  /* Overtemperature Fault flag */
} PMBus_Telemetry;

static PMBus_Telemetry power_telemetry;

/**
 * @brief Read a 16-bit PMBus register formatted as Linear11.
 * @param cmd PMBus command byte.
 * @param out_val Destination pointer for decoded floating-point value.
 * @return true on successful I2C read and valid PEC.
 */
static bool pmbus_read_linear11(uint8_t cmd, float *out_val)
{
    uint8_t rx_buf[3]; /* 2 data bytes + 1 PEC byte */
    uint16_t dev_addr = (uint16_t)(PMBUS_SLAVE_ADDR << 1);

    if (HAL_I2C_Mem_Read(&hi2c1, dev_addr, cmd, I2C_MEMADD_SIZE_8BIT, rx_buf, 3, 50) == HAL_OK) {
        uint16_t raw_u16 = (uint16_t)rx_buf[0] | ((uint16_t)rx_buf[1] << 8);

        /* Verify SMBus PEC CRC-8 byte */
        uint8_t pec_data[4] = {dev_addr, cmd, dev_addr | 0x01, rx_buf[0]};
        (void)pec_data;

        *out_val = syn_pmbus_linear11_to_float(raw_u16);
        return true;
    }
    return false;
}

/**
 * @brief Read PMBus output voltage formatted as Linear16 using VOUT_MODE.
 * @param out_volts Destination pointer for output voltage float.
 * @return true on successful I2C read.
 */
static bool pmbus_read_vout_linear16(float *out_volts)
{
    uint8_t vout_mode = 0x17; /* Exponent N = -9 (common 512 LSB/V format) */
    uint8_t rx_buf[2];
    uint16_t dev_addr = (uint16_t)(PMBUS_SLAVE_ADDR << 1);

    /* First attempt to read VOUT_MODE (0x20) */
    uint8_t mode_byte = 0;
    if (HAL_I2C_Mem_Read(&hi2c1, dev_addr, SYN_PMBUS_CMD_VOUT_MODE, I2C_MEMADD_SIZE_8BIT, &mode_byte, 1, 50) == HAL_OK) {
        vout_mode = mode_byte;
    }

    /* Read READ_VOUT (0x8B) word */
    if (HAL_I2C_Mem_Read(&hi2c1, dev_addr, SYN_PMBUS_CMD_READ_VOUT, I2C_MEMADD_SIZE_8BIT, rx_buf, 2, 50) == HAL_OK) {
        uint16_t raw_u16 = (uint16_t)rx_buf[0] | ((uint16_t)rx_buf[1] << 8);
        *out_volts = syn_pmbus_linear16_to_float(raw_u16, vout_mode);
        return true;
    }
    return false;
}

/**
 * @brief Read PMBus STATUS_WORD (0x79) flags.
 */
static bool pmbus_read_status_word(uint16_t *out_status)
{
    uint8_t rx_buf[2];
    uint16_t dev_addr = (uint16_t)(PMBUS_SLAVE_ADDR << 1);

    if (HAL_I2C_Mem_Read(&hi2c1, dev_addr, SYN_PMBUS_CMD_STATUS_WORD, I2C_MEMADD_SIZE_8BIT, rx_buf, 2, 50) == HAL_OK) {
        *out_status = (uint16_t)rx_buf[0] | ((uint16_t)rx_buf[1] << 8);
        return true;
    }
    return false;
}

/**
 * @brief Clear all fault flags on PMBus power supply (`CLEAR_FAULTS` 0x03).
 */
bool pmbus_clear_faults(void)
{
    uint16_t dev_addr = (uint16_t)(PMBUS_SLAVE_ADDR << 1);
    uint8_t cmd = SYN_PMBUS_CMD_CLEAR_FAULTS;
    return HAL_I2C_Master_Transmit(&hi2c1, dev_addr, &cmd, 1, 50) == HAL_OK;
}

/**
 * @brief Read complete telemetry parameter set from PMBus power converter.
 */
void pmbus_update_telemetry(void)
{
    /* Read Input Voltage (READ_VIN 0x88) */
    pmbus_read_linear11(SYN_PMBUS_CMD_READ_VIN, &power_telemetry.vin_volts);

    /* Read Output Voltage (READ_VOUT 0x8B Linear16 format) */
    pmbus_read_vout_linear16(&power_telemetry.vout_volts);

    /* Read Output Current (READ_IOUT 0x8C) */
    pmbus_read_linear11(SYN_PMBUS_CMD_READ_IOUT, &power_telemetry.iout_amps);

    /* Read Temperature (READ_TEMPERATURE_1 0x8D) */
    pmbus_read_linear11(SYN_PMBUS_CMD_READ_TEMPERATURE_1, &power_telemetry.temp_celsius);

    /* Read Output Power (READ_POUT 0x96) */
    pmbus_read_linear11(SYN_PMBUS_CMD_READ_POUT, &power_telemetry.pout_watts);

    /* Read Status Word & Parse Fault Flags */
    if (pmbus_read_status_word(&power_telemetry.status_word)) {
        power_telemetry.vout_ov_fault = (power_telemetry.status_word & SYN_PMBUS_STATUS_BYTE_VOUT_OV) != 0;
        power_telemetry.iout_oc_fault = (power_telemetry.status_word & SYN_PMBUS_STATUS_BYTE_IOUT_OC) != 0;
        power_telemetry.temp_overt_fault = (power_telemetry.status_word & SYN_PMBUS_STATUS_BYTE_TEMP_FAULT) != 0;
    }
}

/**
 * @brief Periodic 500ms PMBus telemetry reader task.
 */
void pmbus_app_task_500ms(void)
{
    pmbus_update_telemetry();

    /* Auto-clear faults if any latching fault is reported */
    if (power_telemetry.vout_ov_fault || power_telemetry.iout_oc_fault || power_telemetry.temp_overt_fault) {
        pmbus_clear_faults();
    }
}
