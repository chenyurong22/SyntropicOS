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
