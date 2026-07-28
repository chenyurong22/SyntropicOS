/**
 * @file syn_ioexp.h
 * @brief Generic I2C GPIO Expander Driver (MCP23017, MCP23008, PCF8574, TCA9555).
 * @ingroup syn_drivers
 */

#ifndef SYN_IOEXP_H
#define SYN_IOEXP_H

#include "../common/syn_defs.h"
#include "../drivers/syn_soft_i2c.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2C GPIO Expander IC Type.
 */
typedef enum {
    SYN_IOEXP_MCP23017 = 0, /**< 16-bit I2C GPIO Expander (Port A & B) */
    SYN_IOEXP_MCP23008 = 1, /**< 8-bit I2C GPIO Expander */
    SYN_IOEXP_PCF8574 = 2,  /**< 8-bit Pseudo-bidirectional I2C Expander */
    SYN_IOEXP_TCA9555 = 3   /**< 16-bit I2C GPIO Expander */
} SYN_IOExpType;

/**
 * @brief Generic I2C GPIO Expander Instance Context.
 */
typedef struct {
    SYN_IOExpType type;   /**< GPIO expander model type */
    SYN_SoftI2C i2c;      /**< Software I2C bus context */
    uint8_t i2c_addr;     /**< 7-bit I2C device address */
    uint8_t num_pins;     /**< Total pins (8 or 16) */
    uint16_t out_mask;    /**< Output state cache */
    uint16_t dir_mask;    /**< Direction mask (1 = input, 0 = output) */
    uint16_t pullup_mask; /**< Pull-up resistor mask */
} SYN_IOExp;

/**
 * @brief Initialize an I2C GPIO Expander instance.
 *
 * @param ioexp    Expander context.
 * @param scl      I2C SCL pin.
 * @param sda      I2C SDA pin.
 * @param i2c_addr I2C slave address (e.g. 0x20 or 0x27).
 * @param type     Expander IC type (MCP23017, MCP23008, PCF8574, TCA9555).
 * @return SYN_OK on success.
 */
SYN_Status syn_ioexp_init(SYN_IOExp *ioexp, SYN_GPIO_Pin scl, SYN_GPIO_Pin sda, uint8_t i2c_addr,
                          SYN_IOExpType type);

/**
 * @brief Set pin mode (input vs output).
 *
 * @param ioexp   Expander context.
 * @param pin     Pin index (0 to num_pins-1).
 * @param mode    Mode (SYN_GPIO_INPUT, SYN_GPIO_OUTPUT, SYN_GPIO_INPUT_PULLUP).
 */
void syn_ioexp_set_pin_mode(SYN_IOExp *ioexp, uint8_t pin, SYN_GPIO_Mode mode);

/**
 * @brief Write digital output state to a pin.
 *
 * @param ioexp Expander context.
 * @param pin   Pin index (0 to num_pins-1).
 * @param state State (SYN_GPIO_HIGH or SYN_GPIO_LOW).
 */
void syn_ioexp_write_pin(SYN_IOExp *ioexp, uint8_t pin, SYN_GPIO_State state);

/**
 * @brief Read digital input state from a pin.
 *
 * @param ioexp Expander context.
 * @param pin   Pin index (0 to num_pins-1).
 * @return SYN_GPIO_HIGH or SYN_GPIO_LOW.
 */
SYN_GPIO_State syn_ioexp_read_pin(SYN_IOExp *ioexp, uint8_t pin);

/**
 * @brief Write entire 16-bit output port mask.
 *
 * @param ioexp Expander context.
 * @param mask  16-bit port value.
 */
void syn_ioexp_write_port(SYN_IOExp *ioexp, uint16_t mask);

/**
 * @brief Read entire 16-bit input port mask.
 *
 * @param ioexp Expander context.
 * @return 16-bit port value.
 */
uint16_t syn_ioexp_read_port(SYN_IOExp *ioexp);

#ifdef __cplusplus
}
#endif

#endif /* SYN_IOEXP_H */
