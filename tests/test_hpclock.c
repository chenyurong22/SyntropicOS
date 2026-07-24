/**
 * @file test_hpclock.c
 * @brief Unity tests for syn_hpclock — capture, resolve, conversion, elapsed.
 */

#include "unity/unity.h"
#include "mocks/mock_port.h"
#include "syntropic/syntropic.h"
#include "syntropic/drivers/syn_hpclock.h"

/* ── Simulated hardware ────────────────────────────────────────────────── */

static volatile uint32_t fake_timer_cnt;

volatile uint32_t *syn_port_hpclock_lsb_ptr(void)
{
    return &fake_timer_cnt;
}

uint32_t syn_port_hpclock_freq_hz(void)
{
    return 16000000UL;  /* 16 MHz — typical AVR/ARM clock */
}

/* ── Tests ─────────────────────────────────────────────────────────────── */

static void test_hpclock_resolve_no_overflow(void)
{
    /* Simulate: counter at 1000, no overflow between reads */
    SYN_HPTimestamp ts;
    ts.lsb_1 = 1000;
    ts.msb_1 = 5;
    ts.lsb_2 = 1020;   /* lsb_2 > lsb_1 → no wrap */
    ts.msb_2 = 5;

    uint64_t ticks = syn_hpclock_resolve(&ts);

    /* Expected: (5 << 32) | 1000 */
    uint64_t expected = ((uint64_t)5 << 32) | 1000ULL;
    TEST_ASSERT_EQUAL_UINT64(expected, ticks);
}

static void test_hpclock_resolve_with_overflow(void)
{
    /* Simulate: counter wrapped between lsb_1 and lsb_2 reads */
    SYN_HPTimestamp ts;
    ts.lsb_1 = 0xFFFFFFF0;   /* near top of counter */
    ts.msb_1 = 5;
    ts.lsb_2 = 0x00000010;   /* wrapped around past zero */
    ts.msb_2 = 6;             /* overflow ISR incremented msb */

    uint64_t ticks = syn_hpclock_resolve(&ts);

    /* lsb_1 >= lsb_2 → overflow detected → use msb_2-1 = 5 */
    uint64_t expected = ((uint64_t)5 << 32) | 0xFFFFFFF0ULL;
    TEST_ASSERT_EQUAL_UINT64(expected, ticks);
}

static void test_hpclock_resolve_exact_boundary(void)
{
    /* lsb_1 == lsb_2 → equal means no strict increase → overflow path */
    SYN_HPTimestamp ts;
    ts.lsb_1 = 5000;
    ts.msb_1 = 10;
    ts.lsb_2 = 5000;   /* exactly equal */
    ts.msb_2 = 10;

    uint64_t ticks = syn_hpclock_resolve(&ts);

    /* lsb_1 >= lsb_2 → overflow path → msb_2-1 = 9 */
    uint64_t expected = ((uint64_t)9 << 32) | 5000ULL;
    TEST_ASSERT_EQUAL_UINT64(expected, ticks);
}

static void test_hpclock_ticks_to_ns(void)
{
    /* 16 MHz → 1 tick = 62.5 ns */
    /* 16 ticks = 1000 ns = 1 µs */
    uint64_t ns = syn_hpclock_ticks_to_ns(16);
    TEST_ASSERT_EQUAL_UINT64(1000, ns);

    /* 16,000,000 ticks = 1 second = 1,000,000,000 ns */
    ns = syn_hpclock_ticks_to_ns(16000000);
    TEST_ASSERT_EQUAL_UINT64(1000000000ULL, ns);

    /* 0 ticks = 0 ns */
    ns = syn_hpclock_ticks_to_ns(0);
    TEST_ASSERT_EQUAL_UINT64(0, ns);
}

static void test_hpclock_ticks_to_ns_large(void)
{
    /* 10 seconds worth of ticks at 16 MHz */
    uint64_t ticks = 160000000ULL;
    uint64_t ns = syn_hpclock_ticks_to_ns(ticks);
    TEST_ASSERT_EQUAL_UINT64(10000000000ULL, ns);
}

