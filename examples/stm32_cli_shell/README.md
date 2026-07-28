# SyntropicOS Embedded CLI Shell STM32 HAL Example

Demonstrates interactive Command Line Interface (CLI) shell integration over STM32 HAL USART drivers (`HAL_UART_...`). Implements command parsing, argument handling, line editing (backspace), local echo, and custom user shell commands for controlling onboard LEDs and reading MCU telemetry.

## Architecture & Features

- **Zero-Heap Command Parsing**: Statically registered command dispatch table (`SYN_CLI_Command`) with zero `malloc()` memory allocations.
- **Interactive Line Editing**: Supports character-by-character UART RX interrupt processing (`syn_cli_process_char`), line buffering, ANSI escape sequences, backspace, and customizable shell prompt (e.g. `stm32> `).
- **Custom Shell Commands**:
  - `led <on|off|toggle>` — Control onboard GPIO LED state (`HAL_GPIO_WritePin`).
  - `gpio <pin> <0|1>` — Control target GPIO output pin.
  - `temp` — Read CPU internal temperature sensor / ADC telemetry.
  - `status` — Dump system status, uptime ticks (`syn_port_get_tick_ms`), and task metrics.
  - `help` — Automatic built-in help text generator for registered commands.

## Hardware Wiring

```
+------------------+                    +-------------------+
|  STM32 Micro     |                    |  USB-to-UART /    |
|  (e.g., STM32F4) |                    |  Serial Console   |
|                  |                    |                   |
|   USART1_TX (PA9) ---------------------> RXD               |
|  USART1_RX (PA10)<--------------------- TXD               |
|  GPIO_LED  (PA5) ---------------------> Onboard LED       |
|                  |                    |                   |
|              GND ---------------------> GND               |
+------------------+                    +-------------------+
```

## Protocol & Serial Port Configuration

- **Baud Rate**: 115,200 baud.
- **UART Frame**: 8 Data Bits, No Parity, 1 Stop Bit (8N1).
- **Line Termination**: Carriage Return / Line Feed (`\r\n`).
