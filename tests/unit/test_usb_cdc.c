/**
 * @file test_usb_cdc.c
 * @brief Unit tests for Zero-Heap USB 2.0 CDC ACM Virtual COM Port Engine.
 */

#include "syntropic/drivers/syn_transport_usb_cdc.h"
#include "syntropic/drivers/syn_usb_cdc.h"
#include "unity/unity.h"

#include <string.h>

static const uint8_t TEST_DEVICE_DESC[18] = {0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0xFE,
                                             0xCA, 0xEF, 0xBE, 0x00, 0x01, 0x01, 0x02, 0x00, 0x01};

void test_usb_cdc_init(void)
{
    SYN_USB_CDC cdc;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_init(&cdc));
    TEST_ASSERT_EQUAL_UINT8(0x81, cdc.ep_in);
    TEST_ASSERT_EQUAL_UINT8(0x01, cdc.ep_out);
    TEST_ASSERT_EQUAL_UINT32(115200, cdc.line_coding.baudrate);
    TEST_ASSERT_FALSE(cdc.configured);
}

void test_usb_cdc_registration(void)
{
    SYN_USB_Device dev;
    SYN_USB_CDC cdc;

    syn_usb_init(&dev, TEST_DEVICE_DESC);
    syn_usb_cdc_init(&cdc);

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_register(&dev, &cdc));
    TEST_ASSERT_EQUAL_UINT8(1, dev.class_count);

    /* Trigger class callbacks through device */
    SYN_USB_SetupPacket setup = {.bmRequestType = 0x00,
                                 .bRequest = SYN_USB_REQ_SET_CONFIGURATION,
                                 .wValue = 1,
                                 .wIndex = 0,
                                 .wLength = 0};
    uint8_t resp[16];
    uint16_t rlen = 0;
    syn_usb_process_setup(&dev, &setup, resp, &rlen);
    TEST_ASSERT_TRUE(cdc.configured);

    setup.bmRequestType = 0x21;
    setup.bRequest = SYN_USB_CDC_SET_CONTROL_LINE_STATE;
    setup.wValue = 0x03;
    syn_usb_process_setup(&dev, &setup, resp, &rlen);
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

    setup.bRequest = SYN_USB_CDC_SET_LINE_CODING;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_handle_setup(&cdc, &setup, resp, &rlen));

    setup.bRequest = 0xFF;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_handle_setup(&cdc, &setup, resp, &rlen));

    /* Partial read & zero rx_len read */
    uint8_t dummy_rx[16] = "1234567890";
    memcpy(cdc.rx_buf, dummy_rx, 10);
    cdc.rx_len = 10;

    uint8_t out_buf[16] = {0};
    size_t read_len = 0;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_read(&cdc, out_buf, 4, &read_len));
    TEST_ASSERT_EQUAL(4, read_len);
    TEST_ASSERT_EQUAL(6, cdc.rx_len);

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_read(&cdc, out_buf, sizeof(out_buf), &read_len));
    TEST_ASSERT_EQUAL(6, read_len);
    TEST_ASSERT_EQUAL(0, cdc.rx_len);

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_read(&cdc, out_buf, sizeof(out_buf), &read_len));
    TEST_ASSERT_EQUAL(0, read_len);

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
    TEST_ASSERT_FALSE(syn_usb_cdc_tx_ready(&cdc));

    uint8_t bigbuf[1000] = {0};
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_write(&cdc, bigbuf, sizeof(bigbuf)));
    TEST_ASSERT_EQUAL_UINT16(sizeof(cdc.tx_buf), cdc.tx_len);

    /* Simulate received USB OUT bulk data */
    const char *incoming = "Command 123";
    memcpy(cdc.rx_buf, incoming, strlen(incoming));
    cdc.rx_len = (uint16_t)strlen(incoming);

    TEST_ASSERT_TRUE(syn_usb_cdc_rx_available(&cdc));

    char out_buf[32] = {0};
    size_t read_len = 0;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_cdc_read(&cdc, out_buf, sizeof(out_buf), &read_len));
    TEST_ASSERT_EQUAL_INT(strlen(incoming), read_len);
    TEST_ASSERT_EQUAL_STRING(incoming, out_buf);
}

void test_usb_cdc_transport_bridge(void)
{
    SYN_USB_CDC cdc;
    SYN_Transport tr;

    syn_usb_cdc_init(&cdc);
    syn_transport_from_usb_cdc(&tr, &cdc);

    const char *msg = "Transport Test";
    TEST_ASSERT_TRUE(syn_transport_send(&tr, (const uint8_t *)msg, strlen(msg)));

    memcpy(cdc.rx_buf, msg, strlen(msg));
    cdc.rx_len = (uint16_t)strlen(msg);

    uint8_t read_buf[32] = {0};
    size_t out_len = 0;
    TEST_ASSERT_TRUE(syn_transport_recv(&tr, read_buf, sizeof(read_buf), &out_len));
    TEST_ASSERT_EQUAL_INT(strlen(msg), out_len);
    TEST_ASSERT_EQUAL_STRING(msg, (char *)read_buf);
}

void test_usb_cdc_null_checks(void)
{
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_cdc_init(NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_cdc_register(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_cdc_handle_setup(NULL, NULL, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_cdc_write(NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_cdc_read(NULL, NULL, 0, NULL));
    TEST_ASSERT_FALSE(syn_usb_cdc_rx_available(NULL));
    TEST_ASSERT_FALSE(syn_usb_cdc_tx_ready(NULL));

    SYN_Transport tr;
    syn_transport_from_usb_cdc(NULL, NULL);
    syn_transport_from_usb_cdc(&tr, NULL);
    TEST_ASSERT_FALSE(syn_transport_send(&tr, (const uint8_t *)"a", 1));

    SYN_USB_CDC cdc_inst;
    syn_transport_from_usb_cdc(&tr, &cdc_inst);
    TEST_ASSERT_FALSE(syn_transport_send(&tr, NULL, 0));
    TEST_ASSERT_FALSE(syn_transport_send(&tr, (const uint8_t *)"a", 0));
    uint8_t out_b[10];
    size_t out_l;
    TEST_ASSERT_FALSE(syn_transport_recv(&tr, NULL, sizeof(out_b), &out_l));
    TEST_ASSERT_FALSE(syn_transport_recv(&tr, out_b, sizeof(out_b), NULL));
}
