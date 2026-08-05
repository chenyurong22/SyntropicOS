/**
 * @file syn_biquad.h
 * @brief Fixed-point Q16.16 Biquad filter (Direct Form I).
 * @ingroup syn_dsp
 */

#ifndef SYN_BIQUAD_H
#define SYN_BIQUAD_H

#include "../common/syn_defs.h"
#include "../util/syn_qmath.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fixed-point Q16.16 Biquad filter state (Direct Form I).
 */
typedef struct {
    q16_t b0; /**< Feedforward coefficient b0 (Q16.16)   */
    q16_t b1; /**< Feedforward coefficient b1 (Q16.16)   */
    q16_t b2; /**< Feedforward coefficient b2 (Q16.16)   */
    q16_t a1; /**< Feedback coefficient a1 (Q16.16, a0 assumed 1.0) */
    q16_t a2; /**< Feedback coefficient a2 (Q16.16)      */
    q16_t x1; /**< Input delay line x[n-1]               */
    q16_t x2; /**< Input delay line x[n-2]               */
    q16_t y1; /**< Output delay line y[n-1]              */
    q16_t y2; /**< Output delay line y[n-2]              */
} SYN_FilterBiquad;

/**
 * @brief Initialize a biquad filter with raw coefficients.
 * @param f   Filter instance.
 * @param b0  Feedforward coefficient b0 (Q16.16).
 * @param b1  Feedforward coefficient b1 (Q16.16).
 * @param b2  Feedforward coefficient b2 (Q16.16).
 * @param a1  Feedback coefficient a1 (Q16.16).
 * @param a2  Feedback coefficient a2 (Q16.16).
 */
void syn_filter_biquad_init(SYN_FilterBiquad *f, q16_t b0, q16_t b1, q16_t b2, q16_t a1, q16_t a2);

/**
 * @brief Reset biquad filter delay lines to zero.
 * @param f  Filter instance.
 */
void syn_filter_biquad_reset(SYN_FilterBiquad *f);

/**
 * @brief Process a single sample through the biquad filter.
 *
 * Uses 64-bit intermediate calculations to prevent overflows.
 *
 * @param f      Biquad filter instance.
 * @param sample Input sample in Q16.16.
 * @return Filtered output in Q16.16.
 */
q16_t syn_filter_biquad_update(SYN_FilterBiquad *f, q16_t sample);

/**
 * @brief Process a block of samples through the biquad filter.
 * @param f      Biquad filter instance.
 * @param in     Input sample array (n elements).
 * @param out    Output sample array (n elements). May alias in.
 * @param count  Number of samples to process.
 */
void syn_filter_biquad_process_block(SYN_FilterBiquad *f, const q16_t *in, q16_t *out,
                                     uint16_t count);

/**
 * @brief Initialize a biquad lowpass filter.
 *
 * Computes standard Butterworth coefficients in Q16.16.
 *
 * @param f      Filter instance.
 * @param fc     Cutoff frequency (Hz) in Q16.16.
 * @param fs     Sample rate (Hz) in Q16.16.
 */
void syn_filter_biquad_lowpass(SYN_FilterBiquad *f, q16_t fc, q16_t fs);

/**
 * @brief Initialize a biquad highpass filter.
 *
 * Computes standard Butterworth highpass coefficients in Q16.16.
 *
 * @param f      Filter instance.
 * @param fc     Cutoff frequency (Hz) in Q16.16.
 * @param fs     Sample rate (Hz) in Q16.16.
 */
void syn_filter_biquad_highpass(SYN_FilterBiquad *f, q16_t fc, q16_t fs);

/**
 * @brief Initialize a biquad bandpass filter.
 *
 * Constant-skirt-gain bandpass. Peak gain = 1.0 at center frequency.
 *
 * @param f      Filter instance.
 * @param fc     Center frequency (Hz) in Q16.16.
 * @param fs     Sample rate (Hz) in Q16.16.
 * @param q      Quality factor in Q16.16 (higher = narrower band).
 */
void syn_filter_biquad_bandpass(SYN_FilterBiquad *f, q16_t fc, q16_t fs, q16_t q);

/**
 * @brief Initialize a biquad notch (band-reject) filter.
 *
 * Rejects frequencies near fc, passes all others.
 *
 * @param f      Filter instance.
 * @param fc     Notch center frequency (Hz) in Q16.16.
 * @param fs     Sample rate (Hz) in Q16.16.
 * @param q      Quality factor in Q16.16 (higher = narrower notch).
 */
void syn_filter_biquad_notch(SYN_FilterBiquad *f, q16_t fc, q16_t fs, q16_t q);

/**
 * @brief Initialize a biquad allpass filter (modifies phase while preserving magnitude).
 * @param f   Filter instance.
 * @param fc  Corner frequency (Hz) in Q16.16.
 * @param fs  Sample rate (Hz) in Q16.16.
 * @param q   Quality factor in Q16.16.
 */
void syn_filter_biquad_allpass(SYN_FilterBiquad *f, q16_t fc, q16_t fs, q16_t q);

/**
 * @brief Initialize a biquad peaking equalizer filter (boost/cut centered at fc).
 * @param f        Filter instance.
 * @param fc       Center frequency (Hz) in Q16.16.
 * @param fs       Sample rate (Hz) in Q16.16.
 * @param gain_db  Gain in dB (Q16.16, e.g. +6.0 for boost, -6.0 for cut).
 * @param q        Quality factor in Q16.16.
 */
void syn_filter_biquad_peaking_eq(SYN_FilterBiquad *f, q16_t fc, q16_t fs, q16_t gain_db, q16_t q);

/* ── Cascaded Biquad Filter Bank ─────────────────────────────────────────── */

#define SYN_BIQUAD_CASCADE_MAX_STAGES 8 /**< Maximum allowed biquad filter stages in cascade */

/**
 * @brief Multi-stage cascaded biquad filter structure for high-order filtering.
 */
typedef struct {
    SYN_FilterBiquad stages[SYN_BIQUAD_CASCADE_MAX_STAGES]; /**< Array of biquad filter stages */
    uint8_t num_stages;                                     /**< Number of active stages */
} SYN_FilterBiquadCascade;

/**
 * @brief Initialize a cascaded biquad filter structure.
 * @param c          Cascade instance.
 * @param num_stages Number of active biquad stages (1 to SYN_BIQUAD_CASCADE_MAX_STAGES).
 * @return SYN_OK on success.
 */
SYN_Status syn_filter_biquad_cascade_init(SYN_FilterBiquadCascade *c, uint8_t num_stages);

/**
 * @brief Reset all delay lines across all stages in the biquad cascade.
 * @param c Cascade instance.
 */
void syn_filter_biquad_cascade_reset(SYN_FilterBiquadCascade *c);

/**
 * @brief Process a single sample sequentially through all active biquad stages.
 * @param c      Cascade instance.
 * @param sample Input sample in Q16.16.
 * @return Filtered output in Q16.16.
 */
q16_t syn_filter_biquad_cascade_update(SYN_FilterBiquadCascade *c, q16_t sample);

/**
 * @brief Process a block of samples through all active biquad stages.
 * @param c     Cascade instance.
 * @param in    Input sample array.
 * @param out   Output sample array (may alias in).
 * @param count Number of samples to process.
 */
void syn_filter_biquad_cascade_process_block(SYN_FilterBiquadCascade *c, const q16_t *in,
                                             q16_t *out, uint16_t count);

#ifdef __cplusplus
}
#endif

#endif /* SYN_BIQUAD_H */
