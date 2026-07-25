/**
 * @file syn_vector.h
 * @brief Fixed-point Q16.16 Vector operations and signal statistics — zero heap allocation.
 * @ingroup syn_util
 */

#ifndef SYN_VECTOR_H
#define SYN_VECTOR_H

#include "../common/syn_defs.h"
#include "syn_qmath.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Elementwise vector addition out = a + b.
 */
void syn_vec_add(const q16_t *a, const q16_t *b, q16_t *out, uint16_t n);

/**
 * @brief Elementwise vector subtraction out = a - b.
 */
void syn_vec_sub(const q16_t *a, const q16_t *b, q16_t *out, uint16_t n);

/**
 * @brief Elementwise vector scalar multiplication out = v * scale.
 */
void syn_vec_scale(const q16_t *v, q16_t scale, q16_t *out, uint16_t n);

/**
 * @brief Elementwise vector clamping: min_val <= out[i] <= max_val.
 */
void syn_vec_clamp(const q16_t *v, q16_t min_val, q16_t max_val, q16_t *out, uint16_t n);

/**
 * @brief Find minimum value in vector.
 */
q16_t syn_vec_min(const q16_t *v, uint16_t n);

/**
 * @brief Find maximum value in vector.
 */
q16_t syn_vec_max(const q16_t *v, uint16_t n);

/**
 * @brief Calculate arithmetic mean (average) of vector.
 */
q16_t syn_vec_mean(const q16_t *v, uint16_t n);

/**
 * @brief Calculate variance of vector elements.
 */
q16_t syn_vec_variance(const q16_t *v, uint16_t n);

/**
 * @brief Calculate Root Mean Square (RMS) of vector.
 */
q16_t syn_vec_rms(const q16_t *v, uint16_t n);

#ifdef __cplusplus
}
#endif

#endif /* SYN_VECTOR_H */
