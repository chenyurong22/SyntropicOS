/**
 * @file board_evt.h
 * @brief Complete Board Support Package (BSP) for WCH CH32V307V-EVT-R1 Evaluation Board.
 */

#ifndef BOARD_EVT_H
#define BOARD_EVT_H

#include "syntropic/common/syn_defs.h"
#include "syntropic/port/syn_port_exti.h"
#include "syntropic/port/syn_port_gpio.h"

#include <stdbool.h>
#include <stdint.h>

#ifndef SYN_GPIO_PIN
#define SYN_GPIO_PIN(port, pin) ((uint16_t)(((port) << 8) | (pin)))
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* ── Board Hardware Identification ──────────────────────────────────────── */

#define BOARD_NAME "CH32V307V-EVT-R1"
#define BOARD_HSE_HZ 8000000UL     /* 8 MHz External Crystal */
#define BOARD_SYSCLK_HZ 144000000UL /* 144 MHz System PLL Clock */

/* ── Console UART Pin Definitions ───────────────────────────────────────── */

#define BOARD_CONSOLE_UART 0        /* USART1 */
#define BOARD_CONSOLE_BAUDRATE 115200U
#define BOARD_UART1_TX_PIN SYN_GPIO_PIN(0, 9)  /* PA9  - USART1_TX */
#define BOARD_UART1_RX_PIN SYN_GPIO_PIN(0, 10) /* PA10 - USART1_RX */

/* ── Onboard LEDs (Active Low) ─────────────────────────────────────────── */

#define BOARD_LED1_PIN SYN_GPIO_PIN(0, 15) /* PA15 - LED1 */
#define BOARD_LED2_PIN SYN_GPIO_PIN(1, 4)  /* PB4  - LED2 */

/* ── Onboard Buttons (Active High) ─────────────────────────────────────── */

#define BOARD_KEY1_PIN SYN_GPIO_PIN(0, 0)  /* PA0  - KEY1 Wakeup Button */

/* ── SPI Pin Definitions ────────────────────────────────────────────────── */

#define BOARD_SPI1_CS_PIN   SYN_GPIO_PIN(0, 4) /* PA4 - SPI1 CS */
#define BOARD_SPI1_SCK_PIN  SYN_GPIO_PIN(0, 5) /* PA5 - SPI1 SCK */
#define BOARD_SPI1_MISO_PIN SYN_GPIO_PIN(0, 6) /* PA6 - SPI1 MISO */
#define BOARD_SPI1_MOSI_PIN SYN_GPIO_PIN(0, 7) /* PA7 - SPI1 MOSI */

/* ── I2C Pin Definitions ────────────────────────────────────────────────── */

#define BOARD_I2C1_SCL_PIN SYN_GPIO_PIN(1, 6) /* PB6 - I2C1 SCL */
#define BOARD_I2C1_SDA_PIN SYN_GPIO_PIN(1, 7) /* PB7 - I2C1 SDA */

/* ── CAN Pin Definitions ────────────────────────────────────────────────── */

#define BOARD_CAN1_RX_PIN SYN_GPIO_PIN(1, 8) /* PB8 - CAN1 RX */
#define BOARD_CAN1_TX_PIN SYN_GPIO_PIN(1, 9) /* PB9 - CAN1 TX */

/* ── USB VBUS Power Pin ─────────────────────────────────────────────────── */

#define BOARD_USB_VBUS_PIN SYN_GPIO_PIN(0, 8) /* PA8 - USB VBUS Power Control */

/* ── BSP API ────────────────────────────────────────────────────────────── */

/**
 * @brief Initialize all hardware and peripheral pinmuxing on CH32V307V-EVT-R1.
 */
void board_init(void);

/**
 * @brief Initialize USB Device & Host board pinmuxing and VBUS power.
 */
void board_usb_init(void);

/**
 * @brief Initialize Ethernet board PHY mode & MAC configuration.
 */
void board_eth_init(void);

/**
 * @brief Initialize CAN bus board transceiver pinmuxing.
 */
void board_can_init(void);

/**
 * @brief Initialize SPI master board pinmuxing.
 */
void board_spi_init(void);

/**
 * @brief Initialize I2C master board pinmuxing.
 */
void board_i2c_init(void);

/**
 * @brief Initialize ADC analog input channel pinmuxing.
 */
void board_adc_init(void);

/**
 * @brief Set onboard LED state.
 */
void board_led_write(SYN_GPIO_Pin led_pin, SYN_GPIO_State state);

/**
 * @brief Toggle onboard LED state.
 */
void board_led_toggle(SYN_GPIO_Pin led_pin);

/**
 * @brief Read onboard button state.
 */
bool board_button_read(SYN_GPIO_Pin key_pin);

/**
 * @brief Configure EXTI interrupt for onboard button.
 */
void board_button_exti_init(SYN_GPIO_Pin key_pin);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_EVT_H */

