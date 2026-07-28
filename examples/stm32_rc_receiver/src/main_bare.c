/**
 * @file main_bare.c
 * @brief STM32 RC Receiver & Digital ESC Example (Bare-Metal while(1) Polling Variant).
 *
 * Demonstrates 16-channel SBUS receiver decoding and DShot600 digital ESC control:
 * - USART RX byte parsing (`syn_sbus`)
 * - SBUS throttle channel to DShot ESC packet encoding (`syn_dshot`)
 */

#include <stdbool.h>
#include <stdint.h>

#include "syntropic/output/syn_dshot.h"
#include "syntropic/port/syn_port_system.h"
#include "syntropic/proto/syn_sbus.h"

static SYN_SBUS_Parser s_sbus;

static void stm32_uart_send_byte(uint8_t byte)
{
    (void)byte;
}

static void stm32_send_dshot_dma(const SYN_DShot_Packet *packet)
{
    /* Send packet->raw_frame via TIM DMA / SPI to ESC */
    (void)packet;
}

int main_bare(void)
{
    syn_sbus_init(&s_sbus);

    uint32_t last_send_ms = syn_port_get_tick_ms();

    while (1) {
        /* Simulate incoming SBUS byte stream from USART RX ISR/buffer */
        uint8_t sbus_byte = 0x0F;
        SYN_SBUS_Frame sbus_frame;
        if (syn_sbus_parse_byte(&s_sbus, sbus_byte, &sbus_frame) == SYN_OK) {
            /* If not in failsafe mode, map SBUS Channel 2 (Throttle) to DShot frame */
            if (!sbus_frame.failsafe) {
                uint16_t throttle_us = syn_sbus_raw_to_us(sbus_frame.channels[2]);
                uint16_t dshot_cmd   = syn_dshot_us_to_throttle(throttle_us);

                SYN_DShot_Packet dshot_pkt;
                if (syn_dshot_encode(dshot_cmd, false, &dshot_pkt) == SYN_OK) {
                    stm32_send_dshot_dma(&dshot_pkt);
                }
            }
        }

        uint32_t now_ms = syn_port_get_tick_ms();
        if (now_ms - last_send_ms >= 20) {
            last_send_ms = now_ms;
            stm32_uart_send_byte(0xAA);
        }
    }

    return 0;
}
