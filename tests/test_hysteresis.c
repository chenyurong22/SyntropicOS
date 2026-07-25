/**
 * @file test_hysteresis.c
 * @brief Unity tests for syn_hysteresis.
 */

#include "unity/unity.h"
#include "mocks/mock_port.h"
#include "syntropic/syntropic.h"
#include "syntropic/util/syn_hysteresis.h"

static void test_hysteresis(void)
{
    SYN_Hysteresis h;
    syn_hyst_init(&h, 1000, 50, false);  /* threshold=1000, band=±50 */

    TEST_ASSERT_FALSE(syn_hyst_state(&h));

    /* Value below threshold — no change */
    syn_hyst_update(&h, 900);
    TEST_ASSERT_FALSE(syn_hyst_state(&h));

    /* Value in deadband — no change */
    syn_hyst_update(&h, 1030);
    TEST_ASSERT_FALSE(syn_hyst_state(&h));

    /* Value crosses high trip (1050) */
    syn_hyst_update(&h, 1051);
    TEST_ASSERT_TRUE(syn_hyst_state(&h));

    /* Value drops but stays above low trip (950) */
    syn_hyst_update(&h, 980);
    TEST_ASSERT_TRUE(syn_hyst_state(&h));

    /* Value drops below low trip */
    syn_hyst_update(&h, 940);
    TEST_ASSERT_FALSE(syn_hyst_state(&h));
}

static void test_hysteresis_set_threshold_and_initial_high(void)
{
    SYN_Hysteresis h;
    syn_hyst_init(&h, 500, 20, true);
    TEST_ASSERT_TRUE(syn_hyst_state(&h));

    /* Value in deadband (490) — stays high */
    syn_hyst_update(&h, 490);
    TEST_ASSERT_TRUE(syn_hyst_state(&h));

    /* Value drops below low trip (479 < 480) */
    syn_hyst_update(&h, 479);
    TEST_ASSERT_FALSE(syn_hyst_state(&h));

    /* Update threshold to 1000, band 100 */
    syn_hyst_init(&h, 1000, 100, h.state);
    TEST_ASSERT_EQUAL_INT32(1000, h.threshold);
    TEST_ASSERT_EQUAL_INT32(100, h.band);

    /* Test updated high trip (1100) */
    syn_hyst_update(&h, 1050);
    TEST_ASSERT_FALSE(syn_hyst_state(&h));

    syn_hyst_update(&h, 1101);
    TEST_ASSERT_TRUE(syn_hyst_state(&h));
}

void run_hysteresis_tests(void)
{
    RUN_TEST(test_hysteresis);
    RUN_TEST(test_hysteresis_set_threshold_and_initial_high);
}
