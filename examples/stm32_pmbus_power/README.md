# SyntropicOS PMBus (Power Management Bus 1.2 / 1.3) STM32 HAL Example

Demonstrates non-blocking PMBus digital power supply telemetry reading, voltage margin control, and Linear11 / Linear16 format decoding using STM32 HAL I2C drivers (`HAL_I2C_...`).

## Architecture & Features

- **PMBus Command Encoding**: Encodes PMBus telemetry read commands (`SYN_PMBUS_CMD_READ_VIN`, `SYN_PMBUS_CMD_READ_VOUT`, `SYN_PMBUS_CMD_READ_IOUT`, `SYN_PMBUS_CMD_READ_TEMPERATURE_1`) with SMBus Packet Error Checking (PEC).
- **Linear11 Format Converter**: Decodes 16-bit 5-bit exponent + 11-bit mantissa telemetry fields into floating-point numbers (`syn_pmbus_linear11_to_float`).
- **Linear16 Format Converter**: Decodes 16-bit output voltage fields using `VOUT_MODE` exponent scaling (`syn_pmbus_linear16_to_float`).
- **Status Word Decoding**: Parses `STATUS_WORD` (0x79) flags for Overvoltage (`VOUT_OV`), Overcurrent (`IOUT_OC`), Undervoltage (`VIN_UV`), and Overtemperature (`TEMP_FAULT`).
- **Hardware Interfacing**: Interfaces with standard PMBus digital point-of-load (PoL) regulators and server power supplies (TI TPS544B20, Vicor, MPS MP2975, ADI LTC3880) connected to STM32 I2C SCL/SDA pins.

## Hardware Wiring

```
+------------------+                    +-------------------+
|  STM32 Micro     |                    |  PMBus Power IC   |
|  (e.g., STM32F4) |                    |  (e.g., TPS544B)  |
|                  |                    |                   |
|    I2C1_SCL (PB6) ---------------------> SCL (10k pullup) |
|    I2C1_SDA (PB7) <-------------------> SDA (10k pullup) |
|     GPIO_SMBALERT --------------------> SMBALERT / CONTROL|
|                  |                    |                   |
|              GND ---------------------> GND               |
+------------------+                    +-------------------+
```

## Protocol Specifications

- **I2C Bus Speed**: 100 kHz (Standard SMBus rate) or 400 kHz.
- **Packet Error Checking (PEC)**: CRC-8 modulo-257 polynomial ($x^8 + x^2 + x + 1$).
- **Slave Address**: 7-bit I2C address (default 0x58..0x5F).
