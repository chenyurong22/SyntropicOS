# SyntropicOS LIN Bus (Local Interconnect Network) STM32 HAL Example

Demonstrates non-blocking LIN 2.0 / 2.1 automotive single-wire bus master and slave communication using STM32 HAL USART hardware drivers (`HAL_UART_...`).

## Architecture & Features

- **LIN Master Scheduling**: Uses `SYN_LIN_Master` to execute schedule tables with precise slot delays (Publish and Subscribe slots).
- **LIN Frame Generation**: Automatic Break Field generation (`HAL_UART_SendBreak`), Sync Byte (`0x55`), and Protected Identifier (PID) parity calculation ($P_0, P_1$).
- **LIN Checksum Engine**: Supports Classic (LIN 1.3 data-only) and Enhanced (LIN 2.0+ PID + data) checksum calculation (`syn_lin_calc_checksum`).
- **LIN Slave State Machine**: Zero-malloc byte stream parser (`syn_lin_slave_process_byte`) with ID filtering and automatic publish/subscribe response handling.
- **Hardware Interfacing**: Interfaces with standard LIN transceivers (TJA1020, TJA1021, MCP2003, NCV7321) connected to STM32 USART TX/RX pins.

## Hardware Wiring

```
+------------------+                    +-------------------+
|  STM32 Micro     |                    |  LIN Transceiver  |
|  (e.g., STM32F4) |                    |  (e.g., TJA1021)  |
|                  |                    |                   |
|   USART1_TX (PA9) ---------------------> TXD               |
|  USART1_RX (PA10)<--------------------- RXD               |
|   GPIO_NSLP (PA8)---------------------> NSLP / EN         |
|                  |                    |                   |
|              GND ---------------------> GND               |
+------------------+                    |     LIN BUS       |
                                        |  <=============>  |
                                        +-------------------+
```

## Protocol Specifications

- **Baud Rate**: 19,200 baud (or 9,600 baud for LIN 1.3).
- **UART Frame**: 8 Data Bits, No Parity, 1 Stop Bit (8N1).
- **Break Field**: 13-bit dominant low level followed by 1-bit recessive break delimiter.
