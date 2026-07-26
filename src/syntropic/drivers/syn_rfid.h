/**
 * @file syn_rfid.h
 * @brief Generic RFID & NFC Card Reader Driver (MFRC522 13.56MHz, PN532, RDM6300 125kHz).
 * @ingroup syn_drivers
 */

#ifndef SYN_RFID_H
#define SYN_RFID_H

#include "../common/syn_defs.h"
#include "../port/syn_port_gpio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RFID Reader Type.
 */
typedef enum {
    SYN_RFID_MFRC522 = 0, /**< MFRC522 13.56MHz SPI/I2C Reader */
    SYN_RFID_PN532   = 1, /**< PN532 NFC/RFID Reader */
    SYN_RFID_RDM6300 = 2  /**< RDM6300 125kHz UART Reader */
} SYN_RFIDType;

/**
 * @brief Generic RFID Instance Context.
 */
typedef struct {
    SYN_RFIDType type;
    SYN_GPIO_Pin ss_pin;
    SYN_GPIO_Pin rst_pin;
    uint8_t uid[10];            /**< Extracted card UID byte array */
    uint8_t uid_len;            /**< UID length (4, 7, or 10 bytes) */
    bool card_present;          /**< True if card detected near antenna */
} SYN_RFID;

/**
 * @brief Initialize RFID Reader context.
 *
 * @param rfid    RFID context.
 * @param ss_pin  SPI SS / I2C SDA pin.
 * @param rst_pin Reset GPIO pin.
 * @param type    RFID IC type (MFRC522, PN532, RDM6300).
 * @return SYN_OK on success.
 */
SYN_Status syn_rfid_init(SYN_RFID *rfid, SYN_GPIO_Pin ss_pin, SYN_GPIO_Pin rst_pin, SYN_RFIDType type);

/**
 * @brief Feed raw UID scan bytes.
 *
 * @param rfid RFID context.
 * @param uid  UID byte buffer.
 * @param len  UID byte count (4, 7, or 10).
 */
void syn_rfid_feed_card(SYN_RFID *rfid, const uint8_t *uid, uint8_t len);

/**
 * @brief Clear card presence state (card removed).
 *
 * @param rfid RFID context.
 */
void syn_rfid_clear_card(SYN_RFID *rfid);

/**
 * @brief Check if a card is present.
 *
 * @param rfid RFID context.
 * @return True if card present.
 */
bool syn_rfid_is_card_present(const SYN_RFID *rfid);

/**
 * @brief Get last scanned UID byte array.
 *
 * @param rfid RFID context.
 * @param len  Pointer to receive UID length.
 * @return Pointer to UID bytes (or NULL).
 */
const uint8_t *syn_rfid_get_uid(const SYN_RFID *rfid, uint8_t *len);

#ifdef __cplusplus
}
#endif

#endif /* SYN_RFID_H */
