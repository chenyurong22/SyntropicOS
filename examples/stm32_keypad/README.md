# SyntropicOS 4x4 Matrix Keypad STM32 HAL Example

Demonstrates non-blocking 4x4 matrix keypad scanning (`SYN_Keypad`), debouncing, keypress event callbacks, and PIN code security entry handling using STM32 HAL GPIO drivers (`HAL_GPIO_...`).

## Architecture & Features

- **Matrix Scanning Engine**: Non-blocking row-by-column scanning (`syn_keypad_scan`) for 4x4 (16 keys: `0-9`, `A-D`, `*`, `#`) or 3x4 keypads.
- **Event Callbacks**: Asynchronous key press and key release events (`syn_keypad_set_callback`) with press counter tracking.
- **PIN Code Verification**: Buffers entered key characters, validates multi-digit security passcodes (e.g. `1234#`), and controls access lock GPIO outputs.
- **Zero-Heap Implementation**: Operates with statically allocated row/col pin arrays and keymap strings without dynamic memory allocation.

## Hardware Wiring

```
+--------------------+                    +---------------------+
|  STM32 Micro       |                    |  4x4 Matrix Keypad  |
|  (e.g., STM32F4)   |                    |  (Membrane / Switch)|
|                    |                    |                     |
|  GPIO_ROW0 (PA0)  ---------------------> Row 0                |
|  GPIO_ROW1 (PA1)  ---------------------> Row 1                |
|  GPIO_ROW2 (PA2)  ---------------------> Row 2                |
|  GPIO_ROW3 (PA3)  ---------------------> Row 3                |
|                    |                    |                     |
|  GPIO_COL0 (PA4)  <--------------------- Col 0 (Pull-Down)    |
|  GPIO_COL1 (PA5)  <--------------------- Col 1 (Pull-Down)    |
|  GPIO_COL2 (PA6)  <--------------------- Col 2 (Pull-Down)    |
|  GPIO_COL3 (PA7)  <--------------------- Col 3 (Pull-Down)    |
|                    |                    |                     |
|  GPIO_LOCK (PB0)  ---------------------> Relay / Solenoid Lock|
+--------------------+                    +---------------------+
```

## Keymap Grid Layout (4x4)

```
        Col 0   Col 1   Col 2   Col 3
Row 0 |   1   |   2   |   3   |   A   |
Row 1 |   4   |   5   |   6   |   B   |
Row 2 |   7   |   8   |   9   |   C   |
Row 3 |   *   |   0   |   #   |   D   |
```

- **Row Pins**: Outputs (High / Low drive).
- **Col Pins**: Inputs (Pulled Down to GND).
- **Scan Period**: 10ms (100Hz scanning rate).
