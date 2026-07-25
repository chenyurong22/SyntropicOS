/**
 * @file test_timesync.c
 * @brief Unity tests for syn_timesync — high-precision time discipline service.
 */

#include "mocks/mock_port.h"
#include "syntropic/drivers/syn_hpclock.h"
#include "syntropic/drivers/syn_timesync.h"
#include "syntropic/syntropic.h"
#include "unity/unity.h"

/* ── Simulated Hardware Clock ──────────────────────────────────────────── */

static volatile uint32_t fake_timer_cnt;

volatile uint32_t *syn_port_hpclock_lsb_ptr(void)
{
    return &fake_timer_cnt;
}

uint32_t syn_port_hpclock_freq_hz(void)
{
    return 16000000UL; /* 16 MHz hardware timer */
}

/* ── Test Suite ────────────────────────────────────────────────────────── */

static void test_timesync_init(void)
{
    SYN_TimeSync tsync;
    syn_timesync_init(&tsync);

    TEST_ASSERT_FALSE(tsync.has_pps_lock);
    TEST_ASSERT_EQUAL_UINT32(50, tsync.base_jitter_ns);
    TEST_ASSERT_EQUAL_UINT32(60, tsync.max_holdover_s);
}

static void test_timesync_unsynced_fallback(void)
{
    SYN_TimeSync tsync;
    syn_timesync_init(&tsync);

    SYN_HPTimestamp event_ts;
    event_ts.msb_1 = 0;
    event_ts.lsb = 16000; /* 16000 ticks = 1 ms */
    event_ts.msb_2 = 0;

    SYN_UTCTimestamp utc;
    SYN_Status st = syn_timesync_resolve_utc(&tsync, &event_ts, &utc);

    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(SYN_TIME_SOURCE_UNSYNCED, utc.source);
    TEST_ASSERT_EQUAL_UINT64(0, utc.sec);
    TEST_ASSERT_EQUAL_UINT32(1000000, utc.nsec); /* 1 ms = 1,000,000 ns */
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFU, utc.uncertainty_ns);
}

static void test_timesync_locked_resolution(void)
{
    SYN_TimeSync tsync;
    syn_timesync_init(&tsync);

    /* Bind PPS #1 at tick 1,000,000, UTC second 1,700,000,000 */
    SYN_HPTimestamp pps_ts;
    pps_ts.msb_1 = 0;
    pps_ts.lsb = 1000000;
    pps_ts.msb_2 = 0;

    uint64_t epoch_sec = 1700000000ULL;
    SYN_Status st = syn_timesync_bind_pps(&tsync, &pps_ts, epoch_sec);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_TRUE(tsync.has_pps_lock);

    /* Event occurs 8,000,000 ticks later (0.5 seconds at 16 MHz) */
    SYN_HPTimestamp event_ts;
    event_ts.msb_1 = 0;
    event_ts.lsb = 9000000;
    event_ts.msb_2 = 0;

    SYN_UTCTimestamp utc;
    st = syn_timesync_resolve_utc(&tsync, &event_ts, &utc);

    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(SYN_TIME_SOURCE_GPS_PPS, utc.source);
    TEST_ASSERT_EQUAL_UINT64(1700000000ULL, utc.sec);
    TEST_ASSERT_EQUAL_UINT32(500000000U, utc.nsec); /* 0.5s = 500,000,000 ns */
    TEST_ASSERT_EQUAL_UINT32(50, utc.uncertainty_ns);
}

static void test_timesync_event_before_pps(void)
{
    SYN_TimeSync tsync;
    syn_timesync_init(&tsync);

    /* Bind PPS at tick 10,000,000, UTC second 100 */
    SYN_HPTimestamp pps_ts;
    pps_ts.msb_1 = 0;
    pps_ts.lsb = 10000000;
    pps_ts.msb_2 = 0;

    syn_timesync_bind_pps(&tsync, &pps_ts, 100);

    /* Event occurs 1,600,000 ticks (100 ms) BEFORE the PPS anchor */
    SYN_HPTimestamp event_ts;
    event_ts.msb_1 = 0;
    event_ts.lsb = 8400000;
    event_ts.msb_2 = 0;

    SYN_UTCTimestamp utc;
    syn_timesync_resolve_utc(&tsync, &event_ts, &utc);

    /* Expected: UTC sec 99, 900,000,000 ns (99.9 seconds) */
    TEST_ASSERT_EQUAL_UINT64(99, utc.sec);
    TEST_ASSERT_EQUAL_UINT32(900000000U, utc.nsec);
    TEST_ASSERT_EQUAL(SYN_TIME_SOURCE_GPS_PPS, utc.source);
}

static void test_timesync_drift_ppm_measurement(void)
{
    SYN_TimeSync tsync;
    syn_timesync_init(&tsync);

    /* PPS 1 at tick 0, UTC sec 100 */
    SYN_HPTimestamp pps1;
    pps1.msb_1 = 0;
    pps1.lsb = 0;
    pps1.msb_2 = 0;
    syn_timesync_bind_pps(&tsync, &pps1, 100);

    /* PPS 2 at tick 16,000,160 (160 ticks fast over 16M nominal = +10 PPM) */
    SYN_HPTimestamp pps2;
    pps2.msb_1 = 0;
    pps2.lsb = 16000160;
    pps2.msb_2 = 0;
    syn_timesync_bind_pps(&tsync, &pps2, 101);

    TEST_ASSERT_EQUAL_INT32(10, tsync.drift_ppm);
}

