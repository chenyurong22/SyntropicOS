/**
 * @file test_usb_host.c
 * @brief Unit tests for Zero-Heap Modular USB 2.0 Host Core and Host CDC Class Driver.
 */

#include "mocks/mock_port.h"
#include "syntropic/drivers/syn_transport_usb_host_cdc.h"
#include "syntropic/drivers/syn_usb_host.h"
#include "syntropic/drivers/syn_usb_host_cdc.h"
#include "unity/unity.h"

#include <string.h>

/* Mock 18-byte Device Descriptor */
static const uint8_t MOCK_DEV_DESC[18] = {
    0x12, 0x01, 0x00, 0x02, 0x02, 0x00, 0x00, 0x40, /* bMaxPacketSize0 = 64 */
    0xFE, 0xCA, 0xEF, 0xBE, 0x00, 0x01, 0x01, 0x02, 0x00, 0x01};

/* Mock Configuration Descriptor (75 bytes, CDC ACM) */
static const uint8_t MOCK_CFG_DESC[] = {
    0x09, 0x02, 0x4B, 0x00, 0x02, 0x01, 0x00, 0xC0, 0x32,
    /* IAD */
    0x08, 0x0B, 0x00, 0x02, 0x02, 0x02, 0x01, 0x00,
    /* Interface 0: CDC Comm */
    0x09, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00, 0x05, 0x24, 0x00, 0x10, 0x01, 0x05, 0x24,
    0x01, 0x00, 0x01, 0x04, 0x24, 0x02, 0x02, 0x05, 0x24, 0x06, 0x00, 0x01, 0x07, 0x05, 0x82, 0x03,
    0x08, 0x00, 0x10,
    /* Interface 1: CDC Data */
    0x09, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00, 0x07, 0x05, 0x01, 0x02, 0x40, 0x00, 0x00,
    0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00};

static bool probe_called = false;
static bool disconnect_called = false;

static SYN_Status test_probe_cb(void *ctx, uint8_t dev_addr, const uint8_t *iface_desc,
                                uint16_t len)
{
    (void)ctx;
    (void)dev_addr;
    (void)iface_desc;
    (void)len;
    probe_called = true;
    return SYN_OK;
}

static void test_disconnect_cb(void *ctx)
{
    (void)ctx;
    disconnect_called = true;
}

void test_usb_host_init_and_state(void)
{
    SYN_USB_Host host;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_host_init(&host));
    TEST_ASSERT_EQUAL_UINT8(SYN_USB_HOST_STATE_DISCONNECTED, host.state);
    TEST_ASSERT_FALSE(syn_usb_host_is_ready(&host));
    TEST_ASSERT_NULL(syn_usb_host_get_dev_info(&host));
}

void test_usb_host_enumeration_flow(void)
{
    mock_usb_host_reset();
    SYN_USB_Host host;
    syn_usb_host_init(&host);

    /* 1. DISCONNECTED state */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_host_process(&host));
    TEST_ASSERT_EQUAL_UINT8(SYN_USB_HOST_STATE_DISCONNECTED, host.state);

    /* 2. Device attach */
    mock_usb_host_attached = true;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_host_process(&host));
    TEST_ASSERT_EQUAL_UINT8(SYN_USB_HOST_STATE_ATTACHED, host.state);
    TEST_ASSERT_TRUE(mock_usb_host_vbus_enabled);

    /* 3. Transition to ENUMERATING */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_host_process(&host));
    TEST_ASSERT_EQUAL_UINT8(SYN_USB_HOST_STATE_ENUMERATING, host.state);

    /* 4. Drive GET_DEV8 -> inject mock 8 bytes */
    memcpy(mock_usb_host_xfer_buf, MOCK_DEV_DESC, 8);
    mock_usb_host_xfer_len = 8;
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_usb_host_process(&host)); /* Submits setup & data */

    /* 5. Process completion -> SET_ADDR */
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_usb_host_process(&host));

    /* 6. SET_ADDR completion -> GET_DEV_FULL -> inject 18 bytes */
    memcpy(mock_usb_host_xfer_buf, MOCK_DEV_DESC, 18);
    mock_usb_host_xfer_len = 18;
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_usb_host_process(&host));

    /* 7. GET_DEV_FULL completion -> GET_CFG -> inject 67 bytes */
    memcpy(mock_usb_host_xfer_buf, MOCK_CFG_DESC, sizeof(MOCK_CFG_DESC));
    mock_usb_host_xfer_len = sizeof(MOCK_CFG_DESC);
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_usb_host_process(&host));

    /* 8. GET_CFG completion -> SET_CFG */
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_usb_host_process(&host));

    /* 9. SET_CFG completion -> CLASS_PROBE -> READY */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_host_process(&host));
    TEST_ASSERT_EQUAL_UINT8(SYN_USB_HOST_STATE_READY, host.state);
    TEST_ASSERT_TRUE(syn_usb_host_is_ready(&host));

    const SYN_USB_HostDevInfo *info = syn_usb_host_get_dev_info(&host);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL_HEX16(0xCAFE, info->vid);
    TEST_ASSERT_EQUAL_HEX16(0xBEEF, info->pid);

    /* 10. Detach device */
    mock_usb_host_attached = false;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_host_process(&host));
    TEST_ASSERT_EQUAL_UINT8(SYN_USB_HOST_STATE_DISCONNECTED, host.state);
    TEST_ASSERT_FALSE(syn_usb_host_is_ready(&host));
}