static void test_hpclock_elapsed(void)
{
    SYN_HPTimestamp start, end;

    /* Start: tick 100, msb 0 */
    start.lsb_1 = 100;
    start.msb_1 = 0;
    start.lsb_2 = 110;
    start.msb_2 = 0;

    /* End: tick 500, msb 0 */
    end.lsb_1 = 500;
    end.msb_1 = 0;
    end.lsb_2 = 510;
    end.msb_2 = 0;

    uint64_t dt = syn_hpclock_elapsed(&start, &end);
    TEST_ASSERT_EQUAL_UINT64(400, dt);
}

static void test_hpclock_elapsed_across_overflow(void)
{
    SYN_HPTimestamp start, end;

    /* Start: near end of msb=2 epoch */
    start.lsb_1 = 0xFFFFFF00;
    start.msb_1 = 2;
    start.lsb_2 = 0xFFFFFF10;
    start.msb_2 = 2;

    /* End: early in msb=3 epoch */
    end.lsb_1 = 0x00000100;
    end.msb_1 = 3;
    end.lsb_2 = 0x00000110;
    end.msb_2 = 3;

    uint64_t dt = syn_hpclock_elapsed(&start, &end);

    /* start resolves to (2 << 32) | 0xFFFFFF00 */
    /* end   resolves to (3 << 32) | 0x00000100 */
    /* diff = 0x100 + (0xFFFFFFFF - 0xFFFFFF00 + 1) = 0x100 + 0x100 = 0x200 */
    TEST_ASSERT_EQUAL_UINT64(0x200, dt);
}

static void test_hpclock_capture_macro(void)
{
    /* Set up simulated hardware state */
    syn_hpclock_msb = 42;
    fake_timer_cnt = 12345;

    SYN_HPTimestamp ts;
    SYN_HPCLOCK_CAPTURE(ts);

    /* lsb reads should reflect the fake counter */
    TEST_ASSERT_EQUAL_UINT32(12345, ts.lsb_1);
    TEST_ASSERT_EQUAL_UINT32(12345, ts.lsb_2);

    /* msb reads should reflect the global overflow counter */
    TEST_ASSERT_EQUAL_UINT32(42, ts.msb_1);
    TEST_ASSERT_EQUAL_UINT32(42, ts.msb_2);
}

static void test_hpclock_is_zero(void)
{
    SYN_HPTimestamp ts = SYN_HPTIMESTAMP_INIT;
    TEST_ASSERT_TRUE(syn_hpclock_is_zero(&ts));

    ts.lsb_1 = 1;
    TEST_ASSERT_FALSE(syn_hpclock_is_zero(&ts));
}

static void test_hpclock_overflow_tick_macro(void)
{
    syn_hpclock_msb = 0;
    SYN_HPCLOCK_OVERFLOW_TICK();
    TEST_ASSERT_EQUAL_UINT32(1, syn_hpclock_msb);

    SYN_HPCLOCK_OVERFLOW_TICK();
    SYN_HPCLOCK_OVERFLOW_TICK();
    TEST_ASSERT_EQUAL_UINT32(3, syn_hpclock_msb);
}

/* ── Runner ────────────────────────────────────────────────────────────── */

void setUp(void) {
    syn_hpclock_msb = 0;
    fake_timer_cnt = 0;
}

void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_hpclock_resolve_no_overflow);
    RUN_TEST(test_hpclock_resolve_with_overflow);
    RUN_TEST(test_hpclock_resolve_exact_boundary);
    RUN_TEST(test_hpclock_ticks_to_ns);
    RUN_TEST(test_hpclock_ticks_to_ns_large);
    RUN_TEST(test_hpclock_elapsed);
    RUN_TEST(test_hpclock_elapsed_across_overflow);
    RUN_TEST(test_hpclock_capture_macro);
    RUN_TEST(test_hpclock_is_zero);
    RUN_TEST(test_hpclock_overflow_tick_macro);
    return UNITY_END();
}
