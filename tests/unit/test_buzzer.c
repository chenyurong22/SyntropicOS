/**
 * @file test_buzzer.c
 * @brief Unity tests for syn_buzzer module.
 */

#include "mocks/mock_port.h"
#include "syntropic/output/syn_buzzer.h"
#include "unity/unity.h"

static void test_buzzer_beep(void)
{
    mock_port_reset();
    SYN_Buzzer buz;

    SYN_Status st = syn_buzzer_init(&buz, 5, true);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_FALSE(syn_buzzer_is_playing(&buz));

    /* Start beep for 100ms at 1000Hz */
    st = syn_buzzer_beep(&buz, 1000, 100);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_TRUE(syn_buzzer_is_playing(&buz));

    /* Step 50ms */
    syn_buzzer_step(&buz, 50);
    TEST_ASSERT_TRUE(syn_buzzer_is_playing(&buz));

    /* Step another 50ms (total 100ms) */
    syn_buzzer_step(&buz, 50);
    TEST_ASSERT_FALSE(syn_buzzer_is_playing(&buz));

    /* Silent pause beep (freq_hz = 0) */
    st = syn_buzzer_beep(&buz, 0, 50);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_TRUE(syn_buzzer_is_playing(&buz));

    /* Zero duration beep (duration_ms = 0) */
    st = syn_buzzer_beep(&buz, 440, 0);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_FALSE(syn_buzzer_is_playing(&buz));
}

static void test_buzzer_pattern(void)
{
    mock_port_reset();
    SYN_Buzzer buz;
    /* Active low buzzer */
    syn_buzzer_init(&buz, 2, false);

    static const uint16_t freqs[] = {440, 0, 880};
    static const uint16_t durs[]  = {50, 20, 50};

    SYN_Status st = syn_buzzer_play_pattern(&buz, freqs, durs, 3);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_TRUE(syn_buzzer_is_playing(&buz));

    /* Note 1 ends -> pause note */
    syn_buzzer_step(&buz, 50);
    TEST_ASSERT_TRUE(syn_buzzer_is_playing(&buz));
    TEST_ASSERT_EQUAL_UINT32(0, buz.freq_hz);

    /* Pause note ends -> note 2 */
    syn_buzzer_step(&buz, 20);
    TEST_ASSERT_TRUE(syn_buzzer_is_playing(&buz));
    TEST_ASSERT_EQUAL_UINT32(880, buz.freq_hz);

    /* Note 2 ends -> pattern complete */
    syn_buzzer_step(&buz, 50);
    TEST_ASSERT_FALSE(syn_buzzer_is_playing(&buz));
}

static void test_buzzer_stop_and_null(void)
{
    mock_port_reset();
    SYN_Buzzer buz;
    syn_buzzer_init(&buz, 1, true);

    syn_buzzer_beep(&buz, 500, 1000);
    TEST_ASSERT_TRUE(syn_buzzer_is_playing(&buz));

    syn_buzzer_stop(&buz);
    TEST_ASSERT_FALSE(syn_buzzer_is_playing(&buz));

    syn_buzzer_stop(NULL);
    syn_buzzer_step(NULL, 10);
    TEST_ASSERT_FALSE(syn_buzzer_is_playing(NULL));
}

void run_buzzer_tests(void)
{
    RUN_TEST(test_buzzer_beep);
    RUN_TEST(test_buzzer_pattern);
    RUN_TEST(test_buzzer_stop_and_null);
}
