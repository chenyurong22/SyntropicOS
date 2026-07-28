/**
 * @file main.c
 * @brief SyntropicOS 8-Position DIP Switch STM32 HAL Example.
 *
 * Demonstrates 8-position DIP switch GPIO sampling (`syn_dipswitch_read`),
 * bitmask value packing (`syn_dipswitch_get_value`), state change detection
 * (`syn_dipswitch_has_changed`), and hardware Modbus / CAN Bus node address decoding
 * using STM32 HAL GPIO drivers.
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Pin Definitions for 8-Position DIP Switch (PA0..PA7) */
#define DIP_PORT GPIOA
#define DS1_PIN  GPIO_PIN_0
#define DS2_PIN  GPIO_PIN_1
#define DS3_PIN  GPIO_PIN_2
#define DS4_PIN  GPIO_PIN_3
#define DS5_PIN  GPIO_PIN_4
#define DS6_PIN  GPIO_PIN_5
#define DS7_PIN  GPIO_PIN_6
#define DS8_PIN  GPIO_PIN_7

/* Decoded System Configuration */
typedef struct {
    uint8_t raw_bitmask;    /* Full 8-bit packed DIP switch value (0..255) */
    uint8_t node_address;   /* Decoded 6-bit Node Address (SW1..SW6 = 1..63) */
    uint8_t baud_select;    /* Decoded 2-bit Baud Rate Index (SW7..SW8) */
    uint32_t baud_rate_hz;  /* Calculated baud rate in Hz */
    bool config_changed;    /* Set true on DIP switch position change */
} System_Config;

static System_Config sys_cfg;
static SYN_DipSwitch dip_switch;

/**
 * @brief Decode 8-bit DIP switch bitmask into node address and baud rate.
 */
static void decode_dipswitch_configuration(uint8_t bitmask)
{
    sys_cfg.raw_bitmask = bitmask;

    /* Extract 6-bit Node Address from SW1..SW6 (Bits 0..5) */
    sys_cfg.node_address = bitmask & 0x3F;
    if (sys_cfg.node_address == 0) {
        sys_cfg.node_address = 1; /* Default address 1 if all switches OFF */
    }

    /* Extract 2-bit Baud Rate Selection from SW7..SW8 (Bits 6..7) */
    sys_cfg.baud_select = (bitmask >> 6) & 0x03;

    switch (sys_cfg.baud_select) {
    case 0: sys_cfg.baud_rate_hz = 9600;   break;
    case 1: sys_cfg.baud_rate_hz = 19200;  break;
    case 2: sys_cfg.baud_rate_hz = 38400;  break;
    case 3: sys_cfg.baud_rate_hz = 115200; break;
    default: sys_cfg.baud_rate_hz = 9600;  break;
    }
}

/**
 * @brief Initialize DIP Switch Driver and GPIO Input Pins.
 */
void dipswitch_app_init(void)
{
    SYN_GPIO_Pin ds_pins[8] = {
        (SYN_GPIO_Pin)DS1_PIN, (SYN_GPIO_Pin)DS2_PIN,
        (SYN_GPIO_Pin)DS3_PIN, (SYN_GPIO_Pin)DS4_PIN,
        (SYN_GPIO_Pin)DS5_PIN, (SYN_GPIO_Pin)DS6_PIN,
        (SYN_GPIO_Pin)DS7_PIN, (SYN_GPIO_Pin)DS8_PIN
    };

    /* Initialize 8 DIP switch pins with Active-Low logic (Pull-Up to 3.3V) */
    syn_dipswitch_init(&dip_switch, ds_pins, 8, true);

    /* Initial read & configuration decode */
    syn_dipswitch_read(&dip_switch);
    decode_dipswitch_configuration((uint8_t)syn_dipswitch_get_value(&dip_switch));
}

/**
 * @brief Read hardware GPIO input pins for DIP switches.
 */
static void poll_dipswitch_pins(void)
{
    uint32_t val = 0;

    for (uint8_t i = 0; i < 8; i++) {
        GPIO_PinState pin_state = HAL_GPIO_ReadPin(DIP_PORT, (uint16_t)(1U << i));

        /* Active-Low logic: ON = LOW (GND) -> set bit 1 */
        if (pin_state == GPIO_PIN_RESET) {
            val |= (1U << i);
        }
    }

    dip_switch.previous_value = dip_switch.current_value;
    dip_switch.current_value  = val;
    dip_switch.changed        = (dip_switch.current_value != dip_switch.previous_value);
}

/**
 * @brief Periodic 100ms DIP Switch Polling Task.
 */
void dipswitch_app_task_100ms(void)
{
    poll_dipswitch_pins();

    /* Step SyntropicOS DIP Switch State Machine */
    syn_dipswitch_read(&dip_switch);

    if (syn_dipswitch_has_changed(&dip_switch)) {
        uint8_t new_bitmask = (uint8_t)syn_dipswitch_get_value(&dip_switch);
        decode_dipswitch_configuration(new_bitmask);
        sys_cfg.config_changed = true;
    }
}
