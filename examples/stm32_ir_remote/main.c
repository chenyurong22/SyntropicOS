/**
 * @file main.c
 * @brief STM32 IR Remote Control Transmit & Receive Example (syn_ir).
 *
 * Demonstrates non-blocking infrared remote control pulse decoding (NEC, Sony,
 * Samsung, RC5, RC6, Panasonic) using STM32 GPIO EXTI dual-edge interrupts, and
 * infrared transmission (syn_ir_encode_frame) via 38kHz PWM timer or software carrier bit-banging.
 */

#include "syntropic/proto/syn_ir.h"
#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "port/stm32_hal/port_stm32_hal.h"

#define IR_RX_PIN  SYN_PORT_STM32_PIN(GPIOA, GPIO_PIN_0) /* PA0 IR Receiver */
#define IR_TX_PIN  SYN_PORT_STM32_PIN(GPIOB, GPIO_PIN_8) /* PB8 IR Transmitter */

extern TIM_HandleTypeDef htim3; /* Configured for 38kHz PWM output on PB8 (Channel 3) */

/* ── Global State ───────────────────────────────────────────────────────── */

static SYN_IR_Decoder ir_decoder;
static uint32_t last_edge_us = 0;

/* ── DWT Microsecond Counter Helper ─────────────────────────────────────── */

static inline void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline uint32_t dwt_get_us(void)
{
    return DWT->CYCCNT / (SystemCoreClock / 1000000U);
}

static void delay_us(uint32_t us)
{
    uint32_t start = dwt_get_us();
    while ((dwt_get_us() - start) < us) {
        /* Busy wait microsecond delay */
    }
}

/* ── Carrier Frequency Delay Helper Table ────────────────────────────────── */

static void get_carrier_delays(uint8_t carrier_khz, uint8_t *high_us, uint8_t *low_us)
{
    switch (carrier_khz) {
    case 36:
        *high_us = 14; *low_us = 14; break; /* ~35.7 kHz */
    case 37:
        *high_us = 13; *low_us = 14; break; /* ~37.0 kHz */
    case 38:
        *high_us = 13; *low_us = 13; break; /* ~38.4 kHz (Standard NEC/Sony) */
    case 40:
        *high_us = 12; *low_us = 13; break; /* ~40.0 kHz */
    case 56:
        *high_us = 9;  *low_us = 9;  break; /* ~55.5 kHz (Panasonic/Denon) */
    default:
        *high_us = 13; *low_us = 13; break;
    }
}

/* ── IR Transmit Function (Software Bit-Banging Carrier) ─────────────────── */

/**
 * @brief Transmit an IR frame using software bit-banging modulation.
 */
void send_ir_bitbang(SYN_IR_Protocol proto, uint32_t addr, uint32_t cmd)
{
    SYN_IR_Frame tx_frame = { .protocol = proto, .address = addr, .command = cmd, .is_repeat = false };
    SYN_IR_Pulse pulses[128];
    size_t pulse_count = 0;

    uint8_t high_us, low_us;
    uint8_t carrier_khz = syn_ir_protocol_carrier_khz(proto);
    get_carrier_delays(carrier_khz, &high_us, &low_us);
    uint32_t period_us = high_us + low_us;

    if (syn_ir_encode_frame(&tx_frame, pulses, 128, &pulse_count) == SYN_OK) {
        printf("[IR TX] Transmitting %s (Addr=0x%04X, Cmd=0x%04X, Carrier=%u kHz)...\n",
               syn_ir_protocol_name(proto), addr, cmd, carrier_khz);

        for (size_t i = 0; i < pulse_count; i++) {
            if (pulses[i].is_mark) {
                /* Generate modulation carrier (13us HIGH, 13us LOW) for duration_us */
                for (uint32_t elapsed = 0; elapsed < pulses[i].duration_us; elapsed += period_us) {
                    HAL_GPIO_WritePin(IR_TX_PORT, IR_TX_PIN, GPIO_PIN_SET);
                    delay_us(high_us);
                    HAL_GPIO_WritePin(IR_TX_PORT, IR_TX_PIN, GPIO_PIN_RESET);
                    delay_us(low_us);
                }
            } else {
                /* Space (Carrier OFF): hold LOW for duration_us */
                HAL_GPIO_WritePin(IR_TX_PORT, IR_TX_PIN, GPIO_PIN_RESET);
                delay_us(pulses[i].duration_us);
            }
        }
        HAL_GPIO_WritePin(IR_TX_PORT, IR_TX_PIN, GPIO_PIN_RESET); /* Ensure IR LED is OFF */
    }
}

