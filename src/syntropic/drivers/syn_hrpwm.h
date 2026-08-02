/**
 * @file syn_hrpwm.h
 * @brief High-Resolution Power & Precision Motion Control Driver.
 *
 * Provides a vendor-agnostic, zero-allocation OS interface for sub-nanosecond
 * PWM edge placement, complementary high-side/low-side gate driver control,
 * hardware dead-time insertion, phase shifting, and emergency fault trips.
 *
 * Usage:
 * @code
 *   SYN_HRPWM hrpwm;
 *   syn_hrpwm_init(&hrpwm, 0, 500000);                     // 500 kHz switching frequency
 *   syn_hrpwm_set_deadtime_ns(&hrpwm, 100, 100);          // 100 ns dead-time
 *   syn_hrpwm_set_duty_q16(&hrpwm, Q16_FROM_FLOAT(0.50f)); // 50% duty cycle
 *   syn_hrpwm_enable(&hrpwm, true);
 * @endcode
 * @ingroup syn_drivers
 */

#ifndef SYN_HRPWM_H
#define SYN_HRPWM_H

#include "../common/syn_defs.h"
#include "../port/syn_port_hrpwm.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief High-Resolution PWM handle. Caller allocates; zero heap. */
typedef struct {
    uint8_t channel;    /**< Platform channel index */
    uint32_t freq_hz;   /**< Switching frequency in Hz */
    uint16_t rise_ns;   /**< Rise dead-time in nanoseconds */
    uint16_t fall_ns;   /**< Fall dead-time in nanoseconds */
    uint16_t phase_deg; /**< Phase offset in degrees */
    bool enabled;       /**< Output state flag */
} SYN_HRPWM;

/**
 * @brief Initialize a High-Resolution PWM driver channel.
 * @param hrpwm    Handle to initialize. Must not be NULL.
 * @param channel  Platform channel index.
 * @param freq_hz  Target switching frequency in Hz.
 * @return SYN_OK on success, SYN_INVALID_PARAM on invalid argument.
 */
SYN_Status syn_hrpwm_init(SYN_HRPWM *hrpwm, uint8_t channel, uint32_t freq_hz);

/**
 * @brief Set high-resolution duty cycle ratio using Q16.16 fixed-point format.
 * @param hrpwm     Initialized HRPWM handle.
 * @param duty_q16  Duty ratio in Q16.16 (0 = 0%, 65536 = 100%).
 * @return SYN_OK on success.
 */
SYN_Status syn_hrpwm_set_duty_q16(const SYN_HRPWM *hrpwm, int32_t duty_q16);

/**
 * @brief Set high-resolution duty cycle ratio using float (0.0 to 1.0).
 * @param hrpwm       Initialized HRPWM handle.
 * @param duty_float  Duty ratio (0.0f = 0%, 1.0f = 100%).
 * @return SYN_OK on success.
 */
SYN_Status syn_hrpwm_set_duty_float(const SYN_HRPWM *hrpwm, float duty_float);

/**
 * @brief Configure complementary dead-time insertion.
 * @param hrpwm    Initialized HRPWM handle.
 * @param rise_ns  High-side rise dead-time in nanoseconds.
 * @param fall_ns  Low-side fall dead-time in nanoseconds.
 * @return SYN_OK on success.
 */
SYN_Status syn_hrpwm_set_deadtime_ns(SYN_HRPWM *hrpwm, uint16_t rise_ns, uint16_t fall_ns);

/**
 * @brief Set channel phase shift offset in degrees.
 * @param hrpwm      Initialized HRPWM handle.
 * @param phase_deg  Phase angle offset in degrees (0–360).
 * @return SYN_OK on success.
 */
SYN_Status syn_hrpwm_set_phase_deg(SYN_HRPWM *hrpwm, uint16_t phase_deg);

/**
 * @brief Bind hardware emergency fault trip protection.
 * @param hrpwm     Initialized HRPWM handle.
 * @param fault_id  Hardware fault pin/comparator ID.
 * @return SYN_OK on success.
 */
SYN_Status syn_hrpwm_bind_fault(const SYN_HRPWM *hrpwm, uint8_t fault_id);

/**
 * @brief Enable or disable complementary HRPWM gate drivers.
 * @param hrpwm   Initialized HRPWM handle.
 * @param enable  true to enable outputs, false to disable.
 * @return SYN_OK on success.
 */
SYN_Status syn_hrpwm_enable(SYN_HRPWM *hrpwm, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* SYN_HRPWM_H */
