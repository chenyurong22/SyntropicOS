/**
 * @file syn_buzzer.h
 * @brief Non-blocking Piezo Buzzer & Tone Generator module.
 * @ingroup syn_output
 */

#ifndef SYN_BUZZER_H
#define SYN_BUZZER_H

#include "../common/syn_defs.h"
#include "../port/syn_port_gpio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Non-blocking piezo buzzer context.
 */
typedef struct {
    SYN_GPIO_Pin pin; /**< GPIO Pin identifier */
    bool active_high; /**< True if logic HIGH activates buzzer */
    bool is_playing;  /**< True if currently outputting tone */

    uint32_t freq_hz;     /**< Current tone frequency in Hz */
    uint32_t duration_ms; /**< Current tone duration in milliseconds */
    uint32_t elapsed_ms;  /**< Time elapsed for current tone */

    const uint16_t *pattern_freqs; /**< Sequence of frequencies for melody/pattern */
    const uint16_t *pattern_durs;  /**< Sequence of durations for melody/pattern */
    size_t pattern_count;          /**< Number of notes in pattern */
    size_t pattern_index;          /**< Active note index */
} SYN_Buzzer;

/**
 * @brief Initialize a buzzer instance on a GPIO pin.
 *
 * @param buz         Buzzer context.
 * @param pin         GPIO pin identifier.
 * @param active_high True if HIGH logic turns on buzzer.
 * @return SYN_OK on success.
 */
SYN_Status syn_buzzer_init(SYN_Buzzer *buz, SYN_GPIO_Pin pin, bool active_high);

/**
 * @brief Play a single tone non-blockingly.
 *
 * @param buz         Buzzer context.
 * @param freq_hz     Frequency in Hz (0 = silent pause).
 * @param duration_ms Duration in milliseconds.
 * @return SYN_OK on success.
 */
SYN_Status syn_buzzer_beep(SYN_Buzzer *buz, uint32_t freq_hz, uint32_t duration_ms);

/**
 * @brief Play a sequence of tones (melody/pattern) non-blockingly.
 *
 * @param buz   Buzzer context.
 * @param freqs Array of note frequencies in Hz.
 * @param durs  Array of note durations in ms.
 * @param count Number of notes in the pattern.
 * @return SYN_OK on success.
 */
SYN_Status syn_buzzer_play_pattern(SYN_Buzzer *buz, const uint16_t *freqs, const uint16_t *durs,
                                   size_t count);

/**
 * @brief Stop audio output immediately.
 *
 * @param buz Buzzer context.
 */
void syn_buzzer_stop(SYN_Buzzer *buz);

/**
 * @brief Update buzzer state machine. Call periodically in main/scheduler loop.
 *
 * @param buz   Buzzer context.
 * @param dt_ms Milliseconds elapsed since last call.
 */
void syn_buzzer_step(SYN_Buzzer *buz, uint32_t dt_ms);

/**
 * @brief Check if buzzer is currently playing a tone or melody pattern.
 *
 * @param buz Buzzer context.
 * @return True if active.
 */
bool syn_buzzer_is_playing(const SYN_Buzzer *buz);

#ifdef __cplusplus
}
#endif

#endif /* SYN_BUZZER_H */
