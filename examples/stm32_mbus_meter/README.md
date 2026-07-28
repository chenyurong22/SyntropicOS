# SyntropicOS M-Bus (Meter-Bus EN 13757-2 / EN 13757-3) STM32 HAL Example

Demonstrates non-blocking M-Bus utility meter communication (water, gas, heat, electricity meters) using STM32 HAL USART hardware drivers (`HAL_UART_...`).

## Architecture & Features

- **M-Bus Master Query Engine**: Formats Link Reset (`SND_NKE`, C=0x40) and Data Request (`REQ_UD2`, C=0x5B) Short Frames (`syn_mbus_encode_short`).
- **M-Bus Frame Types**: Supports Single Character ACK (`0xE5`), Short Frames (5 bytes), Control Frames (9 bytes), and Variable Long Frames ($N \ge 9$ bytes, C=0x08 `RSP_UD`).
- **Streaming Rx Decoder**: Non-blocking byte-by-byte UART stream ingestion (`syn_mbus_decoder_feed`) with modulo-256 checksum verification.
- **Hardware Interfacing**: Interfaces with standard M-Bus level converter ICs (e.g. TSS721A, NVT2002, or discrete transistor level shifter) connected to STM32 USART TX/RX pins.

## Hardware Wiring

```
+------------------+                    +-------------------+
|  STM32 Micro     |                    |  M-Bus Master IC  |
|  (e.g., STM32F4) |                    |  (e.g., TSS721A)  |
|                  |                    |                   |
|   USART2_TX (PA2) ---------------------> TXD               |
|   USART2_RX (PA3) <--------------------- RXD               |
|                  |                    |                   |
|              GND ---------------------> GND               |
+------------------+                    |    M-BUS LINE     |
                                        |   24V-36V Bus     |
                                        |  <=============>  |
                                        +-------------------+
```

## Protocol Specifications

- **Baud Rate**: 2,400 baud (standard M-Bus primary communication rate) or 300 / 9,600 baud.
- **UART Frame**: 8 Data Bits, Even Parity, 1 Stop Bit (8E1) per EN 13757-2 standard.
- **Address Range**: Primary Address 1..250 (0xFE broadcast with reply, 0xFF broadcast without reply).
