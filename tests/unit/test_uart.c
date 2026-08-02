#include "mocks/mock_port.h"
#include "syntropic/drivers/syn_uart.h"
#include "unity/unity.h"

#include <string.h>

static SYN_UART uart;

/** Init: success path */
static void test_uart_init(void)
{
    SYN_Status st = syn_uart_init(&uart, 0, 115200);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_TRUE(uart.initialized);
    TEST_ASSERT_EQUAL(0, uart.instance);
}

/** Deinit: success path */
static void test_uart_deinit(void)
{
    syn_uart_init(&uart, 0, 115200);
    SYN_Status st = syn_uart_deinit(&uart);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_FALSE(uart.initialized);
}

/** Deinit when not initialized — returns OK without calling port */
static void test_uart_deinit_not_initialized(void)
{
    memset(&uart, 0, sizeof(uart));
    uart.initialized = false;
    SYN_Status st = syn_uart_deinit(&uart);
    TEST_ASSERT_EQUAL(SYN_OK, st);
}

/** Write string */
static void test_uart_write_str(void)
{
    syn_uart_init(&uart, 0, 115200);
    mock_uart_tx_len = 0;
    SYN_Status st = syn_uart_write_str(&uart, "hello", 1000);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(5, mock_uart_tx_len);
    TEST_ASSERT_EQUAL_UINT8('h', mock_uart_tx_buf[0]);
}

/** Write string: empty string */
static void test_uart_write_str_empty(void)
{
    syn_uart_init(&uart, 0, 115200);
    mock_uart_tx_len = 0;
    SYN_Status st = syn_uart_write_str(&uart, "", 1000);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(0, mock_uart_tx_len);
}

/** Write bytes */
static void test_uart_write(void)
{
    syn_uart_init(&uart, 0, 115200);
    mock_uart_tx_len = 0;
    uint8_t data[] = {0xAA, 0xBB, 0xCC};
    SYN_Status st = syn_uart_write(&uart, data, sizeof(data), 1000);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(3, mock_uart_tx_len);
    TEST_ASSERT_EQUAL_UINT8(0xAA, mock_uart_tx_buf[0]);
}

/** Write bytes: zero length */
static void test_uart_write_zero_len(void)
{
    syn_uart_init(&uart, 0, 115200);
    mock_uart_tx_len = 0;
    SYN_Status st = syn_uart_write(&uart, NULL, 0, 1000);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(0, mock_uart_tx_len);
}

/** Read from rx ringbuffer */
static void test_uart_read(void)
{
    syn_uart_init(&uart, 0, 115200);
    /* Feed bytes via ISR */
    syn_uart_rx_isr_feed(&uart, 'A');
    syn_uart_rx_isr_feed(&uart, 'B');
    syn_uart_rx_isr_feed(&uart, 'C');

    uint8_t buf[8];
    size_t n = syn_uart_read(&uart, buf, sizeof(buf));
    TEST_ASSERT_EQUAL(3, n);
    TEST_ASSERT_EQUAL_UINT8('A', buf[0]);
    TEST_ASSERT_EQUAL_UINT8('B', buf[1]);
    TEST_ASSERT_EQUAL_UINT8('C', buf[2]);
}

/** Read when empty */
static void test_uart_read_empty(void)
{
    syn_uart_init(&uart, 0, 115200);
    uint8_t buf[8];
    size_t n = syn_uart_read(&uart, buf, sizeof(buf));
    TEST_ASSERT_EQUAL(0, n);
}

/** rx_isr_feed: feed and verify */
static void test_uart_rx_isr_feed(void)
{
    syn_uart_init(&uart, 0, 115200);
    bool ok = syn_uart_rx_isr_feed(&uart, 0x42);
    TEST_ASSERT_TRUE(ok);

    /* Fill buffer until overflow */
    for (size_t i = 0; i < sizeof(uart.rx_buf) - 2; i++) {
        syn_uart_rx_isr_feed(&uart, (uint8_t)i);
    }
    /* Next feed should fail because rx_rb is full */
    TEST_ASSERT_FALSE(syn_uart_rx_isr_feed(&uart, 0xFF));
}

/** Init: port fails */
static void test_uart_init_fail(void)
{
    mock_uart_init_fail = true;
    SYN_Status st = syn_uart_init(&uart, 0, 115200);
    TEST_ASSERT_EQUAL(SYN_ERROR, st);
    TEST_ASSERT_FALSE(uart.initialized);
    mock_uart_init_fail = false;
}

/** Init config: DMA and non-DMA paths */
static void test_uart_init_config(void)
{
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_uart_init_config(NULL, NULL));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_uart_init_config(&uart, NULL));

    static const uint8_t dummy_reg = 0;
    SYN_UART_Config cfg = {.instance = 1,
                           .baudrate = 115200,
                           .use_dma = false,
#if defined(SYN_USE_DMA) && SYN_USE_DMA
                           .dma_channel_rx = 0,
                           .periph_rx_reg = &dummy_reg
#endif
    };

    TEST_ASSERT_EQUAL(SYN_OK, syn_uart_init_config(&uart, &cfg));
    TEST_ASSERT_TRUE(uart.initialized);
    TEST_ASSERT_EQUAL(1, uart.instance);
    TEST_ASSERT_FALSE(uart.use_dma);

    syn_uart_deinit(&uart);

#if defined(SYN_USE_DMA) && SYN_USE_DMA
    cfg.use_dma = true;
    TEST_ASSERT_EQUAL(SYN_OK, syn_uart_init_config(&uart, &cfg));
    TEST_ASSERT_TRUE(uart.initialized);
    TEST_ASSERT_TRUE(uart.use_dma);

    uint8_t rx_data[16];
    size_t read_bytes = syn_uart_read(&uart, rx_data, sizeof(rx_data));
    (void)read_bytes;

    syn_uart_deinit(&uart);

    /* DMA start failure path when periph_rx_reg is NULL */
    cfg.periph_rx_reg = NULL;
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_uart_init_config(&uart, &cfg));
    TEST_ASSERT_FALSE(uart.initialized);
#endif
}

void run_uart_tests(void)
{
    RUN_TEST(test_uart_init);
    RUN_TEST(test_uart_init_config);
    RUN_TEST(test_uart_init_fail);
    RUN_TEST(test_uart_deinit);
    RUN_TEST(test_uart_deinit_not_initialized);
    RUN_TEST(test_uart_write_str);
    RUN_TEST(test_uart_write_str_empty);
    RUN_TEST(test_uart_write);
    RUN_TEST(test_uart_write_zero_len);
    RUN_TEST(test_uart_read);
    RUN_TEST(test_uart_read_empty);
    RUN_TEST(test_uart_rx_isr_feed);
}