void test_usb_host_class_registration_and_probing(void)
{
    SYN_USB_Host host;
    syn_usb_host_init(&host);

    probe_called = false;
    disconnect_called = false;

    SYN_USB_HostClassDriver cls = {.class_code = 0x02, /* CDC Comm */
                                   .subclass_code = 0xFF,
                                   .protocol_code = 0xFF,
                                   .ctx = NULL,
                                   .probe = test_probe_cb,
                                   .disconnected = test_disconnect_cb,
                                   .process = NULL};

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_host_register_class(&host, &cls));

    /* Drive through enumeration with CDC config descriptor */
    mock_usb_host_reset();
    mock_usb_host_attached = true;
    syn_usb_host_process(&host); /* DISCONNECTED -> ATTACHED */
    syn_usb_host_process(&host); /* ATTACHED -> ENUMERATING */

    memcpy(mock_usb_host_xfer_buf, MOCK_DEV_DESC, 8);
    mock_usb_host_xfer_len = 8;
    syn_usb_host_process(&host); /* GET_DEV8 */

    syn_usb_host_process(&host); /* SET_ADDR */

    memcpy(mock_usb_host_xfer_buf, MOCK_DEV_DESC, 18);
    mock_usb_host_xfer_len = 18;
    syn_usb_host_process(&host); /* GET_DEV_FULL */

    memcpy(mock_usb_host_xfer_buf, MOCK_CFG_DESC, sizeof(MOCK_CFG_DESC));
    mock_usb_host_xfer_len = sizeof(MOCK_CFG_DESC);
    syn_usb_host_process(&host); /* GET_CFG */

    syn_usb_host_process(&host); /* SET_CFG */

    syn_usb_host_process(&host); /* CLASS_PROBE */
    TEST_ASSERT_TRUE(probe_called);
    TEST_ASSERT_EQUAL_UINT8(SYN_USB_HOST_STATE_READY, host.state);

    /* Simulate detach */
    mock_usb_host_attached = false;
    syn_usb_host_process(&host);
    TEST_ASSERT_TRUE(disconnect_called);
    TEST_ASSERT_EQUAL_UINT8(SYN_USB_HOST_STATE_DISCONNECTED, host.state);
}

void test_usb_host_cdc_driver(void)
{
    SYN_USB_Host host;
    SYN_USB_HostCDC hcdc;
    syn_usb_host_init(&host);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_host_cdc_init(&hcdc));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_host_cdc_register(&host, &hcdc));

    /* Test write & read API */
    const char *msg = "hello host";
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_host_cdc_write(&hcdc, msg, strlen(msg)));
    TEST_ASSERT_FALSE(syn_usb_host_cdc_tx_ready(&hcdc));

    char rx_buf[32];
    size_t out_len = 0;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_usb_host_cdc_read(&hcdc, rx_buf, sizeof(rx_buf), &out_len));
    TEST_ASSERT_EQUAL(0, out_len);
    TEST_ASSERT_FALSE(syn_usb_host_cdc_rx_available(&hcdc));
}

void test_usb_host_cdc_transport_bridge(void)
{
    SYN_USB_HostCDC hcdc;
    syn_usb_host_cdc_init(&hcdc);

    SYN_Transport tr;
    syn_transport_from_usb_host_cdc(&tr, &hcdc);
    TEST_ASSERT_NOT_NULL(tr.send);
    TEST_ASSERT_NOT_NULL(tr.recv);

    uint8_t tx_data[] = {0x01, 0x02, 0x03};
    TEST_ASSERT_TRUE(syn_transport_send(&tr, tx_data, sizeof(tx_data)));

    uint8_t rx_buf[16];
    size_t rlen = 0;
    TEST_ASSERT_TRUE(syn_transport_recv(&tr, rx_buf, sizeof(rx_buf), &rlen));
    TEST_ASSERT_EQUAL(0, rlen);

    /* Test NULL transport bridge binding */
    SYN_Transport tr_null;
    syn_transport_from_usb_host_cdc(&tr_null, NULL);
    TEST_ASSERT_NULL(tr_null.send);
    TEST_ASSERT_NULL(tr_null.recv);
}

void test_usb_host_null_checks(void)
{
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_host_init(NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_host_register_class(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_host_process(NULL));
    TEST_ASSERT_FALSE(syn_usb_host_is_ready(NULL));
    TEST_ASSERT_NULL(syn_usb_host_get_dev_info(NULL));

    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_host_cdc_init(NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_host_cdc_register(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_host_cdc_write(NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_usb_host_cdc_read(NULL, NULL, 0, NULL));
    TEST_ASSERT_FALSE(syn_usb_host_cdc_rx_available(NULL));
    TEST_ASSERT_FALSE(syn_usb_host_cdc_tx_ready(NULL));
}

void run_usb_host_tests(void)
{
    RUN_TEST(test_usb_host_init_and_state);
    RUN_TEST(test_usb_host_enumeration_flow);
    RUN_TEST(test_usb_host_class_registration_and_probing);
    RUN_TEST(test_usb_host_cdc_driver);
    RUN_TEST(test_usb_host_cdc_transport_bridge);
    RUN_TEST(test_usb_host_null_checks);
}
