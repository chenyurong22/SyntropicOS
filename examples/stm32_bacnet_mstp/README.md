# STM32 BACnet MS/TP Smart Thermostat / Sensor Node Example

This example demonstrates how to implement a **BACnet MS/TP (Master-Slave/Token-Passing / ISO 16484-5)** Smart Sensor node on STM32 microcontrollers using SyntropicOS **`syn_bacnet`**.

## Key Features

1. **Cleanroom BACnet MS/TP Implementation**:
   - Zero-malloc MS/TP Frame codec with Preamble `0x55 0xFF`, Header CRC-8, and Data CRC-16.
   - Responds to `Who-Is` requests with `I-Am` unconfirmed APDU frames.
   - Responds to `ReadProperty` requests with `ReadProperty-ACK` confirmed APDU frames.

2. **Static BACnet Object Database**:
   - Device Object (Instance ID `123456`).
   - Analog Input Object 1 (Temperature Sensor, AI:1).
   - Analog Input Object 2 (Humidity Sensor, AI:2).

3. **RS485 Driver Enable (DE) Direction Control**:
   - Hardware RS485 transceiver direction control pin (PA1).

## Hardware Setup
- **Board**: STM32 Nucleo / Discovery (STM32F4 / STM32F1)
- **UART**: USART2 (PA2=TX, PA3=RX, PA1=RS485 DE Driver Enable) @ 38400 or 76800 bps 8N1
- **Transceiver**: MAX485 / SP3485 connected to BACnet MS/TP bus.
