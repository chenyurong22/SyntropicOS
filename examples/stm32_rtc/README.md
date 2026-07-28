# SyntropicOS Real-Time Clock (RTC) & Calendar STM32 HAL Example

Demonstrates Real-Time Clock (RTC) date and time management (`syn_rtc`), 1ms SysTick time accumulation (`syn_port_get_tick_ms`), perpetual calendar leap year calculations, Unix epoch timestamp conversions (`syn_rtc_to_epoch` / `syn_rtc_from_epoch`), and date-time validation (`syn_rtc_is_valid`) using STM32 HAL drivers.

## Architecture & Features

- **Perpetual Calendar Engine**: Converts between 32-bit UTC Unix epoch timestamps (`uint32_t`) and human-readable date/time structures (`SYN_RTC_DateTime`).
- **Leap Year & Month Bounds**: Automatically calculates leap years (February 29 vs 28) and variable month day counts (28, 29, 30, 31).
- **Date/Time Validation**: Validates year ($1970..2099$), month ($1..12$), day ($1..31$), hour ($0..23$), minute ($0..59$), and second ($0..59$) bounds (`syn_rtc_is_valid`).
- **Software RTC Accumulation**: Accumulates 1ms SysTick ticks to increment the calendar epoch timestamp without requiring an external 32.768kHz crystal.
- **USART Serial Command Interface**: Supports reading (`GET_TIME`) and updating (`SET_TIME=YYYY-MM-DD HH:MM:SS`) the real-time clock over USART.

## Hardware Wiring

```
+--------------------+                    +---------------------+
|  STM32 Micro       |                    |  USB-to-UART Adapter|
|  (e.g., STM32F4)   |                    |  (FT232 / CP2102)   |
|                    |                    |                     |
|  USART1_TX (PA9)  ---------------------> RXD Pin              |
|  USART1_RX (PA10) <--------------------- TXD Pin              |
|                GND ---------------------> GND                 |
+--------------------+                    +---------------------+
```

## Serial Protocol Specification

- **Baud Rate**: 115200 bps, 8 data bits, no parity, 1 stop bit (8N1).
- **Get Time Command**: `GET_TIME\r\n` $\rightarrow$ Returns `TIME=2026-07-28 12:00:00\r\n`
- **Set Time Command**: `SET_TIME=2026-07-28 12:30:00\r\n` $\rightarrow$ Returns `OK TIME=2026-07-28 12:30:00\r\n`
