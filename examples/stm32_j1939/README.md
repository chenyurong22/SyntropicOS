# STM32 HAL SAE J1939 CAN Protocol Example

This example demonstrates how to integrate SyntropicOS SAE J1939 heavy-duty vehicle CAN protocol stack (`syn_j1939.h`) with STM32 HAL drivers (`HAL_CAN_...`).

## Features
- **100% Zero Dynamic Allocation (`malloc`)**: Uses static memory buffers for all CAN frames, address claiming state machine, BAM transport protocol, and DM1 active DTC logs.
- **SAE J1939-81 Address Claiming**: Automatic 64-bit NAME structure encoding and address contention resolution.
- **J1939 29-Bit Extended CAN Header Parsing/Packing**: Priority, PDU Format (PF), PDU Specific (PS/DA/GE), and Parameter Group Number (PGN) computation.
- **J1939-21 Transport Protocol (BAM)**: Broadcast Announce Message multi-packet frame transmission and reassembly for payloads larger than 8 bytes.
- **J1939-73 Active Diagnostic Trouble Codes (DM1)**: Automatic SPN/FMI/Occurrence Count encoding and periodic broadcast transmission.
- **Seamless STM32 HAL CAN Integration**: Interrupt-driven frame ingestion via `HAL_CAN_RxFifo0MsgPendingCallback`.

## Hardware Setup
- Connect STM32 CAN1 (e.g. PB8/PB9 or PA11/PA12) to an external 3.3V CAN transceiver (SN65HVD230 or VP230).
- Connect CAN High (CAN_H) and CAN Low (CAN_L) to a J1939 bus (or CAN analyzer / vector tool) terminated with 120Ω resistors at each bus end.
