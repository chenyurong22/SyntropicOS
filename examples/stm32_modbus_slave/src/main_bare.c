/**
 * @file main_bare.c
 * @brief STM32 Bare-Metal Modbus RTU Slave Example (Bare-Metal while(1) Polling Loop).
 *
 * Demonstrates bare-metal STM32 HAL Modbus RTU slave execution:
 * - Direct polling loop via syn_modbus_poll() inside while(1) without task scheduler
 * - Manual timestamp subtraction (now_ms - last_ms >= 100) for periodic sensor register updates
 * - Holding Registers (0x03/0x06/0x10) and Input Registers (0x04) maps
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

static uint16_t s_holding_regs[HOLDING_REG_COUNT];
static uint16_t s_input_regs[INPUT_REG_COUNT];
static uint8_t  s_coils[(COIL_COUNT + 7) / 8];
static uint8_t  s_discrete_inputs[(DISCRETE_INPUT_COUNT + 7) / 8];

static SYN_Modbus s_modbus_slave;
static uint8_t    s_modbus_rx_buf[256];

static uint16_t stm32_read_adc_temperature(void)
{
    return 245U; /* 24.5 °C */
}

static uint16_t stm32_read_adc_bus_voltage(void)
{
    return 2400U; /* 24.00 V */
}

static void stm32_set_relay_state(bool state)
{
    (void)state;
}

static void stm32_set_pwm_duty(uint16_t duty_0_to_1000)
{
    (void)duty_0_to_1000;
}

/* Bare-metal entry point */
int main_bare(void)
{
    SYN_Modbus_Config cfg = {
        .slave_addr      = MODBUS_SLAVE_ADDR,
        .uart            = 0,
        .holding_regs    = s_holding_regs,
        .holding_count   = HOLDING_REG_COUNT,
        .input_regs      = s_input_regs,
        .input_count     = INPUT_REG_COUNT,
        .coils           = s_coils,
        .coils_count     = COIL_COUNT,
        .discrete_inputs = s_discrete_inputs,
        .discrete_count  = DISCRETE_INPUT_COUNT,
    };

    syn_modbus_init(&s_modbus_slave, &cfg, s_modbus_rx_buf, sizeof(s_modbus_rx_buf));

    s_holding_regs[0] = 500U;
    s_holding_regs[1] = 1152;

    uint32_t last_sensor_update_ms = syn_port_get_tick_ms();

    while (1) {
        /* Step A: Direct polling loop */
        syn_modbus_poll(&s_modbus_slave);

        /* Step B: Manual timestamp check for 100ms periodic update */
        uint32_t now_ms = syn_port_get_tick_ms();
        if (now_ms - last_sensor_update_ms >= 100u) {
            last_sensor_update_ms = now_ms;

            s_input_regs[0] = stm32_read_adc_temperature();
            s_input_regs[1] = stm32_read_adc_bus_voltage();
            s_input_regs[2] = (uint16_t)(now_ms / 1000u);
        }

        /* Step C: Apply outputs */
        bool relay_on = (s_coils[0] & 0x01) != 0;
        stm32_set_relay_state(relay_on);

        uint16_t pwm_setpoint = s_holding_regs[0];
        if (pwm_setpoint > 1000U) {
            pwm_setpoint = 1000U;
        }
        stm32_set_pwm_duty(pwm_setpoint);
    }

    return 0;
}
