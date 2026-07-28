/**
 * @file main_bare.c
 * @brief SensorLogger Example (Bare-Metal while(1) Polling Loop Variant).
 *
 * Demonstrates sensor data acquisition, EMA filtering, and statistics
 * running in a direct polling loop with manual tick timestamp checking.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "syntropic/drivers/syn_adc.h"
#include "syntropic/dsp/syn_filter.h"
#include "syntropic/dsp/syn_signal.h"
#include "syntropic/output/syn_led.h"
#include "syntropic/port/syn_port_serial.h"
#include "syntropic/port/syn_port_system.h"
#include "syntropic/cli/syn_cli.h"

static SYN_CLI       cli;
static SYN_LED       led;
static SYN_ADC       adc[2];
static SYN_FilterEMA ema[2];
static int32_t       stats_buf[2][8];
static SYN_Signal    signal_stats[2];
static int16_t       raw_val[2];
static int16_t       filt_val[2];

int main_bare(void)
{
    syn_port_serial_init(115200);
    syn_led_init(&led, 13, SYN_LED_ACTIVE_HIGH);

    for (int i = 0; i < 2; i++) {
        syn_signal_init(&signal_stats[i], stats_buf[i], 8);
        syn_filter_ema_init(&ema[i], 64);

        SYN_ADC_Config cfg = {
            .channel         = (uint8_t)i,
            .oversample      = 4,
            .filter          = &ema[i],
            .filter_type     = SYN_ADC_FILTER_EMA,
            .cal_offset      = 0,
            .cal_scale       = 1,
            .cal_scale_shift = 0
        };
        syn_adc_init(&adc[i], &cfg);
        syn_adc_set_stats(&adc[i], &signal_stats[i]);
    }

    uint32_t last_led_ms = syn_port_get_tick_ms();
    uint32_t last_adc_ms = syn_port_get_tick_ms();

    while (1) {
        uint32_t now = syn_port_get_tick_ms();

        /* LED Update (50ms) */
        if (now - last_led_ms >= 50) {
            last_led_ms = now;
            syn_led_update(&led);
        }

        /* ADC Update (200ms) */
        if (now - last_adc_ms >= 200) {
            last_adc_ms = now;
            for (int i = 0; i < 2; i++) {
                syn_adc_read(&adc[i]);
                raw_val[i]  = (int16_t)syn_adc_raw(&adc[i]);
                filt_val[i] = (int16_t)syn_adc_filtered(&adc[i]);
            }
        }

        /* UART CLI Ingestion */
        uint8_t ch;
        if (syn_port_serial_read(&ch, 1) > 0) {
            syn_cli_process_char(&cli, (char)ch);
        }
    }

    return 0;
}
