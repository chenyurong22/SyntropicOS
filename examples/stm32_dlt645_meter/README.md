# STM32 DL/T 645 Smart Electricity Meter Communication Example

This example demonstrates how to interface an **STM32 microcontroller** (STM32F4, STM32F1, STM32G0) with a **DL/T 645-2007 / DL/T 645-1997 Smart Electricity Meter** over RS-485 (2400 Baud, 8 Data bits, Even Parity, 1 Stop bit) using SyntropicOS **`syn_dlt645`**.

---

## Hardware Configuration & Wiring

Connect the STM32 to an **RS-485 Transceiver** (MAX485, SP3485, or VP230):

| STM32 Pin | Transceiver Pin | Description |
|---|---|---|
| **PA9 (USART1_TX)** | `DI` (Driver Input) | Transmit data line |
| **PA10 (USART1_RX)** | `RO` (Receiver Output) | Receive data line |
| **PB2** | `DE / ~RE` | RS-485 Transmit / Receive Flow Direction Control |
| **3.3V / 5V** | `VCC` | Power supply |
| **GND** | `GND` | Ground reference |

Connect `A` and `B` differential outputs on the transceiver directly to the **DL/T 645 Smart Meter RS-485 terminals**.

---

## Key Features & Architecture

1. **2400 8E1 UART Configuration**:
   - DL/T 645 meters require **8 Data bits, Even Parity, 1 Stop bit (8E1)**.
   - Configure `huart1.Init.Parity = UART_PARITY_EVEN`.

2. **Single-Byte Interrupt Streaming (`syn_dlt645_decoder_feed`)**:
   - Uses `HAL_UART_Receive_IT(&huart1, &rx_byte, 1)`.
   - Automatically filters `0xFE` preamble wake-up bytes.
   - Decodes frame on `0x16` (EOF) and verifies modulo-256 checksum.

3. **Automatic Byte Offset Offset Handling (`+0x33` / `-0x33`)**:
   - DL/T 645 specifies adding `0x33` to every byte in the data field during transmission to avoid control character collision. `syn_dlt645` handles offset encoding/decoding 100% automatically.

4. **BCD Energy Unpacking**:
   - Decodes 4-byte BCD payload into decimal active energy reading (`kWh` with 0.01 resolution).

---

## Building & Flashing

Add `syn_dlt645.c` to your STM32CubeIDE / Keil / IAR project build sources and compile `main.c`.
