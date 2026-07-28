# Communication & Networking Modules

SyntropicOS provides a comprehensive suite of communication protocols ranging from zero-overhead byte stuffing and point-to-point packet routing up to industrial fieldbuses and automotive networks.

---

## Protocol Overview

| Layer | Module | Header | Description |
|---|---|---|---|
| **Framing** | COBS | `proto/syn_cobs.h` | Consistent Overhead Byte Stuffing (`0x00` packet delimiter) |
| **Routing** | Router | `net/syn_router.h` | Addressed packet dispatch (Node ID), type routing, & ACKs |
| **Industrial** | Modbus | `proto/syn_modbus.h` | Modbus RTU & Modbus TCP Master/Slave support |
| **Building** | BACnet MS/TP | `proto/syn_bacnet.h` | ANSI/ASHRAE 135 BACnet MS/TP framing, APDU codec, & Object DB |
| **Building** | DALI | `proto/syn_dali.h` | IEC 62386-101/102 DALI Master/Slave Manchester encoding |
| **Metering** | M-Bus | `proto/syn_mbus.h` | EN 13757-2 / EN 13757-3 European Meter Bus protocol |
| **Automotive** | ISO-TP | `proto/syn_isotp.h` | ISO 15765-2 multi-frame CAN transport layer |
| **Automotive** | J1939 | `proto/syn_j1939.h` | SAE J1939 heavy vehicle network protocol (PGN / SPN) |
| **Marine** | NMEA 2000 | `proto/syn_n2k.h` | NMEA 2000 marine CAN bus protocol decoder |

---

## 1. COBS & Packet Router Pipeline (`syn_cobs` + `syn_router`)

For MCU-to-MCU serial communication over UART TTL or RS232/RS485, SyntropicOS pairs **COBS Framing** with the **Addressed Router**.

### Pipeline Data Flow

```mermaid
flowchart LR
    UART["Single-Byte UART RX Interrupt"] --> Decoder["syn_cobs_decoder_feed()"]
    Decoder -- 0x00 Delimiter Found --> Assembly["Decoded Frame"]
    Assembly --> Router["syn_router_feed()"]
    Router -- Match Node ID & Msg Type --> Callback["Handler Callback (e.g. on_set_led)"]
```

### Complete STM32 HAL UART Single-Byte Interrupt Example

```c
#include <syntropic/proto/syn_cobs.h>
#include <syntropic/net/syn_router.h>

#define MASTER_NODE_ID 0x01
#define SLAVE_NODE_ID  0x02
#define MSG_TYPE_LED   0x10

static uint8_t rx_byte;
static SYN_COBS_Decoder cobs_dec;
static uint8_t cobs_buf[128];

static SYN_Router router;
static SYN_RouterHandler handlers[4];

// Custom Transport: Send framed COBS packet over UART
static SYN_Status uart_send(const uint8_t *data, size_t len, void *ctx) {
    uint8_t enc[140];
    size_t enc_len = syn_cobs_encode(data, len, enc);
    enc[enc_len++] = 0x00; // Append 0x00 frame delimiter
    
    HAL_UART_Transmit(&huart2, enc, (uint16_t)enc_len, 100);
    return SYN_OK;
}

static SYN_Transport transport = { .send = uart_send, .ctx = NULL };

// COBS Decoder Callback when a complete frame arrives
static void on_cobs_frame(const uint8_t *data, size_t len, void *ctx) {
    syn_router_feed(&router, data, len);
}

// Single-Byte UART Interrupt Callback (No DMA)
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        syn_cobs_decoder_feed(&cobs_dec, rx_byte);
        HAL_UART_Receive_IT(&huart2, &rx_byte, 1); // Re-arm interrupt
    }
}

// Message Handler Callback
static void on_led_command(const SYN_Packet *pkt, void *ctx) {
    if (pkt->len > 0 && pkt->payload[0] == 0x01) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET); // LED ON
    }
}

void app_init(void) {
    syn_cobs_decoder_init(&cobs_dec, cobs_buf, sizeof(cobs_buf), on_cobs_frame, NULL);
    
    syn_router_init(&router, SLAVE_NODE_ID, &transport, handlers, 4);
    syn_router_register(&router, MSG_TYPE_LED, on_led_command, NULL);
    
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}
```

---

## 2. M-Bus Protocol (`syn_mbus.h`)

The M-Bus (Meter-Bus) driver supports utility metering devices (water, gas, electricity, heat meters) compliant with EN 13757-2 / EN 13757-3.

### Features
- Single-byte ACK (`0xE5`) parsing.
- Short frame (`0x10`) and Long frame (`0x68`) header validation.
- Arithmetic checksum calculation and byte-at-a-time streaming parser.

```c
#include <syntropic/proto/syn_mbus.h>

void parse_mbus_stream(const uint8_t *buffer, size_t len) {
    SYN_MBusFrame frame;
    if (syn_mbus_parse_long(buffer, len, &frame) == SYN_OK) {
        printf("M-Bus Frame Received! C-Field: 0x%02X, Address: 0x%02X\n",
               frame.control, frame.address);
    }
}
```

---

## 3. BACnet MS/TP Protocol (`syn_bacnet.h`)

The BACnet MS/TP protocol engine implements ANSI/ASHRAE 135 / ISO 16484-5 token-passing serial communication over RS485 without dynamic memory allocation (`malloc`).

### Features
- **MS/TP Framing & CRC**: Encodes/decodes Preamble (`0x55 0xFF`), Header CRC-8, and Data CRC-16 (ANSI X3.28 polynomial).
- **APDU Services**: Supports `Who-Is` / `I-Am` unconfirmed services and `ReadProperty` / `WriteProperty` confirmed services.
- **Static Object Database**: Manages Device, Analog Input (AI), Analog Output (AO), Binary Input (BI), and Binary Output (BO) objects.

```c
#include <syntropic/proto/syn_bacnet.h>

static SYN_BACnet_Node node;

void setup_bacnet(void) {
    syn_bacnet_node_init(&node, 12, 123456); // MAC 12, Device ID 123456
    syn_bacnet_add_object(&node, SYN_BACNET_OBJ_ANALOG_INPUT, 1, 23.5f, "Temperature");
}
```

---

## 4. DALI Protocol (`syn_dali.h`)

The DALI (Digital Addressable Lighting Interface / IEC 62386-101/102) engine implements Master and Control Gear (Slave) lighting nodes.

### Features
- **Manchester Encoding/Decoding**: Bi-phase bit encoding/decoding helper functions.
- **Forward & Backward Frames**: 16-bit Master Forward Frame decoding (`syn_dali_decode_forward`) and 8-bit Slave Backward Frame encoding (`syn_dali_encode_backward`).
- **Direct Arc Power Control (DAPC)**: Manages logarithmic/linear arc dimming levels ($0..254$), min/max bounds, power-on levels, and system failure levels.

---

## 5. Modbus RTU Master & Slave (`syn_modbus.h`, `syn_modbus_master.h`)

SyntropicOS provides non-blocking state-machine drivers for both Modbus RTU Slave (`syn_modbus`) and Modbus RTU Master (`syn_modbus_master`).

### Features
- **Non-blocking State Machine**: Processing occurs without thread blocking via periodic polling (`syn_modbus_process` / `syn_modbus_master_process`).
- **Inter-Frame Silence (t3.5)**: Timing gap detection handles partial frame arrival cleanly over UART DMA / ring buffers.

