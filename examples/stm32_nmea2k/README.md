# STM32 HAL NMEA 2000 (N2K) Marine CAN Protocol Example

This example demonstrates how to integrate SyntropicOS NMEA 2000 marine CAN protocol stack (`syn_n2k.h`) with STM32 HAL drivers (`HAL_CAN_...`).

## Features
- **100% Zero Dynamic Allocation (`malloc`)**: Uses static memory buffers for all N2K marine PGN encoders/decoders and Fast-Packet reassembly sessions.
- **Standard Marine Parameter Group Numbers (PGNs)**:
  - PGN 129025: Position, Rapid Update (Latitude / Longitude in 1e-7 degrees).
  - PGN 129026: COG & SOG, Rapid Update (Course Over Ground, Speed Over Ground).
  - PGN 127250: Vessel Heading (Magnetic / True Heading, Deviation, Variation).
  - PGN 127508: Battery Status (Voltage in 0.01V, Current in 0.1A, Temperature).
  - PGN 130310: Environmental Parameters (Water Temp, Air Temp, Atmospheric Pressure).
- **Fast-Packet Protocol Reassembly**: Handles multi-frame NMEA 2000 payloads up to 223 bytes without dynamic heap memory (`SYN_N2K_FastPacketRx`).
- **Seamless STM32 HAL CAN Integration**: Interrupt-driven frame ingestion via `HAL_CAN_RxFifo0MsgPendingCallback`.

## Hardware Setup
- Connect STM32 CAN1 (e.g. PB8/PB9 or PA11/PA12) to a 3.3V CAN transceiver (SN65HVD230 / VP230).
- Wire the transceiver to an NMEA 2000 backbone (Micro-C M12 5-pin connector: NET-H, NET-L, NET-S, V+, V-).
- Ensure the NMEA 2000 network backbone has 120Ω terminating resistors installed at both ends.
