/**
 * @file test_ao.c
 * @brief Unity tests for Active Object concurrency framework.
 */

#include "mocks/mock_port.h"
#include "syntropic/syntropic.h"
#include "unity/unity.h"

enum { AO_ST_OFF, AO_ST_ON };
enum { AO_EV_TOGGLE, AO_EV_SET_VAL };

static int ao_action_called = 0;
static int ao_last_val = 0;

static void ao_on_toggle(void *ctx)
{
    (void)ctx;
    ao_action_called++;
}

static void ao_on_set_val(void *ctx)
{
    SYN_ActiveObject *ao = (SYN_ActiveObject *)ctx;
    if (ao && ao->last_event.data) {
        ao_last_val = *(int *)ao->last_event.data;
    }
}

static const SYN_FSM_Transition test_ao_table[] = {
    {AO_ST_OFF, AO_EV_TOGGLE, AO_ST_ON, NULL, ao_on_toggle},
    {AO_ST_ON, AO_EV_TOGGLE, AO_ST_OFF, NULL, ao_on_toggle},
    {AO_ST_OFF, AO_EV_SET_VAL, AO_ST_OFF, NULL, ao_on_set_val},
    {AO_ST_ON, AO_EV_SET_VAL, AO_ST_ON, NULL, ao_on_set_val},
    SYN_FSM_END};

static SYN_ActiveObject test_ao_obj;
static SYN_AO_Event mbox_buf[4];

static void test_active_object(void)
{
    syn_ao_init(&test_ao_obj, "test_ao", test_ao_table, AO_ST_OFF, mbox_buf, 4);

    TEST_ASSERT_EQUAL(AO_ST_OFF, syn_fsm_state(&test_ao_obj.fsm));

    /* Post toggle event */
    bool ok = syn_ao_post(&test_ao_obj, AO_EV_TOGGLE, NULL);
    TEST_ASSERT_TRUE(ok);

    /* Setup a minimal scheduler and run once */
    SYN_Sched sched;
    syn_sched_init(&sched, &test_ao_obj.task, 1);

    ao_action_called = 0;
    syn_sched_run(&sched);

    TEST_ASSERT_EQUAL(AO_ST_ON, syn_fsm_state(&test_ao_obj.fsm));
    TEST_ASSERT_EQUAL_INT(1, ao_action_called);

    /* Post again */
    ok = syn_ao_post(&test_ao_obj, AO_EV_TOGGLE, NULL);
    TEST_ASSERT_TRUE(ok);

    syn_sched_run(&sched);

    TEST_ASSERT_EQUAL(AO_ST_OFF, syn_fsm_state(&test_ao_obj.fsm));
    TEST_ASSERT_EQUAL_INT(2, ao_action_called);
}

static void test_active_object_overflow(void)
{
    /* Mailbox capacity is 4. Since SPSC mailbox reserves 1 slot as sentinel, max capacity is 3. */
    syn_ao_init(&test_ao_obj, "test_ao_overflow", test_ao_table, AO_ST_OFF, mbox_buf, 4);

    bool ok = syn_ao_post(&test_ao_obj, AO_EV_TOGGLE, NULL);
    TEST_ASSERT_TRUE(ok);
    ok = syn_ao_post(&test_ao_obj, AO_EV_TOGGLE, NULL);
    TEST_ASSERT_TRUE(ok);
    ok = syn_ao_post(&test_ao_obj, AO_EV_TOGGLE, NULL);
    TEST_ASSERT_TRUE(ok);

    /* 4th post should fail because mailbox is full */
    ok = syn_ao_post(&test_ao_obj, AO_EV_TOGGLE, NULL);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_EQUAL_UINT32(1, syn_mailbox_overflows(&test_ao_obj.mailbox));
}

static void test_active_object_payload(void)
{
    syn_ao_init(&test_ao_obj, "test_ao_payload", test_ao_table, AO_ST_OFF, mbox_buf, 4);

    int val = 42;
    TEST_ASSERT_TRUE(syn_ao_post(&test_ao_obj, AO_EV_SET_VAL, &val));

    SYN_Sched sched;
    syn_sched_init(&sched, &test_ao_obj.task, 1);
    syn_sched_run(&sched);

    TEST_ASSERT_EQUAL_INT(42, ao_last_val);
}

void run_ao_tests(void)
{
    RUN_TEST(test_active_object);
    RUN_TEST(test_active_object_overflow);
    RUN_TEST(test_active_object_payload);
}
