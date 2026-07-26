/**
 * @file main.c
 * @brief STM32 IR Remote Control Receiver Example (syn_ir).
 *
 * Demonstrates non-blocking infrared remote control pulse decoding (NEC, Sony,
 * Samsung, RC5, RC6, Panasonic) using STM32 GPIO EXTI dual-edge interrupts
 * and a DWT microsecond pulse timer.
 */

#include "syntropic/proto/syn_ir.h"
#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define IR_RX_PORT GPIOA
#define IR_RX_PIN  GPIO_PIN_0

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

/* ── GPIO EXTI Dual-Edge Interrupt Callback ──────────────────────────────── */

/**
 * @brief Called on BOTH Rising and Falling edges of PA0 (IR Receiver Pin).
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == IR_RX_PIN) {
        uint32_t now_us = dwt_get_us();
        uint32_t duration_us = now_us - last_edge_us;
        last_edge_us = now_us;

        /* TSOP IR receivers are Active-Low:
         * LOW (RESET = 0V) -> 38kHz Carrier Present (MARK)
         * HIGH (SET = 3.3V) -> Space / Idle (SPACE)
         */
        bool is_mark = (HAL_GPIO_ReadPin(IR_RX_PORT, IR_RX_PIN) == GPIO_PIN_RESET);

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
    PT_BEGIN(pt);

    for (;;) {
        /* Task checks system watchdog or periodic state */
        PT_TASK_DELAY_MS(pt, task, 500);
    }

    PT_END(pt);
}

/* ── Main Application Entry Point ───────────────────────────────────────── */

int main(void)
{
    HAL_Init();
    /* MCU Clock & USART2 (115200 8N1) initialization here */

    /* Initialize DWT microsecond cycle counter */
    dwt_init();

    /* Initialize SyntropicOS IR decoder state machine */
    syn_ir_decoder_init(&ir_decoder);

    printf("[IR RX] STM32 IR Receiver ready. Point an NEC/Sony/Samsung remote at PA0.\n");

    /* Create scheduler tasks */
    static SYN_Task tasks[1];
    static SYN_Sched sched;

    syn_task_create(&tasks[0], "ir_task", ir_task, 0, NULL);
    syn_sched_init(&sched, tasks, 1);

    /* Run cooperative OS kernel */
    syn_sched_run_forever(&sched);
}
