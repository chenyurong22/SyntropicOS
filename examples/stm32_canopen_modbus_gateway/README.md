# STM32 CANopen DS301 to Modbus RTU/TCP Gateway Example

This example demonstrates multi-protocol fieldbus translation on STM32 microcontrollers between CANopen Object Dictionary (DS301/CiA 418) telemetry and Modbus RTU/TCP holding registers (FC03/FC06/FC16).

## Overview

The gateway node hosts a CANopen DS301 Object Dictionary engine and maps live telemetry variables (voltage, current, temperature, state of charge, alerts) to Modbus holding registers (40001..40010):

| Modbus Register | Address | Type | CANopen OD Index | Parameter Description |
|-----------------|---------|------|------------------|-----------------------|
| 40001           | `0x0000` | U16  | `0x6401:01`      | Battery Voltage (0.01V) |
| 40002           | `0x0001` | I16  | `0x6401:02`      | Battery Current (0.1A)  |
| 40003           | `0x0002` | U16  | `0x6401:03`      | Temperature (0.1 K)    |
| 40004           | `0x0003` | U16  | `0x6402:01`      | State of Charge (%)     |
| 40005           | `0x0004` | U16  | `0x6402:02`      | State of Health (%)     |
| 40006           | `0x0005` | U16  | `0x6402:03`      | Time to Go (minutes)    |
| 40007           | `0x0006` | U16  | `0x6000:02`      | Alert Severity          |
| 40008..40009    | `0x0007` | U32  | `0x6000:01`      | Alert Bitmask Flags     |
| 40010           | `0x0009` | U16  | `0x1001:00`      | Error Register          |

## Features

- **Non-blocking & Zero-malloc**: Fixed stack allocations, microsecond execution.
- **Bi-directional Mapping**: Modbus write queries (FC06/FC16) update the corresponding Object Dictionary entry in real time.
- **Protocol Neutrality**: Generic architectural template suitable for any BMS or industrial drive configuration.
