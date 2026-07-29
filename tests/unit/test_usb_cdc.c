/**
 * @file test_usb_cdc.c
 * @brief Unit tests for Zero-Heap USB 2.0 CDC ACM Virtual COM Port Engine.
 */

#include "syntropic/drivers/syn_usb_cdc.h"
#include "unity/unity.h"

#include <string.h>

void test_usb_cdc_init(void)
{
    SYN_USB_CDC cdc;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_init(&cdc));
    TEST_ASSERT_EQUAL_UINT8(0x81, cdc.ep_in);
    TEST_ASSERT_EQUAL_UINT8(0x01, cdc.ep_out);
    TEST_ASSERT_EQUAL_UINT32(115200, cdc.line_coding.baudrate);
    TEST_ASSERT_FALSE(cdc.configured);
}

void test_usb_cdc_setup_requests(void)
{
    SYN_USB_CDC cdc;
    syn_usb_cdc_init(&cdc);

    SYN_USB_SetupPacket setup = {.bmRequestType = 0x00,
                                 .bRequest = SYN_USB_REQ_SET_ADDRESS,
                                 .wValue = 0x05,
                                 .wIndex = 0,
                                 .wLength = 0};

    uint8_t resp[16];
    size_t rlen = 0;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_handle_setup(&cdc, &setup, resp, &rlen));
    TEST_ASSERT_EQUAL_UINT8(5, cdc.dev_address);

    setup.bRequest = SYN_USB_REQ_SET_CONFIGURATION;
    setup.wValue = 1;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_handle_setup(&cdc, &setup, resp, &rlen));
    TEST_ASSERT_TRUE(cdc.configured);

    setup.bRequest = SYN_USB_CDC_GET_LINE_CODING;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_handle_setup(&cdc, &setup, resp, &rlen));
    TEST_ASSERT_EQUAL_INT(7, rlen);
}

void test_usb_cdc_read_write(void)
{
    SYN_USB_CDC cdc;
    syn_usb_cdc_init(&cdc);

    const char *hello = "Hello USB CDC";
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_write(&cdc, hello, strlen(hello)));
    TEST_ASSERT_EQUAL_UINT16(strlen(hello), cdc.tx_len);
    TEST_ASSERT_EQUAL_STRING(hello, (char *)cdc.tx_buf);

    uint8_t bigbuf[1000] = {0};
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_write(&cdc, bigbuf, sizeof(bigbuf)));
    TEST_ASSERT_EQUAL_UINT16(sizeof(cdc.tx_buf), cdc.tx_len);

    /* Simulate received USB OUT bulk data */
    const char *incoming = "Command 123";
    memcpy(cdc.rx_buf, incoming, strlen(incoming));
    cdc.rx_len = (uint16_t)strlen(incoming);

    char out_buf[32] = {0};
    size_t read_len = 0;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_read(&cdc, out_buf, sizeof(out_buf), &read_len));
    TEST_ASSERT_EQUAL_INT(strlen(incoming), read_len);
    TEST_ASSERT_EQUAL_STRING(incoming, out_buf);
}

void test_usb_cdc_null_checks(void)
{
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_cdc_init(NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_cdc_handle_setup(NULL, NULL, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_cdc_write(NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_cdc_read(NULL, NULL, 0, NULL));
}

void test_usb_cdc_extended_edge_cases(void)
{
    SYN_USB_CDC cdc;
    syn_usb_cdc_init(&cdc);

    /* Test SET_LINE_CODING, SET_CONTROL_LINE_STATE, and default setup requests */
    SYN_USB_SetupPacket setup = {.bmRequestType = 0x21,
                                 .bRequest = SYN_USB_CDC_SET_LINE_CODING,
                                 .wValue = 0,
                                 .wIndex = 0,
                                 .wLength = 7};
    uint8_t resp[16];
    size_t rlen = 0;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_handle_setup(&cdc, &setup, resp, &rlen));

    setup.bRequest = SYN_USB_CDC_SET_CONTROL_LINE_STATE;
    setup.wValue = 0x03; /* DTR | RTS */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_handle_setup(&cdc, &setup, resp, &rlen));

    setup.bRequest = 0xFE; /* Default unhandled request */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_handle_setup(&cdc, &setup, resp, &rlen));

    /* Test write with payload exceeding buffer size (clamping to 64 bytes) */
    uint8_t oversized[128];
    memset(oversized, 'A', sizeof(oversized));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_write(&cdc, oversized, sizeof(oversized)));
    TEST_ASSERT_EQUAL_UINT16(sizeof(cdc.tx_buf), cdc.tx_len);

    /* Test read when rx_len is 0 */
    uint8_t read_buf[32];
    size_t out_len = 999;
    cdc.rx_len = 0;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_read(&cdc, read_buf, sizeof(read_buf), &out_len));
    TEST_ASSERT_EQUAL_INT(0, out_len);

    /* Test partial read (max_len < rx_len) triggering memmove */
    const char *full_msg = "0123456789ABCDEF";
    memcpy(cdc.rx_buf, full_msg, 16);
    cdc.rx_len = 16;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_read(&cdc, read_buf, 8, &out_len));
    TEST_ASSERT_EQUAL_INT(8, out_len);
    TEST_ASSERT_EQUAL_STRING_LEN("01234567", (char *)read_buf, 8);
    TEST_ASSERT_EQUAL_UINT16(8, cdc.rx_len);
    TEST_ASSERT_EQUAL_STRING_LEN("89ABCDEF", (char *)cdc.rx_buf, 8);
}
