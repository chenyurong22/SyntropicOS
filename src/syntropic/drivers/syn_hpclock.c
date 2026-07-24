#if __has_include("syn_config.h")
  #include "syn_config.h"
#endif

#if !defined(SYN_USE_HPCLOCK) || SYN_USE_HPCLOCK

/**
 * @file syn_hpclock.c
 * @brief High-precision clock — resolution and conversion implementation.
 */

#include "syn_hpclock.h"
#include "../util/syn_assert.h"

/* ── Overflow counter (shared with port ISR) ───────────────────────────── */

volatile uint32_t syn_hpclock_msb;

/* ── Resolution ────────────────────────────────────────────────────────── */

uint64_t syn_hpclock_resolve(const SYN_HPTimestamp *ts)
{
    SYN_ASSERT(ts != NULL);

    /*
     * If lsb_1 < lsb_2 the counter did not wrap between the two
     * LSB reads, so msb_2 is the correct high word for lsb_1.
     *
     * If lsb_1 >= lsb_2 the counter wrapped (overflowed) between
     * the reads, meaning msb_2 was incremented by the overflow ISR
     * after lsb_1 was captured.  Subtract one to get the MSB that
     * was current at the instant of lsb_1.
     */
    uint32_t msb = (ts->lsb_1 < ts->lsb_2)
                    ? ts->msb_2
                    : ts->msb_2 - 1u;

    return ((uint64_t)msb << 32) | (uint64_t)ts->lsb_1;
}

/* ── Conversion ────────────────────────────────────────────────────────── */

uint64_t syn_hpclock_ticks_to_ns(uint64_t ticks)
{
    uint32_t freq_hz = syn_port_hpclock_freq_hz();
    if (freq_hz == 0) return 0;

    /*
     * ns = ticks * 1,000,000,000 / freq_hz
     *
     * To avoid overflow in the multiply, split into whole seconds
     * and remainder ticks:
     *   whole_s    = ticks / freq_hz
     *   rem_ticks  = ticks % freq_hz
     *   ns = whole_s * 1,000,000,000 + rem_ticks * 1,000,000,000 / freq_hz
     *
     * The remainder multiply fits in 64 bits as long as
     * rem_ticks < freq_hz and freq_hz < 4.29 GHz (always true for
     * any practical MCU clock).
     */
    uint64_t whole_s   = ticks / freq_hz;
    uint64_t rem_ticks = ticks % freq_hz;

    return whole_s * 1000000000ULL
         + rem_ticks * 1000000000ULL / freq_hz;
}

/* ── Elapsed ───────────────────────────────────────────────────────────── */

uint64_t syn_hpclock_elapsed(const SYN_HPTimestamp *start,
                              const SYN_HPTimestamp *end)
{
    SYN_ASSERT(start != NULL);
    SYN_ASSERT(end   != NULL);

    return syn_hpclock_resolve(end) - syn_hpclock_resolve(start);
}

#endif /* SYN_USE_HPCLOCK */
