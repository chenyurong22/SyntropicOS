/**
 * @file syn_transport_usb_cdc.h
 * @brief USB CDC Transport Binding Header.
 * @ingroup syn_net
 */

#ifndef SYN_TRANSPORT_USB_CDC_H
#define SYN_TRANSPORT_USB_CDC_H

#include "syntropic/drivers/syn_usb_cdc.h"
#include "syntropic/net/syn_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bind SYN_USB_CDC instance to a SYN_Transport interface.
 *
 * @param t Pointer to SYN_Transport struct to initialize.
 * @param cdc Pointer to SYN_USB_CDC instance.
 */
void syn_transport_from_usb_cdc(SYN_Transport *t, SYN_USB_CDC *cdc);

#ifdef __cplusplus
}
#endif

#endif /* SYN_TRANSPORT_USB_CDC_H */
