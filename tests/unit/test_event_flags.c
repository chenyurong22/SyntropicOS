/**
 * @file test_event_flags.c
 * @brief Unit test suite for Event Flag Groups (syn_event_flags).
 */

#include "mocks/mock_port.h"
#include "syntropic/sched/syn_event_flags.h"
#include "syntropic/syntropic.h"
#include "unity/unity.h"

void test_event_flags_set_clear_wait(void)
{
    SYN_EventFlags ef;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_event_flags_init(&ef));
    TEST_ASSERT_EQUAL_UINT32(0, syn_event_flags_get(&ef));

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_event_flags_set(&ef, 0x05U)); /* Bits 0 & 2 */
    TEST_ASSERT_EQUAL_UINT32(0x05U, syn_event_flags_get(&ef));

    uint32_t matched = 0;

    /* WAIT_ALL - unsatisfiable (requires 0x07U) */
    TEST_ASSERT_EQUAL_INT(SYN_BUSY,
                          syn_event_flags_wait(&ef, 0x07U, SYN_EVENT_FLAGS_WAIT_ALL, &matched));

    /* WAIT_ANY - satisfied (bit 0 or 2 present) */
    TEST_ASSERT_EQUAL_INT(SYN_OK,
                          syn_event_flags_wait(&ef, 0x07U, SYN_EVENT_FLAGS_WAIT_ANY, &matched));
    TEST_ASSERT_EQUAL_UINT32(0x05U, matched);

    /* WAIT_ALL with AUTO_CLEAR */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_event_flags_set(&ef, 0x02U)); /* Bit 1 set -> now 0x07U */
    TEST_ASSERT_EQUAL_INT(
        SYN_OK, syn_event_flags_wait(
                    &ef, 0x07U, SYN_EVENT_FLAGS_WAIT_ALL | SYN_EVENT_FLAGS_AUTO_CLEAR, &matched));
    TEST_ASSERT_EQUAL_UINT32(0x07U, matched);
    TEST_ASSERT_EQUAL_UINT32(0, syn_event_flags_get(&ef)); /* Flags auto-cleared */

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_event_flags_clear(&ef, 0xFFFFFFFFU));
}

void test_event_flags_null_and_invalid_params(void)
{
    SYN_EventFlags ef;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_event_flags_init(&ef));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_event_flags_set(&ef, 0x01U));

    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_event_flags_init(NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_event_flags_set(NULL, 0x01U));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_event_flags_clear(NULL, 0x01U));
    TEST_ASSERT_EQUAL_UINT32(0, syn_event_flags_get(NULL));

    /* Wait with NULL context or wait_mask == 0 */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM,
                          syn_event_flags_wait(NULL, 0x01U, SYN_EVENT_FLAGS_WAIT_ANY, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM,
                          syn_event_flags_wait(&ef, 0, SYN_EVENT_FLAGS_WAIT_ANY, NULL));

    /* Wait with out_flags == NULL (valid call) */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_event_flags_wait(&ef, 0x01U, SYN_EVENT_FLAGS_WAIT_ANY, NULL));
}

static void test_event_flags_wait_any_auto_clear(void)
{
    SYN_EventFlags ef;
    syn_event_flags_init(&ef);
    syn_event_flags_set(&ef, 0x0CU);
    uint32_t matched = 0;
    TEST_ASSERT_EQUAL_INT(
        SYN_OK, syn_event_flags_wait(
                    &ef, 0x0EU, SYN_EVENT_FLAGS_WAIT_ANY | SYN_EVENT_FLAGS_AUTO_CLEAR, &matched));
    TEST_ASSERT_EQUAL_UINT32(0x0CU, matched);
    TEST_ASSERT_EQUAL_UINT32(0x00U, syn_event_flags_get(&ef));
}

/**
 * Regression: set/clear/wait are ISR-callable read-modify-writes and must
 * run inside critical sections (volatile alone is not atomic). Verifies
 * every path — including the wait() early-return — enters and fully exits
 * the critical section.
 */
static void test_event_flags_rmw_critical_sections(void)
{
    SYN_EventFlags ef;
    syn_event_flags_init(&ef);

    int enters = mock_critical_enter_count;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_event_flags_set(&ef, 0x01U));
    TEST_ASSERT_EQUAL_INT(enters + 1, mock_critical_enter_count);
    TEST_ASSERT_EQUAL_INT(0, mock_critical_depth);

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_event_flags_clear(&ef, 0x01U));
    TEST_ASSERT_EQUAL_INT(enters + 2, mock_critical_enter_count);
    TEST_ASSERT_EQUAL_INT(0, mock_critical_depth);

    /* wait(): satisfied path with AUTO_CLEAR (RMW) must be protected and
     * must not clobber bits outside the matched set */
    syn_event_flags_set(&ef, 0x03U);
    uint32_t matched = 0;
    TEST_ASSERT_EQUAL_INT(
        SYN_OK, syn_event_flags_wait(
                    &ef, 0x01U, SYN_EVENT_FLAGS_WAIT_ANY | SYN_EVENT_FLAGS_AUTO_CLEAR, &matched));
    TEST_ASSERT_EQUAL_UINT32(0x01U, matched);
    TEST_ASSERT_EQUAL_UINT32(0x02U, syn_event_flags_get(&ef));
    TEST_ASSERT_EQUAL_INT(0, mock_critical_depth);

    /* wait(): unsatisfied path must not leak the critical section */
    TEST_ASSERT_EQUAL_INT(SYN_BUSY,
                          syn_event_flags_wait(&ef, 0x10U, SYN_EVENT_FLAGS_WAIT_ANY, NULL));
    TEST_ASSERT_EQUAL_INT(0, mock_critical_depth);
}

static SYN_EventFlags g_block_ef;

static SYN_PT_Status pt_block_event_func(SYN_PT *pt, SYN_Task *task)
{
    PT_BEGIN(pt);
    PT_BLOCK_EVENT(pt, task, &g_block_ef, 0x01U);
    PT_END(pt);
}

static void test_pt_block_event_critical_section(void)
{
    SYN_Task task;
    SYN_Sched sched;
    syn_task_create(&task, "evt_block", pt_block_event_func, 0, NULL);
    syn_sched_init(&sched, &task, 1);
    syn_event_flags_init(&g_block_ef);
    syn_event_flags_set(&g_block_ef, 0x01U);

    int enters_before = mock_critical_enter_count;
    syn_sched_run(&sched);
    syn_sched_run(&sched);

    TEST_ASSERT_GREATER_THAN(enters_before, mock_critical_enter_count);
    TEST_ASSERT_EQUAL_INT(0, mock_critical_depth);
    TEST_ASSERT_EQUAL_UINT32(0, syn_event_flags_get(&g_block_ef));
}

void run_event_flags_tests(void)
{
    RUN_TEST(test_event_flags_set_clear_wait);
    RUN_TEST(test_event_flags_null_and_invalid_params);
    RUN_TEST(test_event_flags_wait_any_auto_clear);
    RUN_TEST(test_event_flags_rmw_critical_sections);
    RUN_TEST(test_pt_block_event_critical_section);
}
