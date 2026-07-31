/**
 * @file syn_transport_usb_host_cdc.c
 * @brief USB Host CDC Transport Binding Implementation.
 */

#include "syntropic/drivers/syn_transport_usb_host_cdc.h"

/**
 * @brief Internal transport send wrapper for USB Host CDC.
 * @param data Packet data buffer.
 * @param len Byte length.
 * @param ctx Context pointer (SYN_USB_HostCDC).
 * @return true on success.
 */
static bool host_cdc_transport_send(const uint8_t *data, size_t len, void *ctx)
{
    SYN_USB_HostCDC *hcdc = (SYN_USB_HostCDC *)ctx;
    if (!hcdc || !data || len == 0) {
        return false;
    }
    return (syn_usb_host_cdc_write(hcdc, data, len) == SYN_OK);
}

/**
 * @brief Internal transport receive wrapper for USB Host CDC.
 * @param data Output data buffer.
 * @param max_len Capacity.
 * @param out_len Pointer to receive read byte count.
 * @param ctx Context pointer (SYN_USB_HostCDC).
 * @return true on success.
 */
static bool host_cdc_transport_recv(uint8_t *data, size_t max_len, size_t *out_len, void *ctx)
{
    SYN_USB_HostCDC *hcdc = (SYN_USB_HostCDC *)ctx;
    if (!hcdc || !data || !out_len) {
        return false;
    }
    return (syn_usb_host_cdc_read(hcdc, data, max_len, out_len) == SYN_OK);
}

void syn_transport_from_usb_host_cdc(SYN_Transport *t, SYN_USB_HostCDC *hcdc)
{
    if (!t) {
        return;
    }
    if (!hcdc) {
        t->send = NULL;
        t->recv = NULL;
        t->ctx = NULL;
        return;
    }
    t->send = host_cdc_transport_send;
    t->recv = host_cdc_transport_recv;
    t->ctx = hcdc;
}
