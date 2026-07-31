/**
 * @file test_usb.c
 * @brief Unit tests for Zero-Heap Modular USB 2.0 Device Core Protocol Engine.
 */

#include "syntropic/drivers/syn_usb.h"
#include "unity/unity.h"

#include <string.h>

/* Standard 18-byte Device Descriptor Template */
static const uint8_t TEST_DEVICE_DESC[18] = {0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0xFE,
                                             0xCA, 0xEF, 0xBE, 0x00, 0x01, 0x01, 0x02, 0x00, 0x01};

/* Test Class Setup Callback */
static SYN_Status test_class_setup(void *ctx, const SYN_USB_SetupPacket *pkt, uint8_t *resp,
                                   uint16_t *rlen)
{
    (void)ctx;
    if (pkt->bRequest == 0x77) {
        resp[0] = 0xAA;
        *rlen = 1;
        return SYN_OK;
    }
    return SYN_ERROR;
}

void test_usb_init_and_state(void)
{
    SYN_USB_Device dev;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_init(&dev, TEST_DEVICE_DESC));
    TEST_ASSERT_EQUAL_UINT8(SYN_USB_STATE_DEFAULT, dev.state);
    TEST_ASSERT_FALSE(syn_usb_is_configured(&dev));
}

void test_usb_set_address_and_config(void)
{
    SYN_USB_Device dev;
    syn_usb_init(&dev, TEST_DEVICE_DESC);

    SYN_USB_SetupPacket pkt = {.bmRequestType = 0x00,
                               .bRequest = SYN_USB_REQ_SET_ADDRESS,
                               .wValue = 0x0A,
                               .wIndex = 0,
                               .wLength = 0};
    uint8_t resp[64];
    uint16_t rlen = 0;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_process_setup(&dev, &pkt, resp, &rlen));
    TEST_ASSERT_EQUAL_UINT8(10, dev.dev_address);
    TEST_ASSERT_EQUAL_UINT8(SYN_USB_STATE_ADDRESS, dev.state);

    pkt.bRequest = SYN_USB_REQ_SET_CONFIGURATION;
    pkt.wValue = 1;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_process_setup(&dev, &pkt, resp, &rlen));
    TEST_ASSERT_EQUAL_UINT8(SYN_USB_STATE_CONFIGURED, dev.state);
    TEST_ASSERT_TRUE(syn_usb_is_configured(&dev));

    pkt.bRequest = SYN_USB_REQ_GET_CONFIGURATION;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_process_setup(&dev, &pkt, resp, &rlen));
    TEST_ASSERT_EQUAL_UINT16(1, rlen);
    TEST_ASSERT_EQUAL_UINT8(1, resp[0]);
}

void test_usb_get_descriptors(void)
{
    SYN_USB_Device dev;
    syn_usb_init(&dev, TEST_DEVICE_DESC);

    static const uint8_t str0[4] = {0x04, 0x03, 0x09, 0x04};
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_set_string_desc(&dev, 0, str0));

    SYN_USB_SetupPacket pkt = {.bmRequestType = 0x80,
                               .bRequest = SYN_USB_REQ_GET_DESCRIPTOR,
                               .wValue = (SYN_USB_DESC_TYPE_DEVICE << 8),
                               .wIndex = 0,
                               .wLength = 18};
    uint8_t resp[64];
    uint16_t rlen = 0;

    /* Get Device Descriptor */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_process_setup(&dev, &pkt, resp, &rlen));
    TEST_ASSERT_EQUAL_UINT16(18, rlen);
    TEST_ASSERT_EQUAL_MEMORY(TEST_DEVICE_DESC, resp, 18);

    /* Get Config Descriptor */
    pkt.wValue = (SYN_USB_DESC_TYPE_CONFIGURATION << 8);
    pkt.wLength = 256;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_process_setup(&dev, &pkt, resp, &rlen));
    TEST_ASSERT_EQUAL_UINT16(9, rlen);

    /* Get String Descriptor */
    pkt.wValue = (SYN_USB_DESC_TYPE_STRING << 8);
    pkt.wLength = 4;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_process_setup(&dev, &pkt, resp, &rlen));
    TEST_ASSERT_EQUAL_UINT16(4, rlen);
    TEST_ASSERT_EQUAL_MEMORY(str0, resp, 4);

    /* Get Status */
    pkt.bRequest = SYN_USB_REQ_GET_STATUS;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_process_setup(&dev, &pkt, resp, &rlen));
    TEST_ASSERT_EQUAL_UINT16(2, rlen);
    TEST_ASSERT_EQUAL_UINT8(0x01, resp[0]);
}

