/**
 * @file board_evt.c
 * @brief Board Support Package (BSP) for WCH CH32V307V-EVT-R1 Evaluation Board.
 *
 * Full board hardware initialization for all peripherals:
 *   1. System Clock: 8 MHz HSE crystal -> PLL -> 144 MHz SysClk
 *   2. Console UART1: PA9 (TX) / PA10 (RX) alternate function pinmuxing
 *   3. USB Device & Host: PA8 VBUS power control & PA11/PA12 transceiver
 *   4. Ethernet 10/100M: EVT-R1 internal 10M PHY selection & MAC setup
 *   5. CAN1 Bus: PB8 (RX) / PB9 (TX) transceiver pinmuxing
 *   6. SPI1 Master: PA4 (CS), PA5 (SCK), PA6 (MISO), PA7 (MOSI)
 *   7. I2C1 Master: PB6 (SCL) & PB7 (SDA)
 *   8. ADC1: PA1 (Channel 1 Analog Input)
 *   9. Onboard Peripherals: LED1 (PA15), LED2 (PB4), KEY1 (PA0)
 */

#include "board_evt.h"
#include "syntropic/port/syn_port_adc.h"
#include "syntropic/port/syn_port_can.h"
#include "port/syn_port_eth.h"
#include "syntropic/port/syn_port_gpio.h"
#include "syntropic/port/syn_port_i2c.h"
#include "syntropic/port/syn_port_spi.h"
#include "syntropic/port/syn_port_system.h"
#include "syntropic/port/syn_port_uart.h"
#include "port/syn_port_usb.h"
#include "port/syn_port_usb_host.h"


/* ═══════════════════════════════════════════════════════════════════════════
 *  Board Hardware Register Definitions
 * ═══════════════════════════════════════════════════════════════════════════ */

#define RCC_BASE      0x40021000UL
#define RCC_CTLR      (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR0     (*(volatile uint32_t *)(RCC_BASE + 0x04))
#define RCC_APB2PCENR (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define RCC_APB1PCENR (*(volatile uint32_t *)(RCC_BASE + 0x1C))

#define FLASH_ACTLR   (*(volatile uint32_t *)(0x40022000UL + 0x00))

#define AFIO_BASE     0x40010000UL
#define AFIO_PCFR1    (*(volatile uint32_t *)(AFIO_BASE + 0x04))

#define GPIOA_BASE    0x40010800UL
#define GPIOA_CFGLR   (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_CFGHR   (*(volatile uint32_t *)(GPIOA_BASE + 0x04))

#define GPIOB_BASE    0x40010C00UL
#define GPIOB_CFGLR   (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_CFGHR   (*(volatile uint32_t *)(GPIOB_BASE + 0x04))

/* ═══════════════════════════════════════════════════════════════════════════
 *  BSP Subsystem Initialization Routines
 * ═══════════════════════════════════════════════════════════════════════════ */

static void board_clock_init(void)
{
    /* 1. Flash Latency: 2 Wait States for 144 MHz core clock */
    FLASH_ACTLR = 0x02;

    /* 2. Enable HSE (8 MHz External Crystal on EVT-R1 board) */
    RCC_CTLR |= (1U << 16);
    while (!(RCC_CTLR & (1U << 17))) {
        /* Wait for HSERDY */
    }

    /* 3. Configure PLL: HSE source, 18x multiplier (8 MHz * 18 = 144 MHz) */
    RCC_CFGR0 = (RCC_CFGR0 & ~((0x0FU << 18) | (1U << 16))) | (1U << 16) | (0x0FU << 18);

    /* 4. Enable PLL */
    RCC_CTLR |= (1U << 24);
    while (!(RCC_CTLR & (1U << 25))) {
        /* Wait for PLLRDY */
    }

    /* 5. Switch System Clock source to PLL */
    RCC_CFGR0 = (RCC_CFGR0 & ~3U) | 2U;
    while ((RCC_CFGR0 & 0x0CU) != 0x08U) {
        /* Wait for PLL to be used as SysClk */
    }
}

static void board_pinmux_init(void)
{
    /* Enable AFIO, GPIOA, and GPIOB peripheral clocks */
    RCC_APB2PCENR |= (1U << 0) | (1U << 2) | (1U << 3);

    /* Configure PA9 (USART1_TX, Alt Output PP 0x0B) & PA10 (USART1_RX, Input Float 0x04) */
    GPIOA_CFGHR = (GPIOA_CFGHR & ~0x00000FF0U) | 0x00000B40U;
}

