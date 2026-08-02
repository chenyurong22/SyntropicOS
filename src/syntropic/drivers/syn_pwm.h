/**
 * @file syn_pwm.h
 * @brief Hardware PWM (Pulse Width Modulation) driver.
 *
 * Provides a high-level, object-oriented wrapper around the platform's
 * hardware PWM port interface (syn_port_pwm.h). Mirrors the pattern of syn_dac.h.
 *
 * Usage:
 * @code
 *   SYN_PWM pwm;
 *   syn_pwm_init(&pwm, 0, 1000);        // Channel 0 @ 1 kHz
 *   syn_pwm_set_duty(&pwm, 75);         // 75% duty cycle
 *   syn_pwm_enable(&pwm, true);
 * @endcode
 * @ingroup syn_drivers
 */

#ifndef SYN_PWM_H
#define SYN_PWM_H

#include "../common/syn_defs.h"
#include "../port/syn_port_pwm.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief PWM channel handle. Caller allocates; zero heap. */
typedef struct {
    uint8_t channel;  /**< Platform PWM channel index */
    uint32_t freq_hz; /**< Configured frequency in Hz */
} SYN_PWM;

/**
 * @brief Initialize a hardware PWM channel.
 * @param pwm      Handle to initialize. Must not be NULL.
 * @param channel  Platform PWM channel index.
 * @param freq_hz  Desired PWM frequency in Hz (must be > 0).
 * @return SYN_OK on success, SYN_INVALID_PARAM or SYN_ERROR on failure.
 */
SYN_Status syn_pwm_init(SYN_PWM *pwm, uint8_t channel, uint32_t freq_hz);

/**
 * @brief Set the PWM duty cycle as a percentage (0–100%).
 * @param pwm       Initialized PWM handle.
 * @param duty_pct  Duty cycle percentage (0 = 0%, 100 = 100%).
 * @return SYN_OK on success, SYN_INVALID_PARAM if handle is invalid.
 */
SYN_Status syn_pwm_set_duty(const SYN_PWM *pwm, uint8_t duty_pct);

/**
 * @brief Set the PWM duty cycle with 16-bit raw resolution (0–65535).
 * @param pwm       Initialized PWM handle.
 * @param duty_u16  16-bit duty value (0 = off, 65535 = 100%).
 * @return SYN_OK on success, SYN_INVALID_PARAM if handle is invalid.
 */
SYN_Status syn_pwm_set_duty_raw(const SYN_PWM *pwm, uint16_t duty_u16);

/**
 * @brief Change the PWM frequency at runtime.
 * @param pwm      Initialized PWM handle.
 * @param freq_hz  New frequency in Hz (must be > 0).
 * @return SYN_OK on success, SYN_INVALID_PARAM if handle is invalid.
 */
SYN_Status syn_pwm_set_freq(SYN_PWM *pwm, uint32_t freq_hz);

/**
 * @brief Enable or disable the PWM output.
 * @param pwm     Initialized PWM handle.
 * @param enable  true to enable, false to disable.
 * @return SYN_OK on success, SYN_INVALID_PARAM if handle is invalid.
 */
SYN_Status syn_pwm_enable(const SYN_PWM *pwm, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* SYN_PWM_H */
