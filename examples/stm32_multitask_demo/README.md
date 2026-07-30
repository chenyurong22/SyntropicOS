# SyntropicOS Bare-Metal STM32 Integrated Multi-Task Protothreads (`SYN_PT`) Example

Demonstrates four concurrent stackless coroutine protothreads (`SYN_PT`) coordinating hardware peripherals and shared global resources in C99 on bare-metal STM32 hardware with zero per-task RAM stack overhead.

## Architecture & Protothread Layout

```
                  +----------------------------------------------+
                  |           SyntropicOS Super-Loop             |
                  +-------+----------+-----------+---------------+
                          |          |           |
          +---------------+          |           +---------------+
          |                          |                           |
          v                          v                           v
  +---------------+          +---------------+           +---------------+
  |  task_button  |          |  task_usart   |           |   task_led    |
  |  (Button PT)  |          |   (USART PT)  |           |    (LED PT)   |
  +-------+-------+          +-------+-------+           +-------+-------+
          |                          |                           ^
          | Post Gesture Event       | Set Mode Command          | Drive Pattern
          v                          v                           |
  +--------------------------------------------------------------+
  |              App_GlobalResources (IPC State)                 |
  |  - current_mode  - button_press_count  - active_errors       |
  +--------------------------------------------------------------+
                                 ^
                                 | Process Event
                         +-------+-------+
                         |   task_ipc    |
                         |   (IPC PT)    |
                         +---------------+
```

## Concurrent Protothread Tasks

1. **`task_button` (Button Task)**: Non-blocking sampling of user push button (`PC13`). Evaluates hold duration to post `EVENT_BUTTON_SHORT_PRESS` and `EVENT_BUTTON_LONG_PRESS` gesture events.
2. **`task_usart` (USART CLI Task)**: Ingests console serial input, line buffers characters, and dispatches zero-malloc CLI commands (`status`, `mode <idle|active|alert|config>`).
3. **`task_ipc` (Global Resource Coordinator)**: Inter-task coordinator monitoring the shared thread-safe `App_GlobalResources` struct. Translates button gestures and CLI commands into system mode state transitions.
4. **`task_led` (Status LED Task)**: Dynamic LED pattern driver (`PA5`). Adjusts flash frequency (`1Hz` slow blink, `4Hz` active blink, `10Hz` rapid alert, or solid ON config) based on `current_mode`.

## Hardware Wiring

```
+-------------------+                    +-------------------+
|  STM32 NUCLEO-F4  |                    |  USB-to-UART /    |
|   Microcontroller |                    |  Serial Console   |
|                   |                    |                   |
|   USART2_TX (PA2) --------------------> RXD               |
|   USART2_RX (PA3) <-------------------- TXD               |
|   USER_LED  (PA5) --------------------> Green LED         |
|   USER_BUTTON(PC13)<------------------- Push Button (Active LOW)
|               GND --------------------> GND               |
+-------------------+                    +-------------------+
```

## Protocol & Serial Port Configuration

- **Baud Rate**: 115,200 baud
- **UART Frame**: 8 Data Bits, No Parity, 1 Stop Bit (8N1)
- **Line Termination**: `\r\n`

## Interactive CLI Commands

- `status` — Dump MCU uptime, system mode, button count, CLI commands, and active error counters.
- `mode <idle|active|alert|config>` — Explicitly switch system mode.
- `help` — Print command help catalog.
