# SyntropicOS SMBus (System Management Bus 1.1 / 2.0 / 3.0) STM32 HAL Example

Demonstrates non-blocking SMBus smart battery telemetry reading (SBS 1.1), Packet Error Checking (PEC) CRC-8 validation, and SMBus Alert Response Address (ARA) interrupt handling using STM32 HAL I2C drivers (`HAL_I2C_...`).

## Architecture & Features

- **SMBus Transaction Protocols**: Encodes and decodes Read Word (`SYN_SMBUS_PROTO_READ_WORD`), Write Word (`SYN_SMBUS_PROTO_WRITE_WORD`), Read Block (`SYN_SMBUS_PROTO_READ_BLOCK`), and Receive Byte protocols (`syn_smbus_encode_packet`, `syn_smbus_decode_packet`).
- **Packet Error Checking (PEC)**: CRC-8 calculation with $x^8 + x^2 + x + 1$ polynomial (`syn_smbus_calc_pec`).
- **Smart Battery System (SBS 1.1)**: Queries Voltage (0x09), Current (0x0A), Relative State of Charge (0x0D), Remaining Capacity (0x0F), and Temperature (0x08).
- **SMBus Alert Response Address (ARA)**: Polls Alert Response Address (`0x0C`) upon `SMBALERT` GPIO interrupt to resolve faulting slave addresses.
- **Hardware Interfacing**: Interfaces with Smart Battery Data ICs (TI BQ20z45, BQ40z50, Maxim MAX17205, LTC4100) connected to STM32 I2C SCL/SDA and SMBALERT pins.

## Hardware Wiring

```
+------------------+                    +-------------------+
|  STM32 Micro     |                    | Smart Battery IC  |
|  (e.g., STM32F4) |                    | (e.g., BQ40z50)   |
|                  |                    |                   |
|    I2C1_SCL (PB6) ---------------------> SMBC (10k pullup)|
|    I2C1_SDA (PB7) <-------------------> SMBD (10k pullup)|
|   GPIO_ALERT(PB5) <-------------------  SMBALERT#         |
|                  |                    |                   |
|              GND ---------------------> GND               |
+------------------+                    +-------------------+
```

## Protocol Specifications

- **Bus Speed**: 100 kHz (Standard SMBus 2.0 rate) or 10 kHz to 100 kHz.
- **PEC Polynomial**: $x^8 + x^2 + x + 1$ (0x07), initial value 0x00.
- **Smart Battery Address**: `0x16` (7-bit I2C address).
- **Alert Response Address**: `0x0C` (7-bit I2C broadcast address).
