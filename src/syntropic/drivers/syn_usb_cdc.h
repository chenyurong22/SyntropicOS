/**
 * @file syn_usb_cdc.h
 * @brief Zero-Heap USB 2.0 CDC ACM Virtual COM Port Class Engine.
 *
 * USB CDC specifications:
 * - Device Class: 0x02 (Communications), SubClass: 0x00, Protocol: 0x00.
 * - Interface 0: CDC Communication Class (Interrupt EP 0x82 / 8B).
 * - Interface 1: CDC Data Class (Bulk IN EP 0x81 / 64B, Bulk OUT EP 0x01 / 64B).
 */

#ifndef SYN_USB_CDC_H
#define SYN_USB_CDC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "syntropic/common/syn_defs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SYN_USB_REQ_GET_STATUS 0x00
#define SYN_USB_REQ_CLEAR_FEATURE 0x01
#define SYN_USB_REQ_SET_FEATURE 0x03
#define SYN_USB_REQ_SET_ADDRESS 0x05
#define SYN_USB_REQ_GET_DESCRIPTOR 0x06
#define SYN_USB_REQ_SET_CONFIGURATION 0x09

#define SYN_USB_CDC_SET_LINE_CODING 0x20
#define SYN_USB_CDC_GET_LINE_CODING 0x21
#define SYN_USB_CDC_SET_CONTROL_LINE_STATE 0x22

/** USB Setup Packet. */
typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} SYN_USB_SetupPacket;

/** CDC Line Coding Config (Baud rate, Stop bits, Parity, Data bits). */
typedef struct {
    uint32_t baudrate;
    uint8_t stop_bits;
    uint8_t parity;
    uint8_t data_bits;
} SYN_USB_CDC_LineCoding;

/** USB CDC Instance. */
typedef struct {
    uint8_t ep_in;
    uint8_t ep_out;
    uint8_t ep_cmd;
    uint8_t dev_address;
    bool configured;
    SYN_USB_CDC_LineCoding line_coding;
    uint8_t rx_buf[128];
    uint16_t rx_len;
    uint8_t tx_buf[128];
    uint16_t tx_len;
} SYN_USB_CDC;

/**
 * @brief Initialize USB CDC Class instance with defaults (115200 baud, 8N1).
 *
 * @param cdc Pointer to USB CDC instance.
 * @return SYN_OK on success.
 */
SYN_Status syn_usb_cdc_init(SYN_USB_CDC *cdc);

/**
 * @brief Handle Control Setup Request from host (EP0).
 *
 * @param cdc   Pointer to USB CDC instance.
 * @param setup Pointer to setup packet.
 * @param resp  Pointer to response buffer.
 * @param rlen  Pointer to receive response length.
 * @return SYN_OK on success.
 */
SYN_Status syn_usb_cdc_handle_setup(SYN_USB_CDC *cdc, const SYN_USB_SetupPacket *setup,
                                    uint8_t *resp, size_t *rlen);

/**
 * @brief Write bytes to USB CDC Bulk IN transmit buffer.
 *
 * @param cdc  Pointer to USB CDC instance.
 * @param data Pointer to data bytes.
 * @param len  Byte length.
 * @return SYN_OK on success.
 */
SYN_Status syn_usb_cdc_write(SYN_USB_CDC *cdc, const void *data, size_t len);

/**
 * @brief Read bytes from USB CDC Bulk OUT receive buffer.
 *
 * @param cdc     Pointer to USB CDC instance.
 * @param buf     Pointer to destination buffer.
 * @param max_len Max capacity.
 * @param out_len Pointer to receive read byte count.
 * @return SYN_OK on success.
 */
SYN_Status syn_usb_cdc_read(SYN_USB_CDC *cdc, void *buf, size_t max_len, size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif /* SYN_USB_CDC_H */
