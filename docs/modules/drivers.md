# Hardware Peripheral Drivers

SyntropicOS provides portable hardware abstraction drivers for GPIO, UART, ADC, DAC, CAN, SPI, I2C, RTC, and DMA. Every driver is guarded by compile-time configuration switches (`SYN_USE_*`).

---

## Technical Specifications

| Feature | Specification |
|---|---|
| **Port Interface** | Hardware-independent wrapper calling `syn_port_*` interfaces. |
| **ISR Safety** | UART and DMA drivers use lock-free SPSC ring buffers for safe ISR-to-task transfers. |
| **Memory Allocation** | **100% Static / Zero Heap**. All driver instances are caller-owned structures. |

---

## Driver Dataflow Pipeline (UART + DMA Example)

```mermaid
flowchart LR
    HW["Hardware Peripheral (UART / ADC)"] -- ISR / DMA Interrupt --> RingBuf["SPSC Ring Buffer (syn_ringbuf)"]
    RingBuf -- syn_uart_read() --> Task["Cooperative Protothread Task"]
    Task --> Processing["Process Byte Stream"]
```

---

## 1. GPIO & Digital I/O (`drivers/syn_gpio.h`)

Provides pin initialization, reading, writing, toggling, and mode configuration (Input, Output, Pull-Up, Pull-Down, Open-Drain).

```c
#include <syntropic/drivers/syn_gpio.h>

void gpio_demo(void) {
    // Initialize pin 13 as Output
    syn_gpio_init(13, SYN_GPIO_OUTPUT);
    
    // Toggle pin state
    syn_gpio_toggle(13);
    
    // Read input level
    SYN_GPIO_State state = syn_gpio_read(12);
}
```

---

## 2. Buffered Serial UART (`drivers/syn_uart.h`)

Buffered UART driver using lock-free SPSC ring buffers for high-speed RX/TX without data loss.

```c
#include <syntropic/drivers/syn_uart.h>

static uint8_t rx_buf[128];
static uint8_t tx_buf[128];
static SYN_UART uart;

void uart_setup(void) {
    // Initialize UART 1 at 115200 8N1
    syn_uart_init(&uart, 1, 115200, rx_buf, sizeof(rx_buf), tx_buf, sizeof(tx_buf));
}

void USART1_IRQHandler(void) {
    // ISR feed: safe to call from interrupt context
    uint8_t rx_byte = (uint8_t)USART1->DR;
    syn_uart_isr_rx_byte(&uart, rx_byte);
}
```

---

## 3. Analog-to-Digital Converter (`drivers/syn_adc.h`)

Provides ADC sampling with oversampling, EMA filtering, voltage conversion (`mV`), and signal statistics integration.

```c
#include <syntropic/drivers/syn_adc.h>

static SYN_ADC adc_ch0;

void adc_setup(void) {
    SYN_ADC_Config cfg = {
        .channel = 0,
        .oversample = 4, // 4x oversampling for noise reduction
        .filter = NULL
    };
    syn_adc_init(&adc_ch0, &cfg);
}

void read_voltage(void) {
    syn_adc_read(&adc_ch0);
    uint32_t millivolts = syn_adc_millivolts(&adc_ch0);
    printf("Channel 0: %lu mV\n", (unsigned long)millivolts);
}
```

---

## 4. DMA Transaction Engine (`drivers/syn_dma.h`)

Bare-metal safe DMA transaction engine featuring address alignment verification, D-cache invalidation, and atomic busy protection.

```c
#include <syntropic/drivers/syn_dma.h>

static SYN_DMA dma;

void on_dma_complete(SYN_DMA *dma_inst, void *ctx) {
    printf("DMA Transfer Complete!\n");
}

void start_dma_transfer(const uint32_t *src, uint32_t *dst, size_t count) {
    syn_dma_init(&dma, 0, on_dma_complete, NULL);
    syn_dma_start(&dma, (const void*)src, (void*)dst, count * sizeof(uint32_t));
}
```
