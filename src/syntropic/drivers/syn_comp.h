/**
 * @file syn_comp.h
 * @brief High-Speed Rail-to-Rail Analog Comparator Driver.
 *
 * Provides a vendor-agnostic, zero-allocation OS interface for internal analog
 * comparators with configurable voltage reference thresholds (VREFINT 1.23V, DAC, or external).
 *
 * Usage:
 * @code
 *   SYN_COMP comp;
 *   syn_comp_init(&comp, 0, SYN_COMP_INV_VREFINT); // COMP1 vs 1.23V VREFINT
 *   syn_comp_enable(&comp, true);
 *   bool state = syn_comp_read(&comp);             // True if PA1 > 1.23V
 * @endcode
 * @ingroup syn_drivers
 */

#ifndef SYN_COMP_H
#define SYN_COMP_H

#include "../common/syn_defs.h"
#include "../port/syn_port_comp.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Analog Comparator handle. Caller allocates; zero heap. */
typedef struct {
    uint8_t comp_id;                /**< Hardware comparator index */
    SYN_COMP_InvertingInput inv_in; /**< Configured reference input */
    bool enabled;                   /**< Enable state flag */
} SYN_COMP;

/**
 * @brief Initialize an Analog Comparator instance.
 * @param comp     Handle to initialize. Must not be NULL.
 * @param comp_id  Comparator hardware index (0 = COMP1, 1 = COMP2).
 * @param inv_in   Reference threshold input source.
 * @return SYN_OK on success, SYN_INVALID_PARAM on invalid argument.
 */
SYN_Status syn_comp_init(SYN_COMP *comp, uint8_t comp_id, SYN_COMP_InvertingInput inv_in);

/**
 * @brief Read real-time logical output state of the comparator.
 * @param comp  Initialized comparator handle.
 * @return true if non-inverting input voltage > inverting reference voltage.
 */
bool syn_comp_read(const SYN_COMP *comp);

/**
 * @brief Enable or disable the analog comparator.
 * @param comp    Initialized comparator handle.
 * @param enable  true to enable, false to disable.
 * @return SYN_OK on success.
 */
SYN_Status syn_comp_enable(SYN_COMP *comp, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* SYN_COMP_H */
