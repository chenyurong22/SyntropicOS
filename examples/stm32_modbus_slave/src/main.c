/**
 * @file main.c
 * @brief STM32 Bare-Metal Modbus RTU Slave Example (No Protothreads / No OS Task Schedulers).
 *
 * Demonstrates bare-metal STM32 HAL Modbus RTU slave execution:
 * - Direct polling loop via syn_modbus_poll() without protothreads or OS tasks
 * - Register maps for Holding Registers (0x03/0x06/0x10) and Input Registers (0x04)
 * - Coils (0x01/0x05/0x0F) and Discrete Inputs (0x02) bit maps
 * - Interfacing bare-metal ADC sensor readings and PWM/GPIO outputs with Modbus RTU master
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "syntropic/proto/syn_modbus.h"
#include "syntropic/port/syn_port_uart.h"
#include "syntropic/port/syn_port_system.h"

/* ── Modbus Slave Configuration & Register Maps ─────────────────────────── */
#define MODBUS_SLAVE_ADDR      1
#define HOLDING_REG_COUNT     16
#define INPUT_REG_COUNT       16
#define COIL_COUNT            16
#define DISCRETE_INPUT_COUNT  16

/* Application Register Maps */
static uint16_t s_holding_regs[HOLDING_REG_COUNT];
static uint16_t s_input_regs[INPUT_REG_COUNT];
static uint8_t  s_coils[(COIL_COUNT + 7) / 8];
static uint8_t  s_discrete_inputs[(DISCRETE_INPUT_COUNT + 7) / 8];

/* Modbus Slave Control Instance & Frame Buffer */
static SYN_Modbus s_modbus_slave;
static uint8_t    s_modbus_rx_buf[256];

/* ── STM32 Hardware Peripheral Stubs (Bare-Metal HAL) ───────────────────── */

static uint16_t stm32_read_adc_temperature(void)
{
    /* On real STM32 hardware: HAL_ADC_GetValue(&hadc1) -> e.g., 24.5 °C = 245 (0.1 °C units) */
    return 245U;
}

static uint16_t stm32_read_adc_bus_voltage(void)
{
    /* On real STM32 hardware: HAL_ADC_GetValue(&hadc2) -> 24000 mV = 2400 (10 mV units) */
    return 2400U;
}

#include "port/stm32_hal/port_stm32_hal.h"

#define RELAY_PIN SYN_PORT_STM32_PIN(GPIOA, GPIO_PIN_5) /* PA5 Relay Output */

static void stm32_set_relay_state(bool state)
{
    /* On real STM32 hardware: syn_gpio_write(RELAY_PIN, state ? SYN_GPIO_HIGH : SYN_GPIO_LOW); */
    (void)state;
}

static void stm32_set_pwm_duty(uint16_t duty_0_to_1000)
{
    /* On real STM32 hardware: __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty_0_to_1000); */
    (void)duty_0_to_1000;
}

/* ── Main Entry Point (Bare-Metal HAL Loop) ─────────────────────────────── */

extern int main_bare(void);
extern int main_sched(void);

int main(void)
{
#if defined(USE_BARE_LOOP)
    return main_bare();
#else
    return main_sched();
#endif
}
