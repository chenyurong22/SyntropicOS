# SyntropicOS Standard GPIO LED STM32 HAL Example

Demonstrates non-blocking status LED control (`SYN_LED`), active-high / active-low polarity configuration, heartbeat blinking (`syn_led_blink`), toggle operations (`syn_led_toggle`), and custom flash patterns (`syn_led_pattern`) using STM32 HAL GPIO drivers (`HAL_GPIO_...`).

## Architecture & Features

- **Polarity Handling**: Supports Active-High (`SYN_LED_ACTIVE_HIGH`) and Active-Low (`SYN_LED_ACTIVE_LOW`) LED wiring without conditional branching in application code.
- **Heartbeat & Blinking**: Non-blocking periodic state toggling with configurable ON/OFF millisecond durations (`syn_led_blink`).
- **Pattern Sequence Decoding**: Plays bitmask flash sequences (e.g. SOS morse code or error diagnostic codes) via `syn_led_pattern`.
- **Zero-Delay Execution**: State changes occur based on system tick timestamps (`syn_port_get_tick_ms`) without blocking thread execution using `HAL_Delay`.

## Hardware Wiring

```
+--------------------+                    +---------------------+
|  STM32 Micro       |                    |  Status LED         |
|  (e.g., STM32F4)   |                    |                     |
|                    |                    |                     |
|  GPIO_LED   (PA0)  ---------------------> Anode (via 330 ohm) |
|                GND ---------------------> Cathode             |
+--------------------+                    +---------------------+
```

## Protocol & Timing Specifications

- **Scan Period**: 10ms (100Hz tick update rate).
- **Heartbeat Rate**: 1Hz (500ms ON / 500ms OFF).
- **Error Flash Code**: 3 rapid flashes (100ms ON / 100ms OFF) repeated every 2 seconds.
