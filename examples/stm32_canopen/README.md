# STM32 HAL CANopen DS301 Slave Example

This example demonstrates how to integrate SyntropicOS CANopen DS301 slave protocol stack (`syn_canopen.h`) with STM32 HAL CAN drivers (`HAL_CAN_...`).

## Features
- **100% Zero Dynamic Allocation (`malloc`)**: Static memory allocation for Object Dictionary (OD) entries, SDO sessions, and TPDO/RPDO mappings.
- **CANopen DS301 Compliant Slave Engine**:
  - Object Dictionary (OD): Standard index entries (0x1000 Device Type, 0x1001 Error Register, 0x1017 Heartbeat Period, 0x6000 Digital Inputs, 0x6200 Digital Outputs, 0x6401 Analog Inputs).
  - SDO Server: Expedited and Segmented SDO reads/writes over COB-ID `0x600 + NodeID` (Rx) and `0x580 + NodeID` (Tx).
  - RPDO / TPDO Engine: Automatic Process Data Object mapping and manual/event trigger support (`syn_canopen_tpdo_trigger`).
  - NMT State Machine: Bootup (0x700 + NodeID), Pre-Operational, Operational, and Stopped states.
  - Heartbeat Producer: Periodic heartbeat broadcast (COB-ID `0x700 + NodeID`).
  - Emergency (EMCY) Alarms: Automatic emergency code reporting (COB-ID `0x080 + NodeID`).
- **STM32 HAL CAN Integration**: Interrupt-driven frame ingestion via `HAL_CAN_RxFifo0MsgPendingCallback`.


## Hardware Setup
- Connect STM32 CAN1 (e.g. PB8/PB9 or PA11/PA12) to a 3.3V CAN transceiver (SN65HVD230 / VP230).
- Connect CAN_H and CAN_L to a CANopen master network (e.g. PLC, CoDeSys master, Kvaser, PEAK PCAN, or Vector CANoe).
- Ensure 120Ω termination resistors are present at both ends of the CAN bus.
