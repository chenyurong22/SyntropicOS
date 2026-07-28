# STM32 Modbus RTU Master Example (RS485 / UART)

This example demonstrates how to implement a non-blocking **Modbus RTU Master / Client** on STM32 microcontrollers using SyntropicOS **`syn_modbus_master`**.

## Key Features

1. **Non-Blocking Transaction Engine**:
   - Zero-malloc, state-machine driven Modbus RTU Master (`syn_modbus_master`).
   - Supports Read Holding Registers (FC 0x03), Read Input Registers (FC 0x04), Write Single Register (FC 0x06), Write Multiple Registers (FC 0x10), and Coil operations.

2. **RS485 Half-Duplex Direction Control**:
   - Configurable RS485 DE (Driver Enable) direction control pin (PA1).
   - Toggles DE HIGH before transmitting request, DE LOW for receiving response.

3. **Automatic Response Timeout & Exception Parsing**:
   - Built-in 500 ms response timeout handler.
   - Parses Modbus RTU exception codes (`0x80 + FC`) and CRC-16 errors automatically.

## Hardware Setup
- **Board**: STM32 Nucleo / Discovery (STM32F4 / STM32F1)
- **UART**: USART2 (115200 8N1)
- **Pins**: PA2 (TX), PA3 (RX), PA1 (RS485 DE Driver Enable)
- **Transceiver**: MAX485 / SP3485 connected to Modbus Slave devices (Slave Address `1`).
