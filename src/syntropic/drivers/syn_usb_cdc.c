/**
 * @file syn_usb_cdc.c
 * @brief Zero-Heap USB 2.0 CDC ACM Virtual COM Port Class Engine Implementation.
 */

#include "syntropic/drivers/syn_usb_cdc.h"

#include <string.h>

SYN_Status syn_usb_cdc_init(SYN_USB_CDC *cdc)
{
    if (!cdc) {
        return SYN_INVALID_PARAM;
    }
    memset(cdc, 0, sizeof(*cdc));

    cdc->ep_in = 0x81;
    cdc->ep_out = 0x01;
    cdc->ep_cmd = 0x82;
    cdc->configured = false;

    cdc->line_coding.baudrate = 115200;
    cdc->line_coding.stop_bits = 0; /* 1 Stop Bit */
    cdc->line_coding.parity = 0;    /* None */
    cdc->line_coding.data_bits = 8;

    return SYN_OK;
}

SYN_Status syn_usb_cdc_handle_setup(SYN_USB_CDC *cdc, const SYN_USB_SetupPacket *setup,
                                    uint8_t *resp, size_t *rlen)
{
    if (!cdc || !setup || !resp || !rlen) {
        return SYN_INVALID_PARAM;
    }

    *rlen = 0;

    switch (setup->bRequest) {
    case SYN_USB_REQ_SET_ADDRESS:
        cdc->dev_address = (uint8_t)(setup->wValue & 0x7F);
        return SYN_OK;

    case SYN_USB_REQ_SET_CONFIGURATION:
        cdc->configured = (setup->wValue != 0);
        return SYN_OK;

    case SYN_USB_CDC_SET_LINE_CODING:
        /* Line coding response parsing done on data stage */
        return SYN_OK;

    case SYN_USB_CDC_GET_LINE_CODING:
        resp[0] = (uint8_t)(cdc->line_coding.baudrate & 0xFF);
        resp[1] = (uint8_t)((cdc->line_coding.baudrate >> 8) & 0xFF);
        resp[2] = (uint8_t)((cdc->line_coding.baudrate >> 16) & 0xFF);
        resp[3] = (uint8_t)((cdc->line_coding.baudrate >> 24) & 0xFF);
        resp[4] = cdc->line_coding.stop_bits;
        resp[5] = cdc->line_coding.parity;
        resp[6] = cdc->line_coding.data_bits;
        *rlen = 7;
        return SYN_OK;

    case SYN_USB_CDC_SET_CONTROL_LINE_STATE:
        return SYN_OK;

    default:
        return SYN_OK;
    }
}

SYN_Status syn_usb_cdc_write(SYN_USB_CDC *cdc, const void *data, size_t len)
{
    if (!cdc || !data || len == 0) {
        return SYN_INVALID_PARAM;
    }

    if (len > sizeof(cdc->tx_buf)) {
        len = sizeof(cdc->tx_buf);
    }

    memcpy(cdc->tx_buf, data, len);
    cdc->tx_len = (uint16_t)len;

    return SYN_OK;
}

SYN_Status syn_usb_cdc_read(SYN_USB_CDC *cdc, void *buf, size_t max_len, size_t *out_len)
{
    if (!cdc || !buf || !out_len) {
        return SYN_INVALID_PARAM;
    }

    uint16_t available = cdc->rx_len;
    if (available == 0) {
        *out_len = 0;
        return SYN_OK;
    }

    if (max_len < available) {
        available = (uint16_t)max_len;
    }

    memcpy(buf, cdc->rx_buf, available);
    *out_len = available;

    if (available < cdc->rx_len) {
        memmove(cdc->rx_buf, &cdc->rx_buf[available], cdc->rx_len - available);
        cdc->rx_len -= available;
    } else {
        cdc->rx_len = 0;
    }

    return SYN_OK;
}