void test_usb_class_registration_and_routing(void)
{
    SYN_USB_Device dev;
    syn_usb_init(&dev, TEST_DEVICE_DESC);

    SYN_USB_ClassDriver cls = {.iface_start = 0,
                               .iface_count = 1,
                               .ctx = NULL,
                               .setup = test_class_setup,
                               .data_out = NULL,
                               .data_in = NULL,
                               .configured = NULL};

    uint8_t dummy_iface_desc[9] = {0x09, 0x04, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00};
    TEST_ASSERT_EQUAL_INT(
        SYN_OK, syn_usb_register_class(&dev, &cls, dummy_iface_desc, sizeof(dummy_iface_desc)));
    TEST_ASSERT_EQUAL_UINT16(18, dev.config_desc_len);

    /* Process class setup request targeted to interface 0 */
    SYN_USB_SetupPacket pkt = {.bmRequestType = 0x21, /* Class request to Interface */
                               .bRequest = 0x77,
                               .wValue = 0,
                               .wIndex = 0,
                               .wLength = 1};
    uint8_t resp[16];
    uint16_t rlen = 0;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_process_setup(&dev, &pkt, resp, &rlen));
    TEST_ASSERT_EQUAL_UINT16(1, rlen);
    TEST_ASSERT_EQUAL_UINT8(0xAA, resp[0]);
}

void test_usb_raw_config_override(void)
{
    SYN_USB_Device dev;
    syn_usb_init(&dev, TEST_DEVICE_DESC);

    static const uint8_t raw_config[9] = {0x09, 0x02, 0x09, 0x00, 0x01, 0x01, 0x00, 0xC0, 0x32};
    TEST_ASSERT_EQUAL_INT(SYN_OK,
                          syn_usb_set_raw_config_desc(&dev, raw_config, sizeof(raw_config)));

    SYN_USB_SetupPacket pkt = {.bmRequestType = 0x80,
                               .bRequest = SYN_USB_REQ_GET_DESCRIPTOR,
                               .wValue = (SYN_USB_DESC_TYPE_CONFIGURATION << 8),
                               .wIndex = 0,
                               .wLength = 9};
    uint8_t resp[16];
    uint16_t rlen = 0;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_process_setup(&dev, &pkt, resp, &rlen));
    TEST_ASSERT_EQUAL_UINT16(9, rlen);
    TEST_ASSERT_EQUAL_MEMORY(raw_config, resp, 9);
}

void test_usb_null_checks(void)
{
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_init(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_register_class(NULL, NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_set_string_desc(NULL, 0, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_set_raw_config_desc(NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_process_setup(NULL, NULL, NULL, NULL));
    TEST_ASSERT_FALSE(syn_usb_is_configured(NULL));

    SYN_USB_Device dev;
    syn_usb_init(&dev, TEST_DEVICE_DESC);
    SYN_USB_ClassDriver dummy_cls = {0};
    dev.class_count = SYN_USB_MAX_CLASSES;
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_usb_register_class(&dev, &dummy_cls, NULL, 0));

    dev.class_count = 0;
    dev.config_buf_used = SYN_USB_MAX_CONFIG_DESC;
    uint8_t dummy_iface[5] = {0};
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_usb_register_class(&dev, &dummy_cls, dummy_iface, 5));

    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_set_string_desc(&dev, 99, dummy_iface));

    /* SET_ADDRESS 0 -> DEFAULT state */
    SYN_USB_SetupPacket pkt = {
        .bmRequestType = 0, .bRequest = SYN_USB_REQ_SET_ADDRESS, .wValue = 0};
    uint8_t resp[16];
    uint16_t rlen = 0;
    syn_usb_process_setup(&dev, &pkt, resp, &rlen);
    TEST_ASSERT_EQUAL_INT(SYN_USB_STATE_DEFAULT, dev.state);

    /* SET_CONFIGURATION 0 -> ADDRESS state */
    pkt.bRequest = SYN_USB_REQ_SET_CONFIGURATION;
    pkt.wValue = 0;
    syn_usb_process_setup(&dev, &pkt, resp, &rlen);
    TEST_ASSERT_EQUAL_INT(SYN_USB_STATE_ADDRESS, dev.state);

    /* String desc request wLength truncation & invalid index */
    uint8_t sdesc[4] = {4, 3, 'A', 0};
    syn_usb_set_string_desc(&dev, 0, sdesc);
    pkt.bRequest = SYN_USB_REQ_GET_DESCRIPTOR;
    pkt.wValue = (SYN_USB_DESC_TYPE_STRING << 8) | 0;
    pkt.wLength = 2;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_process_setup(&dev, &pkt, resp, &rlen));
    TEST_ASSERT_EQUAL_UINT16(2, rlen);

    pkt.wValue = (SYN_USB_DESC_TYPE_STRING << 8) | 10;
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_usb_process_setup(&dev, &pkt, resp, &rlen));

    /* Unknown bRequest */
    pkt.bRequest = 0xFF;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_process_setup(&dev, &pkt, resp, &rlen));
}
