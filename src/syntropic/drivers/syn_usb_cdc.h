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

#define SYN_USB_REQ_GET_STATUS 0x00        /**< USB Standard Request Get Status (0x00) */
#define SYN_USB_REQ_CLEAR_FEATURE 0x01     /**< USB Standard Request Clear Feature (0x01) */
#define SYN_USB_REQ_SET_FEATURE 0x03       /**< USB Standard Request Set Feature (0x03) */
#define SYN_USB_REQ_SET_ADDRESS 0x05       /**< USB Standard Request Set Address (0x05) */
#define SYN_USB_REQ_GET_DESCRIPTOR 0x06    /**< USB Standard Request Get Descriptor (0x06) */
#define SYN_USB_REQ_SET_CONFIGURATION 0x09 /**< USB Standard Request Set Configuration (0x09) */

#define SYN_USB_CDC_SET_LINE_CODING 0x20        /**< CDC Request Set Line Coding (0x20) */
#define SYN_USB_CDC_GET_LINE_CODING 0x21        /**< CDC Request Get Line Coding (0x21) */
#define SYN_USB_CDC_SET_CONTROL_LINE_STATE 0x22 /**< CDC Request Set Control Line State (0x22) */

/** USB Setup Packet. */
typedef struct {
    uint8_t bmRequestType; /**< Characteristics of request (direction, type, recipient) */
    uint8_t bRequest;      /**< Specific request code */
    uint16_t wValue;       /**< Word-sized field according to request */
    uint16_t wIndex;       /**< Word-sized field (index/interface/endpoint) */
    uint16_t wLength;      /**< Number of bytes to transfer if data stage */
} SYN_USB_SetupPacket;

/** CDC Line Coding Config (Baud rate, Stop bits, Parity, Data bits). */
typedef struct {
    uint32_t baudrate; /**< Transmission baud rate in bits per second */
    uint8_t stop_bits; /**< Stop bits setting (0=1, 1=1.5, 2=2) */
    uint8_t parity;    /**< Parity setting (0=None, 1=Odd, 2=Even) */
    uint8_t data_bits; /**< Data bits count (5, 6, 7, 8, 16) */
} SYN_USB_CDC_LineCoding;

/** USB CDC Instance. */
typedef struct {
    uint8_t ep_in;                      /**< Bulk IN Endpoint address */
    uint8_t ep_out;                     /**< Bulk OUT Endpoint address */
    uint8_t ep_cmd;                     /**< Interrupt Command Endpoint address */
    uint8_t dev_address;                /**< Assigned USB device bus address */
    bool configured;                    /**< True if USB device is in Configured state */
    SYN_USB_CDC_LineCoding line_coding; /**< Active CDC line coding configuration */
    uint8_t rx_buf[128];                /**< Internal RX ring/linear payload buffer */
    uint16_t rx_len;                    /**< Length of unread data in rx_buf */
    uint8_t tx_buf[128];                /**< Internal TX ring/linear payload buffer */
    uint16_t tx_len;                    /**< Length of pending data in tx_buf */
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
