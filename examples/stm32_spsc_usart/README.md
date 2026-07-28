# STM32 USART Interrupt RX with `syn_spsc_queue` Example

This example demonstrates how to use SyntropicOS **`syn_spsc_queue`** (lock-free Single-Producer Single-Consumer queue) to safely pass structured message packets from a hardware UART interrupt (ISR) to the main application loop without mutexes or critical sections.

## Key Differences: `syn_ringbuf` vs `syn_spsc_queue`

| Feature | `syn_ringbuf` | `syn_spsc_queue` |
| :--- | :--- | :--- |
| **Data Unit** | Raw byte stream (`uint8_t`) | Typed message objects/structs (`elem_size`) |
| **API** | `syn_ringbuf_put` / `get` / `read` | `syn_spsc_queue_push` / `pop` |
| **Use Case** | Serial stream buffering, CLI input | Telemetry events, framed packets, multithreaded queues |

## Hardware Setup
- **Board**: STM32 Nucleo / Discovery (STM32F4 / STM32F1)
- **UART**: USART2 (115200 8N1)
- **Pins**: PA2 (TX), PA3 (RX)

## Architecture

1. **Structured Message**:
   ```c
   typedef struct {
       uint32_t timestamp_ms;
       uint8_t data;
   } UART_RxEvent;
   ```

2. **ISR Producer (`HAL_UART_RxCpltCallback`)**:
   - Pushes an entire `UART_RxEvent` struct into the lock-free SPSC queue using `syn_spsc_queue_push(&spsc_q, &event)`.

3. **Main Loop Consumer (`syn_spsc_queue_pop`)**:
   - Pops events using `syn_spsc_queue_pop(&spsc_q, &event)` and processes timestamps & payload.
