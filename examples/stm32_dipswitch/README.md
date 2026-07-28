# SyntropicOS 8-Position DIP Switch STM32 HAL Example

Demonstrates 8-position DIP switch hardware reading (`SYN_DipSwitch`), bitmask value packing (`syn_dipswitch_get_value`), state change detection (`syn_dipswitch_has_changed`), and hardware Modbus/CAN bus address selection using STM32 HAL GPIO drivers (`HAL_GPIO_...`).

## Architecture & Features

- **Multi-Bit Position Packing**: Packs 8 individual GPIO switch inputs into an 8-bit integer bitmask ($0..255$) representing slave device address or baud rate selection.
- **Active-Low / Pull-Up Support**: Configures internal/external pull-ups with Active-Low logic (`active_low = true`) where ON connects the pin to GND.
- **Edge & Change Detection**: Detects DIP switch position toggles between scan passes (`syn_dipswitch_has_changed`).
- **Hardware Integration**: Uses 8 STM32 GPIO pins (PA0..PA7) connected to an 8-way DIP switch module.

## Hardware Wiring

```
+--------------------+                    +---------------------+
|  STM32 Micro       |                    |  8-Position DIP     |
|  (e.g., STM32F4)   |                    |  Switch Module      |
|                    |                    |                     |
|   GPIO_DS1 (PA0)  <---------------------> Position 1 (Bit 0)  |
|   GPIO_DS2 (PA1)  <---------------------> Position 2 (Bit 1)  |
|   GPIO_DS3 (PA2)  <---------------------> Position 3 (Bit 2)  |
|   GPIO_DS4 (PA3)  <---------------------> Position 4 (Bit 3)  |
|   GPIO_DS5 (PA4)  <---------------------> Position 5 (Bit 4)  |
|   GPIO_DS6 (PA5)  <---------------------> Position 6 (Bit 5)  |
|   GPIO_DS7 (PA6)  <---------------------> Position 7 (Bit 6)  |
|   GPIO_DS8 (PA7)  <---------------------> Position 8 (Bit 7)  |
|                    |                    |                     |
|                GND ---------------------> Common Switch GND   |
+--------------------+                    +---------------------+
```

## Switch Bitmask Layout

| Switch Pin | Bit Position | Bit Weight | Configuration Function |
|---|---|---|---|
| PA0 (SW1) | Bit 0 | 1 ($2^0$) | Modbus / CAN Node ID Bit 0 |
| PA1 (SW2) | Bit 1 | 2 ($2^1$) | Modbus / CAN Node ID Bit 1 |
| PA2 (SW3) | Bit 2 | 4 ($2^2$) | Modbus / CAN Node ID Bit 2 |
| PA3 (SW4) | Bit 3 | 8 ($2^3$) | Modbus / CAN Node ID Bit 3 |
| PA4 (SW5) | Bit 4 | 16 ($2^4$) | Modbus / CAN Node ID Bit 4 |
| PA5 (SW6) | Bit 5 | 32 ($2^5$) | Modbus / CAN Node ID Bit 5 |
| PA6 (SW7) | Bit 6 | 64 ($2^6$) | Baud Rate Select Bit 0 |
| PA7 (SW8) | Bit 7 | 128 ($2^7$) | Parity Select Bit 1 |
