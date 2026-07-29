# STM32 HAL CANopen to NMEA 2000 Marine CAN Gateway Example

This example demonstrates how to implement a non-blocking, zero-malloc gateway on STM32 translating CANopen DS301 Object Dictionary telemetry (battery voltage, current, state of charge, alerts) into NMEA 2000 (IEC 61162-3) marine CAN broadcast messages every 1500ms.

## Features
- **100% Zero Dynamic Allocation (`malloc`)**: Static memory structure for CANopen Object Dictionary entries and NMEA 2000 PGN frame encoders.
- **NMEA 2000 Periodic Broadcast Messages (1500ms Interval)**:
  1. **Battery Status** (`0x19F214F3`): PGN 127508 (0x1F214), Priority 6, SA 0xF3, 1500ms rate.
  2. **DC Detailed Status** (`0x19F212F3`): PGN 127506 (0x1F212), Priority 6, SA 0xF3, 1500ms rate.
  3. **DC Voltage / Current** (`0x19F307F3`): PGN 127751 (0x1F307), Priority 6, SA 0xF3, 1500ms rate.
  4. **Alert Status** (`0x09F007F3`): PGN 126983 (0x0F007), Priority 2, SA 0xF3, 1500ms rate.
- **CANopen DS301 Node**: Serves as CANopen Slave/Master Node (Node-ID 0x20) receiving TPDO/SDO power telemetry updates.
- **Dual Frame Identifier Ingestion**: Handles both 11-bit Standard CAN IDs (CANopen COB-IDs) and 29-bit Extended CAN IDs (NMEA 2000 18-bit J1939 headers) over the same CAN bus.

## Hardware Setup
- Connect STM32 CAN1 (e.g. PB8/PB9 or PA11/PA12) to a 3.3V CAN transceiver (SN65HVD230 / VP230).
- Connect CAN_H and CAN_L to the shared CANopen / NMEA 2000 Marine CAN bus backbone.
- Ensure 120Ω terminating resistors are present at both physical ends of the CAN bus.
