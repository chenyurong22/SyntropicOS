# SyntropicOS EtherCAT (IEEE 802.3 EtherType 0x88A4) STM32 HAL Example

Demonstrates non-blocking bare-metal EtherCAT Slave node processing, EtherCAT State Machine (ESM) transitions, CAN Application Protocol over EtherCAT (CoE) Object Dictionary binding, and raw L2 Ethernet frame processing using STM32 HAL Ethernet drivers (`HAL_ETH_...`).

## Architecture & Features

- **L2 Raw Ethernet Processing**: Processes EtherCAT datagram frames (EtherType `0x88A4`) directly at Layer 2 without TCP/IP overhead.
- **EtherCAT State Machine (ESM)**: Manages node state transitions:
  - `INIT`: Hardware initialization and Station Address configuration.
  - `PREOP`: CoE Mailbox SDO parameter configuration.
  - `SAFEOP`: Cyclic TxPDO telemetry active (RxPDO outputs muted).
  - `OP`: Fully operational cyclic RxPDO / TxPDO servo drive control loop.
- **CoE (CANoverEtherCAT) Mailbox & SDO**: Expedited SDO read/write requests (`syn_ecat_coe_encode_sdo_download`, `syn_ecat_coe_encode_sdo_upload`) bound to CANopen Object Dictionary (`SYN_CANOpenNode`).
- **Datagram & WKC Accounting**: Validates Logical Read/Write (`LRW`), Configured Address Read/Write (`FPRD`/`FPWR`), and Working Counter (`wkc`) values.
- **Hardware Interfacing**: Interfaces with STM32 Ethernet MAC and LAN8720 / DP83848 PHY, or dedicated EtherCAT Slave Controller ICs (Microchip LAN9252 / Beckhoff ET1100).

## Hardware Wiring

```
+------------------+                    +-------------------+
|  STM32 Micro     |                    |  Ethernet PHY /   |
|  (e.g., STM32F4) |                    |  LAN9252 ESC      |
|                  |                    |                   |
|   RMII_TXD0/1    ---------------------> TXD0/1            |
|   RMII_TX_EN     ---------------------> TX_EN             |
|   RMII_RXD0/1    <--------------------- RXD0/1            |
|   RMII_CRS_DV    <--------------------- CRS_DV            |
|   RMII_REF_CLK   <--------------------- 50MHz REF_CLK     |
|                  |                    |                   |
|              GND ---------------------> GND               |
+------------------+                    +-------------------+
```

## Protocol Specifications

- **EtherType**: `0x88A4` (IEEE 802.3 Raw Ethernet frame).
- **ESM States**: INIT (0x01), PREOP (0x02), SAFEOP (0x04), OP (0x08).
- **CoE Mailbox**: SDO Expedited Transfer (Index 0x1000..0x9FFF).
- **Cycle Time**: Sub-millisecond to 1ms cyclic exchange rate.
