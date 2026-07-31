# STM32 HAL CCP over CAN Example

This example demonstrates how to integrate the SyntropicOS ASAM CCP v2.1 (CAN Calibration Protocol) slave stack (`syn_ccp.h`) with STM32 HAL CAN drivers (`HAL_CAN_...`).

## Features
- **Zero Dynamic Memory Allocation (`malloc`)**: Static allocation for CCP slave instance and DAQ/ODT list structures.
- **CCP v2.1 Slave Engine**:
  - Connect (`0x01`), Exchange ID (`0x02`), Set MTA (`0x08`), Upload (`0x04`), Download (`0x03`), Short Upload (`0x0F`), Disconnect (`0x17`).
  - DAQ List streaming: Set DAQ Size (`0x15`), Build Packet (`0x10`), Start/Stop DAQ (`0x11` / `0x06`).
- **STM32 HAL CAN Integration**:
  - Receives CRO packets on CAN ID `0x600` via `HAL_CAN_RxFifo0MsgPendingCallback`.
  - Transmits DTO/CRM responses on CAN ID `0x601` via `HAL_CAN_AddTxMessage`.

## Hardware Setup
- Connect STM32 CAN1 (e.g. PB8/PB9 or PA11/PA12) to a 3.3V CAN transceiver (SN65HVD230 / VP230).
- Connect CAN_H and CAN_L to a calibration tool master (e.g. Vector CANape, INCA, or Python ccp library).
- Ensure 120Ω termination resistors are present at both ends of the CAN bus.
