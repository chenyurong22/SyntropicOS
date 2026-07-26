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

static void stm32_set_relay_state(bool state)
{
    /* On real STM32 hardware: HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET); */
    (void)state;
}

static void stm32_set_pwm_duty(uint16_t duty_0_to_1000)
{
    /* On real STM32 hardware: __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty_0_to_1000); */
    (void)duty_0_to_1000;
}

/* ── Main Entry Point (Bare-Metal HAL Loop) ─────────────────────────────── */

int main(void)
{
    /* 1. STM32 HAL Hardware Initialization (Clock, GPIO, USART1, ADC, PWM) */
    /* HAL_Init(); SystemClock_Config(); MX_GPIO_Init(); MX_USART1_UART_Init(); */

    /* 2. Configure Modbus RTU Slave Parameters */
    SYN_Modbus_Config cfg = {
        .slave_addr      = MODBUS_SLAVE_ADDR,
        .uart            = 0, /* UART instance 0 (USART1) */
        .holding_regs    = s_holding_regs,
        .holding_count   = HOLDING_REG_COUNT,
        .input_regs      = s_input_regs,
        .input_count     = INPUT_REG_COUNT,
        .coils           = s_coils,
        .coils_count     = COIL_COUNT,
        .discrete_inputs = s_discrete_inputs,
        .discrete_count  = DISCRETE_INPUT_COUNT,
    };

    /* Initialize Modbus RTU Slave Driver */
    syn_modbus_init(&s_modbus_slave, &cfg, s_modbus_rx_buf, sizeof(s_modbus_rx_buf));

    /* Initialize default Holding Register values (Setpoints) */
    s_holding_regs[0] = 500U;  /* Default PWM Duty setpoint: 50.0% (500/1000) */
    s_holding_regs[1] = 1152;  /* Default Baud Rate Code (115200) */

    uint32_t last_sensor_update_ms = syn_port_get_tick_ms();

    /* 3. Bare-Metal Main Loop (No OS Scheduler / No Protothreads) */
    while (1) {
        /* Step A: Poll & Process incoming Modbus RTU frames from UART */
        syn_modbus_poll(&s_modbus_slave);

        /* Step B: Periodically update Input Registers with fresh sensor data (every 100 ms) */
        uint32_t now_ms = syn_port_get_tick_ms();
        if (now_ms - last_sensor_update_ms >= 100u) {
            last_sensor_update_ms = now_ms;

            s_input_regs[0] = stm32_read_adc_temperature();  /* Input Reg 30001: Temperature */
            s_input_regs[1] = stm32_read_adc_bus_voltage();  /* Input Reg 30002: Bus Voltage */
            s_input_regs[2] = (uint16_t)(now_ms / 1000u);    /* Input Reg 30003: System Uptime (s) */
        }

        /* Step C: Apply Holding Register & Coil settings written by Modbus Master */
        bool relay_on = (s_coils[0] & 0x01) != 0;
        stm32_set_relay_state(relay_on);

        uint16_t pwm_setpoint = s_holding_regs[0];
        if (pwm_setpoint > 1000U) {
            pwm_setpoint = 1000U; /* Clamp max duty */
        }
        stm32_set_pwm_duty(pwm_setpoint);
    }

    return 0;
}
