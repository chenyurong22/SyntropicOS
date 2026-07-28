# STM32 SIM800 Cellular MQTT Integration Example

This example demonstrates how to integrate a **SIMCom SIM800** (SIM800L / SIM800C / SIM800H) 2G/GPRS cellular modem with **SyntropicOS** on STM32 microcontrollers.

## Architecture

Rather than using vendor-locked modem MQTT commands, this example bridges the modem's TCP/IP stack to SyntropicOS's native `syn_port_socket` interface using protothreads and `syn_at_parser`.

```
┌─────────────────────────────────────────────────────────────┐
│                   SyntropicOS Application                   │
│                                                             │
│   ┌───────────────┐     ┌────────────────────────────────┐  │
│   │ syn_mqtt_task │ --> │ syn_port_socket (SIM800 Driver)│  │
│   └───────────────┘     └────────────────────────────────┘  │
│                                         │                   │
│                                 ┌───────────────┐           │
│                                 │ syn_at_parser │           │
│                                 └───────────────┘           │
│                                         │                   │
│                                 ┌───────────────┐           │
│                                 │  STM32 USART  │           │
│                                 └───────────────┘           │
└─────────────────────────────────────────┬───────────────────┘
                                          v
                                   SIM800 Cellular Modem
```

## Features
- **Zero Heap / Malloc Free**: 100% caller-owned static ring buffers and protothreads.
- **Non-Blocking AT Driver**: Initializes GPRS network attachment (`AT+CSQ`, `AT+CGATT`), opens TCP connections (`AT+CIPSTART`), sends data (`AT+CIPSEND`), and receives network packets without blocking the main CPU loop.
- **Native SyntropicOS MQTT**: Inherits full MQTT 3.1.1 protocol capabilities (QoS 0/1, subscriptions, retain flags, keep-alives) over cellular networks.

## Pinout / Wiring
- **STM32 USART2 TX (PA2)** -> SIM800 RXD
- **STM32 USART2 RX (PA3)** -> SIM800 TXD
- **GND** -> SIM800 GND
- **PWR / RST** -> Configurable GPIO output for hardware power toggling
