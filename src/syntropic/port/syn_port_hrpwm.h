/**
 * @file syn_port_hrpwm.h
 * @brief Platform Port Interface for High-Resolution PWM & Power Driver.
 *
 * Defines low-level target hardware binding for sub-nanosecond edge placement,
 * complementary gate output pairs with dead-time insertion, phase shifting,
 * and zero-latency hardware fault trip interlocks.
 * @ingroup syn_system
 */

#ifndef SYN_PORT_HRPWM_H
#define SYN_PORT_HRPWM_H

#include "../common/syn_defs.h"
#include "../util/syn_qmath.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize high-resolution PWM hardware timer channel.
 * @param channel  Platform-specific HRPWM channel index (0..5).
 * @param freq_hz  Switching frequency in Hz.
 * @return SYN_OK on success, SYN_INVALID_PARAM or SYN_ERROR on hardware error.
 */
SYN_Status syn_port_hrpwm_init(uint8_t channel, uint32_t freq_hz);

/**
 * @brief Set high-resolution duty cycle using Q16.16 fixed-point ratio.
 * @param channel   Platform-specific HRPWM channel index.
 * @param duty_q16  Q16.16 fixed point ratio (0 = 0.0, 65536 = 1.0).
 * @return SYN_OK on success.
 */
SYN_Status syn_port_hrpwm_set_duty_q16(uint8_t channel, int32_t duty_q16);

/**
 * @brief Configure complementary dead-time insertion in nanoseconds.
 * @param channel  Platform-specific HRPWM channel index.
 * @param rise_ns  High-side turn-on rise dead-time in nanoseconds.
 * @param fall_ns  Low-side turn-on fall dead-time in nanoseconds.
 * @return SYN_OK on success.
 */
SYN_Status syn_port_hrpwm_set_deadtime_ns(uint8_t channel, uint16_t rise_ns, uint16_t fall_ns);

/**
 * @brief Set phase shift angle in degrees (0–360).
 * @param channel    Platform-specific HRPWM channel index.
 * @param phase_deg  Phase offset angle in degrees.
 * @return SYN_OK on success.
 */
SYN_Status syn_port_hrpwm_set_phase_deg(uint8_t channel, uint16_t phase_deg);

/**
 * @brief Enable hardware emergency fault trip protection input.
 * @param channel   Platform-specific HRPWM channel index.
 * @param fault_id  Hardware fault pin/comparator ID.
 * @param enable    true to enable hardware trip interlock.
 * @return SYN_OK on success.
 */
SYN_Status syn_port_hrpwm_enable_fault(uint8_t channel, uint8_t fault_id, bool enable);

/**
 * @brief Enable or disable HRPWM complementary outputs.
 * @param channel  Platform-specific HRPWM channel index.
 * @param enable   true to enable complementary gate drivers, false to disable.
 * @return SYN_OK on success.
 */
SYN_Status syn_port_hrpwm_enable(uint8_t channel, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* SYN_PORT_HRPWM_H */
