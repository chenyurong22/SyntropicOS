# STM32 HAL XCP over USART/UART Example

This example demonstrates how to integrate the SyntropicOS ASAM XCP v1.x (Universal Measurement and Calibration Protocol) slave stack (`syn_xcp.h`) with STM32 HAL USART/UART drivers (`HAL_UART_...`).

## Features
- **Zero Dynamic Memory Allocation (`malloc`)**: Static memory allocation for XCP slave instance and DAQ/ODT list structures.
- **XCP over Serial Transport Layer**:
  - Encapsulates XCP CTO command frames and DTO response/DAQ frames over UART using byte framing length headers.
  - Connect (`0xFF`), Disconnect (`0xFE`), Get Status (`0xFD`), Upload (`0xF5`), Download (`0xF0`), DAQ Streaming.
- **STM32 HAL USART Integration**:
  - Interrupt or DMA-driven byte reception via `HAL_UART_RxCpltCallback`.
  - Non-blocking transmission using `HAL_UART_Transmit` / `HAL_UART_Transmit_IT`.

## Hardware Setup
- Connect STM32 USART (e.g. USART1 / USART3 TX/RX pins) to a USB-to-UART serial converter (FTDI, CP2102, or CH340).
- Connect to an XCP master calibration tool over Serial/RS232/USB-UART (e.g. Vector CANape, pyxcp, or custom serial calibration host).
