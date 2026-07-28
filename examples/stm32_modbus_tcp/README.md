# SyntropicOS Modbus TCP Master & Slave STM32 HAL Example

Demonstrates simultaneous non-blocking Modbus TCP Client (Master) and Server (Slave) protocol engines running on a single STM32 MCU using LwIP / Ethernet hardware (`HAL_ETH_...`).

## Architecture & Features

- **Dual MBAP Protocol Stack**: Single STM32 project acting as both Modbus TCP Server (port 502 listener for SCADA/PLC clients) and Modbus TCP Client (master polling field devices).
- **MBAP Header Parsing**: Encapsulates PDU frames with 7-byte MBAP Header (`Transaction ID`, `Protocol ID = 0x0000`, `Length`, `Unit ID`).
- **Modbus Functions Supported**:
  - Read Holding Registers (FC 0x03)
  - Read Input Registers (FC 0x04)
  - Write Single Register (FC 0x06)
  - Write Multiple Registers (FC 0x10)
- **Zero-Heap Networking**: Operates via socket abstractions (`syn_port_sock_*`) or LwIP netconn sockets without `malloc()` dynamic memory allocation.

## Hardware Wiring & Topology

```
+-----------------------------------------------------------+
|                  STM32 Ethernet Microcontroller           |
|                  (e.g., STM32F407 / STM32F767)            |
|                                                           |
|  +-------------------------+   +-----------------------+  |
|  | Modbus TCP Server (502) |   | Modbus TCP Client     |  |
|  |  (Slave - Reg Map 0..31)|   | (Master - Query Loop) |  |
|  +------------+------------+   +-----------+-----------+  |
|               |                            |              |
|               +-------------+--------------+              |
|                             |                             |
|                        LwIP / Socket                      |
|                      Ethernet PHY (LAN8720)               |
+-----------------------------+-----------------------------+
                              | RJ45 Ethernet Cable
                              v
                  Industrial Network / Router
```

## Protocol Specifications

- **Default TCP Port**: 502 (Modbus TCP standard port).
- **MBAP Header Structure**:
  - `Bytes 0..1`: Transaction Identifier (mirrored in response).
  - `Bytes 2..3`: Protocol Identifier (0x0000 = Modbus TCP).
  - `Bytes 4..5`: Length (number of following bytes).
  - `Byte 6`: Unit Identifier (Slave Address, default 1 or 0xFF).
