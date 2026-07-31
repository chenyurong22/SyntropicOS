/**
 * @file main.c
 * @brief SyntropicOS Piezo Buzzer Audio Tone & Melody STM32 HAL Example.
 *
 * Demonstrates non-blocking piezo buzzer initialization (`syn_buzzer_init`),
 * single-frequency beep alerts (`syn_buzzer_beep`), multi-note musical melody
 * playback (`syn_buzzer_play_pattern`), and tone progress stepping (`syn_buzzer_step`) using STM32 HAL.
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32g4xx_hal.h) */

/* Pin Definitions for Buzzer Output */
#define BUZZER_PORT GPIOA
#define BUZZER_PIN  GPIO_PIN_6

/* Musical Note Frequencies (Hz) */
#define NOTE_C5 523
#define NOTE_E5 659
#define NOTE_G5 784
#define NOTE_C6 1047

/* Success Chime Pattern (3 notes) */
static const uint16_t SUCCESS_MELODY_FREQS[3] = { NOTE_C5, NOTE_E5, NOTE_G5 };
static const uint16_t SUCCESS_MELODY_DURS[3]  = { 100,     100,     200 };

/* Dual-Tone Siren Alarm Pattern (4 notes) */
static const uint16_t ALARM_SIREN_FREQS[4] = { 1000, 2000, 1000, 2000 };
static const uint16_t ALARM_SIREN_DURS[4]  = { 200,  200,  200,  200 };

/* SyntropicOS Buzzer Driver Handle */
static SYN_Buzzer buzzer;

/* Application Audio State */
typedef struct {
    uint32_t last_step_tick;
    uint32_t last_trigger_tick;
    uint8_t  demo_stage;
} Buzzer_AppState;

static Buzzer_AppState buzzer_app = {0};

/**
 * @brief Hardware Output Callback invoked by SyntropicOS to toggle buzzer GPIO/PWM.
 */
static void update_hardware_buzzer_tone(SYN_Buzzer *buz, bool active, uint32_t freq_hz)
{
    (void)buz;
    (void)freq_hz;

    /* Write GPIO output pin (or update Timer PWM frequency register TIMx->ARR) */
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, active ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief Initialize Piezo Buzzer Driver.
 */
void buzzer_app_init(void)
{
    /* Initialize Buzzer on PA6 (Active High) */
    syn_buzzer_init(&buzzer, (SYN_GPIO_Pin)BUZZER_PIN, true);
}

/**
 * @brief Periodic 10ms Task for Buzzer Tone Stepping and Audio Demo Sequencing.
 */
void buzzer_app_task_10ms(void)
{
    uint32_t now = syn_port_get_tick_ms();

    /* 1. Step SyntropicOS Buzzer State Machine with elapsed time (10ms) */
    if ((now - buzzer_app.last_step_tick) >= 10U) {
        uint32_t dt = now - buzzer_app.last_step_tick;
        buzzer_app.last_step_tick = now;

        syn_buzzer_step(&buzzer, dt);

        /* Update physical GPIO / PWM hardware output */
        update_hardware_buzzer_tone(&buzzer, syn_buzzer_is_playing(&buzzer), 1000);
    }

    /* 2. Audio Sequence Demo State Machine (triggers a new chime every 4 seconds) */
    if ((now - buzzer_app.last_trigger_tick) >= 4000U && !syn_buzzer_is_playing(&buzzer)) {
        buzzer_app.last_trigger_tick = now;

        switch (buzzer_app.demo_stage) {
        case 0:
            /* Play Single 2.4kHz Key Click Beep for 50ms */
            syn_buzzer_beep(&buzzer, 2400, 50);
            buzzer_app.demo_stage = 1;
            break;

        case 1:
            /* Play 3-Note Success Arpeggio Chime */
            syn_buzzer_play_pattern(&buzzer, SUCCESS_MELODY_FREQS, SUCCESS_MELODY_DURS, 3);
            buzzer_app.demo_stage = 2;
            break;

        case 2:
            /* Play 4-Note Dual-Tone Siren Alarm */
            syn_buzzer_play_pattern(&buzzer, ALARM_SIREN_FREQS, ALARM_SIREN_DURS, 4);
            buzzer_app.demo_stage = 0; /* Loop back to stage 0 */
            break;

        default:
            buzzer_app.demo_stage = 0;
            break;
        }
    }
}
