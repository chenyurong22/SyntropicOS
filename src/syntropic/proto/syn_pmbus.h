/**
 * @file syn_pmbus.h
 * @brief PMBus (Power Management Bus 1.2 / 1.3) Protocol Engine & Linear Format Converters.
 *
 * Provides PMBus standard command definitions, telemetry status decoding, and
 * Linear11 / Linear16 numeric data converters.
 * @ingroup syn_proto
 */

#ifndef SYN_PMBUS_H
#define SYN_PMBUS_H

#include "../common/syn_defs.h"
#include "syn_smbus.h"

#include <stdbool.h>
#include <stdint.h>

#if defined(SYN_USE_PMBUS) && SYN_USE_PMBUS

#ifdef __cplusplus
extern "C" {
#endif

/* ── PMBus Command Codes ────────────────────────────────────────────────── */

#define SYN_PMBUS_CMD_PAGE 0x00                 /**< Select page/channel */
#define SYN_PMBUS_CMD_OPERATION 0x01            /**< On/off and margin control */
#define SYN_PMBUS_CMD_ON_OFF_CONFIG 0x02        /**< On/off switch configuration */
#define SYN_PMBUS_CMD_CLEAR_FAULTS 0x03         /**< Clear fault flags */
#define SYN_PMBUS_CMD_PHASE 0x04                /**< Phase selection */
#define SYN_PMBUS_CMD_WRITE_PROTECT 0x10        /**< Write protect configuration */
#define SYN_PMBUS_CMD_STORE_DEFAULT_ALL 0x11    /**< Store default settings to NVM */
#define SYN_PMBUS_CMD_RESTORE_DEFAULT_ALL 0x12  /**< Restore default settings from NVM */
#define SYN_PMBUS_CMD_CAPABILITY 0x19           /**< Bus speed and alert capabilities */
#define SYN_PMBUS_CMD_VOUT_MODE 0x20            /**< Output voltage mode & exponent */
#define SYN_PMBUS_CMD_VOUT_COMMAND 0x21         /**< Output voltage setpoint */
#define SYN_PMBUS_CMD_VOUT_MAX 0x24             /**< Maximum output voltage limit */
#define SYN_PMBUS_CMD_VOUT_MARGIN_HIGH 0x25     /**< High margin voltage setting */
#define SYN_PMBUS_CMD_VOUT_MARGIN_LOW 0x26      /**< Low margin voltage setting */
#define SYN_PMBUS_CMD_VOUT_TRANSITION_RATE 0x27 /**< Voltage slew rate */
#define SYN_PMBUS_CMD_VOUT_DROOP 0x28           /**< Voltage droop rate */
#define SYN_PMBUS_CMD_STATUS_BYTE 0x78          /**< Summary status byte */
#define SYN_PMBUS_CMD_STATUS_WORD 0x79          /**< Summary status word */
#define SYN_PMBUS_CMD_STATUS_VOUT 0x7A          /**< Output voltage status flags */
#define SYN_PMBUS_CMD_STATUS_IOUT 0x7B          /**< Output current status flags */
#define SYN_PMBUS_CMD_STATUS_INPUT 0x7C         /**< Input status flags */
#define SYN_PMBUS_CMD_STATUS_TEMPERATURE 0x7D   /**< Temperature status flags */
#define SYN_PMBUS_CMD_STATUS_CBUFFER 0x7E       /**< Communication/logic status */
#define SYN_PMBUS_CMD_STATUS_OTHER 0x7F         /**< Other status flags */
#define SYN_PMBUS_CMD_STATUS_MFR_SPECIFIC 0x80  /**< Manufacturer status flags */
#define SYN_PMBUS_CMD_STATUS_FANS_1_2 0x81      /**< Fan 1 & 2 status flags */
#define SYN_PMBUS_CMD_READ_VIN 0x88             /**< Read input voltage */
#define SYN_PMBUS_CMD_READ_IIN 0x89             /**< Read input current */
#define SYN_PMBUS_CMD_READ_VOUT 0x8B            /**< Read output voltage */
#define SYN_PMBUS_CMD_READ_IOUT 0x8C            /**< Read output current */
#define SYN_PMBUS_CMD_READ_TEMPERATURE_1 0x8D   /**< Read primary temperature */
#define SYN_PMBUS_CMD_READ_TEMPERATURE_2 0x8E   /**< Read secondary temperature */
#define SYN_PMBUS_CMD_READ_FAN_SPEED_1 0x90     /**< Read fan speed 1 (RPM) */
#define SYN_PMBUS_CMD_READ_DUTY_CYCLE 0x94      /**< Read PWM duty cycle */
#define SYN_PMBUS_CMD_READ_FREQUENCY 0x95       /**< Read switching frequency */
#define SYN_PMBUS_CMD_READ_POUT 0x96            /**< Read output power */
#define SYN_PMBUS_CMD_READ_PIN 0x97             /**< Read input power */
#define SYN_PMBUS_CMD_PMBUS_REVISION 0x98       /**< PMBus spec version */

/* ── PMBus Status Bitmask Definitions ───────────────────────────────────── */

#define SYN_PMBUS_STATUS_BYTE_BUSY (1u << 7)
#define SYN_PMBUS_STATUS_BYTE_OFF (1u << 6)
#define SYN_PMBUS_STATUS_BYTE_VOUT_OV (1u << 5)
#define SYN_PMBUS_STATUS_BYTE_IOUT_OC (1u << 4)
#define SYN_PMBUS_STATUS_BYTE_VIN_UV (1u << 3)
#define SYN_PMBUS_STATUS_BYTE_TEMP_FAULT (1u << 2)
#define SYN_PMBUS_STATUS_BYTE_CBUF_FAULT (1u << 1)
#define SYN_PMBUS_STATUS_BYTE_NONE_OF_ABOVE (1u << 0)

/* ── API Function Declarations ───────────────────────────────────────────── */

/**
 * @brief Convert PMBus 16-bit Linear11 format (5-bit exponent + 11-bit mantissa) to float.
 *
 * @param raw 16-bit raw Linear11 word.
 * @return Decoded floating-point value.
 */
float syn_pmbus_linear11_to_float(uint16_t raw);

/**
 * @brief Convert floating-point value to PMBus 16-bit Linear11 format.
 *
 * @param val Floating-point value.
 * @return Encoded 16-bit Linear11 word.
 */
uint16_t syn_pmbus_float_to_linear11(float val);

/**
 * @brief Convert PMBus 16-bit Linear16 format (unsigned 16-bit mantissa + VOUT_MODE exponent) to
 * float.
 *
 * @param raw       16-bit raw mantissa word.
 * @param vout_mode VOUT_MODE byte (or raw 5-bit signed exponent N).
 * @return Decoded floating-point output voltage.
 */
float syn_pmbus_linear16_to_float(uint16_t raw, uint8_t vout_mode);

/**
 * @brief Convert floating-point output voltage to PMBus 16-bit Linear16 format.
 *
 * @param val       Floating-point voltage value.
 * @param vout_mode VOUT_MODE byte (or raw 5-bit signed exponent N).
 * @return Encoded 16-bit Linear16 raw mantissa word.
 */
uint16_t syn_pmbus_float_to_linear16(float val, uint8_t vout_mode);

/**
 * @brief Encode a PMBus telemetry read request into an SMBus packet.
 *
 * @param pkt        Output SMBus packet.
 * @param slave_addr 7-bit PMBus slave address.
 * @param cmd        PMBus command code (e.g., SYN_PMBUS_CMD_READ_VIN).
 * @param use_pec    Enable Packet Error Checking (PEC).
 */
void syn_pmbus_encode_read_cmd(SYN_SMBUS_Packet *pkt, uint8_t slave_addr, uint8_t cmd,
                               bool use_pec);

#ifdef __cplusplus
}
#endif

#endif /* SYN_USE_PMBUS */
#endif /* SYN_PMBUS_H */
