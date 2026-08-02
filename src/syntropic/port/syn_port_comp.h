/**
 * @file syn_port_comp.h
 * @brief Platform Port Interface for Analog Comparators.
 *
 * Defines low-level target hardware bindings for internal high-speed analog
 * comparators, threshold selection (internal VREFINT / DAC / external pin),
 * and output state reading.
 * @ingroup syn_system
 */

#ifndef SYN_PORT_COMP_H
#define SYN_PORT_COMP_H

#include "../common/syn_defs.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Inverting input selection for comparator */
typedef enum {
    SYN_COMP_INV_VREFINT_1_4 = 0, /**< 1/4 VREFINT (~0.307 V) */
    SYN_COMP_INV_VREFINT_1_2 = 1, /**< 1/2 VREFINT (~0.615 V) */
    SYN_COMP_INV_VREFINT_3_4 = 2, /**< 3/4 VREFINT (~0.923 V) */
    SYN_COMP_INV_VREFINT = 3,     /**< Full VREFINT (~1.230 V) */
    SYN_COMP_INV_DAC1_CH1 = 4,    /**< DAC1 Channel 1 output */
    SYN_COMP_INV_EXTERNAL = 5     /**< External I/O pin */
} SYN_COMP_InvertingInput;

/**
 * @brief Initialize hardware analog comparator instance.
 * @param comp_id  Comparator hardware index (0 = COMP1, 1 = COMP2).
 * @param inv_in   Inverting reference input source.
 * @return SYN_OK on success, SYN_INVALID_PARAM or SYN_ERROR on hardware error.
 */
SYN_Status syn_port_comp_init(uint8_t comp_id, SYN_COMP_InvertingInput inv_in);

/**
 * @brief Read real-time logical output state of the hardware comparator.
 * @param comp_id  Comparator hardware index.
 * @return true if non-inverting input > inverting reference, false otherwise.
 */
bool syn_port_comp_read(uint8_t comp_id);

/**
 * @brief Enable or disable hardware analog comparator.
 * @param comp_id  Comparator hardware index.
 * @param enable   true to enable comparator, false to disable.
 * @return SYN_OK on success.
 */
SYN_Status syn_port_comp_enable(uint8_t comp_id, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* SYN_PORT_COMP_H */
