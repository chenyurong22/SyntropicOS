# SyntropicOS STM32 DMX512 Slave Receiver Example

Demonstrates zero-allocation, non-blocking DMX512-A Slave Receiver (`syn_dmx512`) integrated with STM32 HAL USART drivers.

## Features
- **Break Detection**: Correct handling of USART Framing Errors (`HAL_UART_ERROR_FE`) in `HAL_UART_ErrorCallback` or Hardware Line Break Detection (`UART_IT_LBD`).
- **Start Code Filtering**: Filters Null Start Code (`0x00`) and extracts target slot values.
- **Auto Re-Arm**: Automatically clears USART error flags and re-enables 1-byte IT reception.