void board_usb_init(void)
{
    /* Configure PA8 as Push-Pull output for USB VBUS 5V power enable on EVT-R1 */
    syn_port_gpio_init(BOARD_USB_VBUS_PIN, SYN_GPIO_OUTPUT);
    syn_port_gpio_write(BOARD_USB_VBUS_PIN, SYN_GPIO_HIGH);

    /* Initialize low-level USB controller */
    syn_port_usb_init();
    syn_port_usb_host_init();
    syn_port_usb_host_vbus(true);
}

void board_eth_init(void)
{
    /* Configure AFIO for CH32V307 Internal 10M PHY selection (Bit 23 of AFIO_PCFR1) */
    AFIO_PCFR1 |= (1U << 23);

    /* Initialize Ethernet MAC with board MAC address */
    static const uint8_t evt_mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    syn_port_eth_init(evt_mac);
}

void board_can_init(void)
{
    /* Remap CAN1 to PB8 (RX) / PB9 (TX) on EVT-R1 header */
    AFIO_PCFR1 = (AFIO_PCFR1 & ~(3U << 13)) | (2U << 13);

    syn_port_gpio_init(BOARD_CAN1_TX_PIN, SYN_GPIO_OUTPUT);
    syn_port_gpio_init(BOARD_CAN1_RX_PIN, SYN_GPIO_INPUT);

    syn_port_can_init(0, 500000U); /* 500 kbps CAN bus speed */
}

void board_spi_init(void)
{
    /* Configure PA4 (CS), PA5 (SCK), PA6 (MISO), PA7 (MOSI) */
    syn_port_gpio_init(BOARD_SPI1_CS_PIN, SYN_GPIO_OUTPUT);
    syn_port_gpio_write(BOARD_SPI1_CS_PIN, SYN_GPIO_HIGH);

    syn_port_gpio_init(BOARD_SPI1_SCK_PIN, SYN_GPIO_OUTPUT);
    syn_port_gpio_init(BOARD_SPI1_MOSI_PIN, SYN_GPIO_OUTPUT);
    syn_port_gpio_init(BOARD_SPI1_MISO_PIN, SYN_GPIO_INPUT);

    syn_port_spi_init(0, 10000000U, 0, 0); /* 10 MHz SPI Master Mode 0 */
}

void board_i2c_init(void)
{
    /* Configure PB6 (SCL) & PB7 (SDA) for I2C1 Master Mode */
    syn_port_gpio_init(BOARD_I2C1_SCL_PIN, SYN_GPIO_OUTPUT);
    syn_port_gpio_init(BOARD_I2C1_SDA_PIN, SYN_GPIO_OUTPUT);

    syn_port_i2c_init(0, 400000U, 0, 0x00); /* 400 kHz Fast-Mode I2C Master */
}

void board_adc_init(void)
{
    /* Configure PA1 as Analog Input for ADC1 Channel 1 */
    syn_port_gpio_init(SYN_GPIO_PIN(0, 1), SYN_GPIO_INPUT);
    syn_port_adc_init(0, (1U << 1));
}

void board_init(void)
{
    /* 1. System Clock: 8 MHz HSE crystal -> 144 MHz SysClk */
    board_clock_init();

    /* 2. AFIO & Console UART Pinmuxing */
    board_pinmux_init();
    syn_port_uart_init(BOARD_CONSOLE_UART, BOARD_CONSOLE_BAUDRATE);

    /* 3. Onboard LEDs & Button */
    syn_port_gpio_init(BOARD_LED1_PIN, SYN_GPIO_OUTPUT);
    syn_port_gpio_init(BOARD_LED2_PIN, SYN_GPIO_OUTPUT);
    syn_port_gpio_write(BOARD_LED1_PIN, SYN_GPIO_HIGH);
    syn_port_gpio_write(BOARD_LED2_PIN, SYN_GPIO_HIGH);
    syn_port_gpio_init(BOARD_KEY1_PIN, SYN_GPIO_INPUT_PULLDOWN);

    /* 4. Peripheral Board Setups */
    board_usb_init();
    board_eth_init();
    board_can_init();
    board_spi_init();
    board_i2c_init();
    board_adc_init();
}

void board_led_write(SYN_GPIO_Pin led_pin, SYN_GPIO_State state)
{
    syn_port_gpio_write(led_pin, state);
}

void board_led_toggle(SYN_GPIO_Pin led_pin)
{
    syn_port_gpio_toggle(led_pin);
}

bool board_button_read(SYN_GPIO_Pin key_pin)
{
    return syn_port_gpio_read(key_pin) == SYN_GPIO_HIGH;
}

void board_button_exti_init(SYN_GPIO_Pin key_pin)
{
    syn_port_gpio_init(key_pin, SYN_GPIO_INPUT_PULLDOWN);
    syn_port_exti_configure(key_pin, SYN_EXTI_RISING);
    syn_port_exti_enable(key_pin);
}


