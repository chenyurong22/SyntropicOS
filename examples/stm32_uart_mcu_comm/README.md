# STM32 MCU-to-MCU Communication Example (UART Interrupt, No DMA)

This example demonstrates how to implement point-to-point, bidirectional MCU-to-MCU communication over UART TTL / RS232 using SyntropicOS **`syn_cobs`** (Consistent Overhead Byte Stuffing packet framing) and **`syn_router`** (addressed message dispatch with optional ACK/retransmissions).

## Hardware Setup
- **MCU 1 (Master)**: Node ID `0x01`
- **MCU 2 (Slave)**: Node ID `0x02`
- **Interface**: USART2 (TX = PA2, RX = PA3) running at 115200 8N1.
- Connect **MCU 1 TX (PA2)** $\rightarrow$ **MCU 2 RX (PA3)** and **MCU 1 RX (PA3)** $\leftarrow$ **MCU 2 TX (PA2)** with a shared Ground (GND).

## Key Concepts

1. **Single-Byte Interrupt Reception (No DMA)**:
   - Uses standard STM32 `HAL_UART_Receive_IT(&huart2, &rx_byte, 1)`.
   - Each byte received in `HAL_UART_RxCpltCallback()` is passed to `syn_cobs_decoder_feed(&cobs_dec, rx_byte)`.

2. **Self-Healing COBS Framing**:
   - Packets are delimited by `0x00` bytes.
   - If line noise or dropped bytes occur, the state machine automatically resynchronizes on the next `0x00` delimiter without lock-up.

3. **Addressed Message Routing (`syn_router`)**:
   - Packets contain Source Node ID, Destination Node ID, and Message Type.
   - Handlers are registered for specific message types (e.g. `MSG_TYPE_SET_LED = 0x10`).
   - Supports optional reliable delivery (`syn_router_send(..., true)`).

## Building
Set `#define IS_MASTER_MCU 1` to compile the Master MCU firmware, or `#define IS_MASTER_MCU 0` to compile the Slave MCU firmware.