static void test_timesync_holdover_uncertainty_growth(void)
{
    SYN_TimeSync tsync;
    syn_timesync_init(&tsync);

    /* Bind PPS 1 & 2 to establish 10 PPM drift */
    SYN_HPTimestamp pps1, pps2;
    pps1.msb_1 = 0;
    pps1.lsb = 0;
    pps1.msb_2 = 0;
    pps2.msb_1 = 0;
    pps2.lsb = 16000160;
    pps2.msb_2 = 0;

    syn_timesync_bind_pps(&tsync, &pps1, 100);
    syn_timesync_bind_pps(&tsync, &pps2, 101);

    /* Event 5 seconds into holdover (16MHz * 5 = 80,000,000 ticks past pps2) */
    SYN_HPTimestamp event_ts;
    event_ts.msb_1 = 0;
    event_ts.lsb = 16000160 + 80000000;
    event_ts.msb_2 = 0;

    SYN_UTCTimestamp utc;
    syn_timesync_resolve_utc(&tsync, &event_ts, &utc);

    TEST_ASSERT_EQUAL(SYN_TIME_SOURCE_GPS_HOLDOVER, utc.source);
    TEST_ASSERT_EQUAL_UINT64(106, utc.sec);

    /* Uncertainty: base (50ns) + drift (10 PPM * 5s * 1000ns = 50,000ns) = 50,050ns */
    TEST_ASSERT_EQUAL_UINT32(50050U, utc.uncertainty_ns);
}

static void test_timesync_holdover_expiration(void)
{
    SYN_TimeSync tsync;
    syn_timesync_init(&tsync);
    tsync.max_holdover_s = 10; /* Shorten holdover to 10s for test */

    SYN_HPTimestamp pps;
    pps.msb_1 = 0;
    pps.lsb = 0;
    pps.msb_2 = 0;
    syn_timesync_bind_pps(&tsync, &pps, 100);

    /* Event 15 seconds after PPS (exceeds 10s max holdover) */
    SYN_HPTimestamp event_ts;
    event_ts.msb_1 = 0;
    event_ts.lsb = 16000000UL * 15;
    event_ts.msb_2 = 0;

    SYN_UTCTimestamp utc;
    syn_timesync_resolve_utc(&tsync, &event_ts, &utc);

    /* Should degrade to UNSYNCED (since RTC not mocked in this test) */
    TEST_ASSERT_EQUAL(SYN_TIME_SOURCE_UNSYNCED, utc.source);
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFU, utc.uncertainty_ns);
}

static void test_timesync_to_epoch_ns_helper(void)
{
    SYN_TimeSync tsync;
    syn_timesync_init(&tsync);

    SYN_HPTimestamp pps;
    pps.msb_1 = 0;
    pps.lsb = 0;
    pps.msb_2 = 0;
    syn_timesync_bind_pps(&tsync, &pps, 10); /* UTC sec = 10 */

    SYN_HPTimestamp event_ts;
    event_ts.msb_1 = 0;
    event_ts.lsb = 16000; /* 1 ms = 1,000,000 ns */
    event_ts.msb_2 = 0;

    uint64_t epoch_ns = syn_timesync_to_epoch_ns(&tsync, &event_ts);
    /* 10 seconds * 1e9 + 1,000,000 ns = 10,001,000,000 ns */
    TEST_ASSERT_EQUAL_UINT64(10001000000ULL, epoch_ns);
}

static void test_timesync_null_params(void)
{
    SYN_TimeSync tsync;
    syn_timesync_init(&tsync);

    SYN_HPTimestamp ts;
    ts.msb_1 = 0;
    ts.lsb = 100;
    ts.msb_2 = 0;
    SYN_UTCTimestamp utc;

    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_timesync_bind_pps(NULL, &ts, 100));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_timesync_bind_pps(&tsync, NULL, 100));

    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_timesync_resolve_utc(NULL, &ts, &utc));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_timesync_resolve_utc(&tsync, NULL, &utc));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_timesync_resolve_utc(&tsync, &ts, NULL));

    TEST_ASSERT_EQUAL_UINT64(0, syn_timesync_to_epoch_ns(NULL, &ts));
}

static void test_timesync_is_pps_locked(void)
{
    TEST_ASSERT_FALSE(syn_timesync_is_pps_locked(NULL));

    SYN_TimeSync tsync;
    syn_timesync_init(&tsync);
    TEST_ASSERT_FALSE(syn_timesync_is_pps_locked(&tsync));

    SYN_HPTimestamp pps;
    pps.msb_1 = 0;
    pps.lsb = 0;
    pps.msb_2 = 0;
    syn_timesync_bind_pps(&tsync, &pps, 100);

    TEST_ASSERT_TRUE(syn_timesync_is_pps_locked(&tsync));
}

/* ── Runner ────────────────────────────────────────────────────────────── */

void setUp(void)
{
    fake_timer_cnt = 0;
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_timesync_init);
    RUN_TEST(test_timesync_unsynced_fallback);
    RUN_TEST(test_timesync_locked_resolution);
    RUN_TEST(test_timesync_event_before_pps);
    RUN_TEST(test_timesync_drift_ppm_measurement);
    RUN_TEST(test_timesync_holdover_uncertainty_growth);
    RUN_TEST(test_timesync_holdover_expiration);
    RUN_TEST(test_timesync_to_epoch_ns_helper);
    RUN_TEST(test_timesync_null_params);
    RUN_TEST(test_timesync_is_pps_locked);
    return UNITY_END();
}
