# STM32 CANopen Master / Manager Example (CiA 302 / CiA 301)

This example demonstrates how to implement a non-blocking **CANopen Master / Network Manager** on STM32 microcontrollers using SyntropicOS **`syn_canopen_mgr`**.

## Key Features

1. **Non-Blocking CANopen Master Engine**:
   - Zero-malloc, state-machine driven SDO Client & NMT Master (`SYN_CANOpenManager`).
   - Supports NMT node state control (Start, Stop, Enter Pre-Operational, Reset Node, Reset Communication).
   - Expedited and segmented SDO read/write transactions (`syn_canopen_mgr_sdo_read_init`, `syn_canopen_mgr_sdo_write_init`).

2. **Node Monitoring & Heartbeat Tracking**:
   - Monitors remote node Heartbeat messages (`0x780 + Node-ID`).
   - Handles incoming TPDO process data frames (`0x180 + Node-ID` for TPDO1).

3. **STM32 HAL CAN Integration**:
   - Transmits 11-bit Standard CAN frames via `HAL_CAN_AddTxMessage()`.
   - Ingests incoming 11-bit CAN frames via `HAL_CAN_RxFifo0MsgPendingCallback()`.

## Hardware Setup

- **Board**: STM32 Nucleo / Discovery (STM32F4 / STM32F1 / STM32G4)
- **CAN Transceiver**: SN65HVD230 / VP230 connected to CAN1 (PA11 RX, PA12 TX)
- **Baud Rate**: 125 kbps / 250 kbps / 500 kbps / 1 Mbps
- **Target Slaves**: CANopen Slave Node (e.g., Node-ID `0x20` / 32)