/* ── IR Transmit Function (Hardware PWM Timer Modulation) ───────────────── */

/**
 * @brief Transmit an IR frame using STM32 Hardware PWM Timer (TIM3 Channel 3).
 */
void send_ir_pwm(SYN_IR_Protocol proto, uint32_t addr, uint32_t cmd)
{
    SYN_IR_Frame tx_frame = { .protocol = proto, .address = addr, .command = cmd, .is_repeat = false };
    SYN_IR_Pulse pulses[128];
    size_t pulse_count = 0;

    if (syn_ir_encode_frame(&tx_frame, pulses, 128, &pulse_count) == SYN_OK) {
        for (size_t i = 0; i < pulse_count; i++) {
            if (pulses[i].is_mark) {
                HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3); /* Enable 38kHz PWM carrier */
                delay_us(pulses[i].duration_us);
            } else {
                HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);  /* Disable PWM carrier */
                delay_us(pulses[i].duration_us);
            }
        }
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3); /* Ensure IR LED is OFF */
    }
}

/* ── GPIO EXTI Dual-Edge Interrupt Callback (Receive) ────────────────────── */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == IR_RX_PIN) {
        uint32_t now_us = dwt_get_us();
        uint32_t duration_us = now_us - last_edge_us;
        last_edge_us = now_us;

        /* Active-Low TSOP Receiver: LOW = Mark (Carrier ON), HIGH = Space (Carrier OFF) */
        bool is_mark = (syn_gpio_read(IR_RX_PIN) == SYN_GPIO_LOW);

        SYN_IR_Frame frame;
        if (syn_ir_decode_pulse(&ir_decoder, (uint16_t)duration_us, is_mark, &frame)) {
            printf("[IR RX] Decoded Frame: Protocol=%d (%s), Addr=0x%04X, Cmd=0x%04X, Repeat=%s\n",
                   frame.protocol, syn_ir_protocol_name(frame.protocol),
                   frame.address, frame.command,
                   frame.is_repeat ? "YES" : "NO");
        }
    }
}

/* ── IR Remote Processing Task ─────────────────────────────────────────── */

static SYN_PT_Status ir_task(SYN_PT *pt, SYN_Task *task)
{
    static uint32_t counter = 0;

    PT_BEGIN(pt);

    for (;;) {
        /* Periodically transmit an NEC IR Power Toggle command every 5000ms */
        PT_TASK_DELAY_MS(pt, task, 5000);

        counter++;
        send_ir_bitbang(SYN_IR_PROTO_NEC, 0x00FF, 0x0045); /* NEC Power Command */
    }

    PT_END(pt);
}

/* ── Main Application Entry Point ───────────────────────────────────────── */

int main(void)
{
    HAL_Init();
    /* MCU Clock, USART2 (115200 8N1), and TIM3 (38kHz PWM) initialization here */

    /* Initialize DWT microsecond cycle counter */
    dwt_init();

    /* Initialize SyntropicOS IR decoder state machine */
    syn_ir_decoder_init(&ir_decoder);

    printf("[IR Transceiver] Ready. Receiving on PA0, Transmitting on PB8.\n");

    /* Create scheduler tasks */
    static SYN_Task tasks[1];
    static SYN_Sched sched;

    syn_task_create(&tasks[0], "ir_task", ir_task, 0, NULL);
    syn_sched_init(&sched, tasks, 1);

    /* Run cooperative OS kernel */
    syn_sched_run_forever(&sched);
}
