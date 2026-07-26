# STM32 Infrared (IR) Remote Receiver Example (`syn_ir`)

This example demonstrates how to capture and decode signals from standard **Consumer IR Remote Controls** (NEC, Sony SIRCS, Samsung, Philips RC5, RC6, Panasonic, Denon) using an **STM32 microcontroller** and SyntropicOS **`syn_ir`**.

---

## Hardware Configuration & Wiring

Connect a standard 38kHz IR receiver module (**TSOP38238**, **VS1838B**, or **ARM338**) to the STM32:

| IR Receiver Module Pin | STM32 Pin | Description |
|---|---|---|
| **OUT (Data)** | **PA0** | GPIO input with Dual-Edge Interrupt (EXTI0) |
| **VCC** | **3.3V / 5V** | Power supply (3.3V for TSOP38238) |
| **GND** | **GND** | Ground reference |

> **Note on Active-Low Output**: Standard IR receivers output **`0V` (LOW)** when a 38kHz infrared burst (MARK) is detected, and **`3.3V` (HIGH)** when idle (SPACE).

---

## Signal Capture Approaches

Microcontroller hardware IR capture can be implemented using two standard approaches:

### Method 1: GPIO EXTI Dual-Edge Interrupt + Microsecond Counter (Recommended)
Attach an EXTI interrupt configured for **Rising & Falling Edges** to PA0, and measure pulse durations in microseconds using the Cortex-M **DWT Cycle Counter (`DWT->CYCCNT`)**:

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == IR_RX_PIN) {
        uint32_t now_us = dwt_get_us();
        uint32_t duration_us = now_us - last_edge_us;
        last_edge_us = now_us;

        // Active-low: 0V = Mark (Carrier ON), 3.3V = Space (Carrier OFF)
        bool is_mark = (HAL_GPIO_ReadPin(IR_RX_PORT, IR_RX_PIN) == GPIO_PIN_RESET);

        SYN_IR_Frame frame;
        if (syn_ir_decode_pulse(&ir_decoder, (uint16_t)duration_us, is_mark, &frame)) {
            // Decoded frame! (frame.protocol, frame.address, frame.command)
        }
    }
}
```

### Method 2: Periodic 50µs Software Timer Interrupt
Sample the GPIO pin state in a 50 microsecond timer ISR:

```c
static bool last_pin_state = true;
static uint16_t state_duration_us = 0;

void on_50us_timer_interrupt(void) {
    bool current_pin_state = (HAL_GPIO_ReadPin(IR_RX_PORT, IR_RX_PIN) == GPIO_PIN_RESET); // true = Mark

    if (current_pin_state == last_pin_state) {
        state_duration_us += 50;
    } else {
        SYN_IR_Frame rx_frame;
        syn_ir_decode_pulse(&ir_decoder, state_duration_us, last_pin_state, &rx_frame);

        last_pin_state = current_pin_state;
        state_duration_us = 50;
    }
}
```

---

## Output Preview

```text
[IR RX] STM32 IR Receiver ready. Point an NEC/Sony/Samsung remote at PA0.
[IR RX] Decoded Frame: Protocol=0 (NEC), Addr=0x00FF, Cmd=0x0045, Repeat=NO
[IR RX] Decoded Frame: Protocol=0 (NEC), Addr=0x00FF, Cmd=0x0045, Repeat=YES
[IR RX] Decoded Frame: Protocol=1 (Sony SIRCS), Addr=0x0001, Cmd=0x0012, Repeat=NO
[IR RX] Decoded Frame: Protocol=4 (Samsung), Addr=0x0707, Cmd=0x0002, Repeat=NO
```
