# STM32 USART Interrupt RX with `syn_ringbuf` Example

This example demonstrates how to use the SyntropicOS **`syn_ringbuf`** (statically allocated, ISR-safe ring buffer) to buffer incoming UART/USART bytes on STM32 microcontrollers using STM32Cube HAL interrupts.

## Hardware Setup
- **Board**: STM32 Nucleo / Discovery / Blue Pill (STM32F1/F4/L4/H7)
- **UART**: USART1 or USART2 (115200 baud, 8N1)
- **Pins**: TX = PA9 / PA2, RX = PA10 / PA3

## Key Concepts

1. **Single-Producer / Single-Consumer ISR Safety**:
   - `SYN_RingBuf` uses lock-free `volatile head` and `volatile tail` pointers, making it safe for 1 ISR producer (UART RX interrupt) and 1 main loop consumer without critical sections.
   
2. **ISR Producer (`HAL_UART_RxCpltCallback`)**:
   - Every received byte in `HAL_UART_RxCpltCallback()` is pushed to the ring buffer using `syn_ringbuf_put(&rx_ringbuf, rx_byte)`.
   - Re-arms `HAL_UART_Receive_IT()` immediately.

3. **Main Loop Consumer (`syn_ringbuf_get` / `syn_ringbuf_read`)**:
   - Non-blocking polling in main loop retrieves available bytes using `syn_ringbuf_get()` or bulk `syn_ringbuf_read()`.
   - Processes data without stalling hardware UART interrupts.

## Building
Include `syntropic/util/syn_ringbuf.h` and `port/stm32_hal/port_stm32_hal.h` in your build system.
