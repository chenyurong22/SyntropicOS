/**
 * @file syn_hpclock.h
 * @brief High-precision clock — 64-bit system-clock-precision timestamps.
 *
 * Provides a zero-overhead timestamp capture primitive and deferred
 * resolution.  The capture is a macro that performs four volatile reads
 * with no branching — suitable for use inside ISRs where every cycle
 * counts.  Resolution is a pure function called lazily in main context.
 *
 * ## Design philosophy
 *
 * Capture fast, resolve later.  The timestamp struct stores the raw
 * snapshot; the overflow ambiguity is resolved only when you ask for
 * the 64-bit tick value.  This keeps ISR latency deterministic and
 * independent of the resolution logic.
 *
 * ## Usage
 * @code
 *   // In an ISR or anywhere timing-critical:
 *   SYN_HPTimestamp ts;
 *   SYN_HPCLOCK_CAPTURE(ts);
 *
 *   // Later, in main context:
 *   uint64_t ticks = syn_hpclock_resolve(&ts);
 *   uint64_t ns    = syn_hpclock_ticks_to_ns(ticks);
 *
 *   // Elapsed time between two events:
 *   uint64_t dt_ticks = syn_hpclock_elapsed(&ts_start, &ts_end);
 * @endcode
 * @ingroup syn_drivers
 */

#ifndef SYN_HPCLOCK_H
#define SYN_HPCLOCK_H

#if __has_include("syn_config.h")
  #include "syn_config.h"
#endif

#if !defined(SYN_USE_HPCLOCK) || SYN_USE_HPCLOCK

#include "../common/syn_defs.h"
#include "../common/syn_compiler.h"
#include "../port/syn_port_hpclock.h"

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Timestamp ─────────────────────────────────────────────────────────── */

/**
 * @brief Raw high-precision timestamp — four-word snapshot.
 *
 * Stores two pairs of (LSB, MSB) reads.  The first LSB read is the
 * "true" captured instant; the second pair is used during resolution
 * to detect and correct for a timer overflow that may have occurred
 * between the reads.
 *
 * 16 bytes, naturally aligned.
 */
typedef struct {
    uint32_t lsb_1;   /**< First read of hardware counter register    */
    uint32_t msb_1;   /**< First read of overflow counter             */
    uint32_t lsb_2;   /**< Second read of hardware counter register   */
    uint32_t msb_2;   /**< Second read of overflow counter            */
} SYN_HPTimestamp;

/** @brief Static initializer for SYN_HPTimestamp (all zeros). */
#define SYN_HPTIMESTAMP_INIT  { 0, 0, 0, 0 }

/* ── Capture macro ─────────────────────────────────────────────────────── */

/**
 * @brief Snapshot the high-precision clock into a timestamp struct.
 *
 * Four volatile reads, no branching, no function call overhead.
 * Safe to call from ISR context.  Compiler barriers between the
 * LSB (peripheral bus) and MSB (SRAM) reads prevent reordering.
 *
 * @param ts  An SYN_HPTimestamp lvalue (not a pointer).
 */
#define SYN_HPCLOCK_CAPTURE(ts) do {                            \
    volatile uint32_t *syn_hp_lsb_p_ = syn_port_hpclock_lsb_ptr(); \
    (ts).lsb_1 = *syn_hp_lsb_p_;                               \
    SYN_COMPILER_BARRIER();                                     \
    (ts).msb_1 = syn_hpclock_msb;                               \
    (ts).lsb_2 = *syn_hp_lsb_p_;                               \
    SYN_COMPILER_BARRIER();                                     \
    (ts).msb_2 = syn_hpclock_msb;                               \
} while (0)

/* ── Resolution ────────────────────────────────────────────────────────── */

/**
 * @brief Resolve a raw timestamp into a 64-bit tick count.
 *
 * Detects whether a timer overflow occurred between the two LSB reads
 * by checking monotonicity: if lsb_1 < lsb_2, no overflow and msb_2
 * is correct; if lsb_1 >= lsb_2, the counter wrapped and msb_2 is
 * decremented by one.  The result is (corrected_msb << 32) | lsb_1.
 *
 * This is a pure function — no side effects, no hardware access.
 *
 * @param ts  Pointer to a captured timestamp.
 * @return 64-bit tick count at system clock precision.
 */
uint64_t syn_hpclock_resolve(const SYN_HPTimestamp *ts);

/**
 * @brief Convert a tick count to nanoseconds.
 *
 * Uses integer-only arithmetic: ns = ticks * 1000000000 / freq_hz.
 * The division is performed with 64-bit precision to avoid overflow.
 *
 * @param ticks  Tick count from syn_hpclock_resolve().
 * @return Equivalent time in nanoseconds.
 */
uint64_t syn_hpclock_ticks_to_ns(uint64_t ticks);

/**
 * @brief Compute elapsed ticks between two timestamps.
 *
 * Resolves both timestamps and returns the difference.
 * Assumes @p end was captured after @p start.
 *
 * @param start  Earlier timestamp.
 * @param end    Later timestamp.
 * @return Elapsed ticks (end - start).
 */
uint64_t syn_hpclock_elapsed(const SYN_HPTimestamp *start,
                              const SYN_HPTimestamp *end);

/**
 * @brief Check if a timestamp is zero (uninitialized).
 *
 * @param ts  Timestamp to check.
 * @return true if all fields are zero.
 */
static inline bool syn_hpclock_is_zero(const SYN_HPTimestamp *ts)
{
    return (ts->lsb_1 == 0) && (ts->msb_1 == 0) &&
           (ts->lsb_2 == 0) && (ts->msb_2 == 0);
}

#ifdef __cplusplus
}
#endif

#endif /* SYN_USE_HPCLOCK */

#endif /* SYN_HPCLOCK_H */
