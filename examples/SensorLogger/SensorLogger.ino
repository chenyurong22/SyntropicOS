/**
 * SyntropicOS — Data Logger & Signal Processing
 *
 * Demonstrates structured data acquisition and digital signal processing:
 *   - Dual-channel ADC driver with 4× oversampling (syn_adc)
 *   - Exponential Moving Average (EMA) noise reduction filter (syn_filter)
 *   - Running signal statistics — min, max, mean, variance (syn_signal)
 *   - Interactive Serial CLI (syn_cli)
 *   - Multi-task cooperative scheduler (syn_sched)
 *
 * Open Serial Monitor at 115200 baud. Type "stats" to view channel statistics.
 * Connect potentiometers to analog pins A0 and A1 (or leave floating).
 *
 * Documentation & Related Features:
 *   - DSP & Filters Guide:  https://outlookhazy.github.io/SyntropicOS/modules/dsp/
 *   - DSP API Ref:          https://outlookhazy.github.io/SyntropicOS/syntropic/group__syn__dsp/
 *   - ADC Driver Docs:      https://outlookhazy.github.io/SyntropicOS/modules/drivers/ (syn_adc)
 *   - Biquad & Median:      See syn_biquad.h for IIR filtering and syn_filter_median.h for spike removal
 */

#include <SyntropicOS.h>
#include <syntropic/port/syn_port_serial.h>
#include <syntropic/drivers/syn_adc.h>
#include <syntropic/dsp/syn_filter.h>
#include <syntropic/dsp/syn_signal.h>
#include <syntropic/output/syn_led.h>
#include <string.h>
#include <stdlib.h>

#define TAG "datalogger"

SYN_CLI       cli;
SYN_LED       led;

/* ADC Objects — 2 Channels */
SYN_ADC       adc[2];
static SYN_FilterEMA ema[2];
static int32_t       stats_buf[2][8];
SYN_Signal    signal_stats[2];
int16_t       raw_val[2];
int16_t       filt_val[2];

#include "tasks.h" /* Opens as second tab in Arduino IDE */

/* ── CLI Commands ─────────────────────────────────────────────────────── */

static int cmd_adc(int argc, char *argv[])
{
    if (argc < 2) {
        syn_cli_printf(&cli, "Usage: adc <0|1>\r\n");
        return 1;
    }
    int ch = atoi(argv[1]);
    if (ch < 0 || ch > 1) {
        syn_cli_printf(&cli, "Error: Channel must be 0 or 1\r\n");
        return 1;
    }
    syn_cli_printf(&cli, "A%d: raw=%d filtered=%d\r\n", ch, raw_val[ch], filt_val[ch]);
    return 0;
}

static int cmd_stats(int argc, char *argv[])
{
    (void)argc; (void)argv;
    syn_cli_printf(&cli, "--- Channel Statistics ---\r\n");
    for (int i = 0; i < 2; i++) {
        int32_t var = syn_signal_variance_q16(&signal_stats[i]);
        syn_cli_printf(&cli, "A%d: min=%ld max=%ld mean=%ld var=%ld.%03ld\r\n",
                       i,
                       (long)syn_signal_min(&signal_stats[i]),
                       (long)syn_signal_max(&signal_stats[i]),
                       (long)syn_signal_mean(&signal_stats[i]),
                       (long)(var >> 16),
                       (long)(((var & 0xFFFF) * 1000) >> 16));
    }
    return 0;
}

static int cmd_reset(int argc, char *argv[])
{
    (void)argc; (void)argv;
    for (int i = 0; i < 2; i++) {
        syn_signal_init(&signal_stats[i], stats_buf[i], 8);
    }
    syn_cli_printf(&cli, "Signal statistics reset for all channels.\r\n");
    return 0;
}

static const SYN_CLI_Command commands[] = {
    { "adc",   "Read current raw & filtered ADC value (adc 0 or adc 1)", cmd_adc },
    { "stats", "Print running signal statistics (min, max, mean, var)", cmd_stats },
    { "reset", "Reset accumulated signal statistics",                   cmd_reset },
};

/* ── Platform Hooks ───────────────────────────────────────────────────── */

extern "C" void syn_assert_failed(const char *f, int l) { (void)f; (void)l; for(;;); }

/* ── Entry Point ──────────────────────────────────────────────────────── */

void setup()
{
    syn_port_serial_init(115200);
    syn_led_init(&led, LED_BUILTIN, SYN_LED_ACTIVE_HIGH);
    syn_led_blink(&led, 500, 500);

    /* Initialize ADC channels with 4× oversampling, EMA filter, and statistics */
    for (int i = 0; i < 2; i++) {
        syn_signal_init(&signal_stats[i], stats_buf[i], 8);
        syn_filter_ema_init(&ema[i], 64); /* alpha = 64/256 = 0.25 */

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

    syn_log_init(SYN_LOG_INFO);
    SYN_LOG_I(TAG, "DataLogger Pipeline Ready");

    syn_cli_init(&cli, commands, sizeof(commands) / sizeof(commands[0]), "> ");
    syn_cli_printf(&cli, "\r\n--- SyntropicOS Data Logger Demo ---\r\n");
    syn_cli_print_prompt(&cli);

    /* Start background tasks (Tab 2) */
    sys_start_tasks();
}

void loop()
{
    sys_run_scheduler();
}
