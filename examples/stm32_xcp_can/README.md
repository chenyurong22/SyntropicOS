# STM32 HAL XCP over CAN Example

This example demonstrates how to integrate the SyntropicOS ASAM XCP v1.x (Universal Measurement and Calibration Protocol) slave stack (`syn_xcp.h`) with STM32 HAL CAN drivers (`HAL_CAN_...`).

## Features
- **Zero Dynamic Memory Allocation (`malloc`)**: Static memory allocation for XCP slave instance and DAQ/ODT list structures.
- **XCP v1.x Slave Protocol Engine**:
  - Connect (`0xFF`), Disconnect (`0xFE`), Get Status (`0xFD`), Synch (`0xFC`), Set MTA (`0xF6`), Upload (`0xF5`), Short Upload (`0xF4`), Download (`0xF0`).
  - DAQ List streaming: Set DAQ Pointer (`0xE2`), Write DAQ (`0xE1`), Set DAQ List Mode (`0xE0`), Start/Stop DAQ (`0xDE` / `0xDD`).
- **STM32 HAL CAN Integration**:
  - Receives CTO command packets on CAN ID `0x555` via `HAL_CAN_RxFifo0MsgPendingCallback`.
  - Transmits DTO responses and DAQ telemetry packets on CAN ID `0x556` via `HAL_CAN_AddTxMessage`.

## Hardware Setup
- Connect STM32 CAN1 (e.g. PB8/PB9 or PA11/PA12) to a 3.3V CAN transceiver (SN65HVD230 / VP230).
- Connect CAN_H and CAN_L to an XCP master calibration tool (e.g. Vector CANape, ETAS INCA, or pyxcp library).
- Ensure 120Ω termination resistors are present at both ends of the CAN bus.
