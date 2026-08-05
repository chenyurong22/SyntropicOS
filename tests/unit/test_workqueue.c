/**
 * @file test_workqueue.c
 * @brief Unity tests for syn_workqueue.
 */

#include "mocks/mock_port.h"
#include "syntropic/sched/syn_workqueue.h"
#include "syntropic/syntropic.h"
#include "unity/unity.h"

static int wq_sum = 0;
static void wq_handler(void *ctx)
{
    wq_sum += *(int *)ctx;
}

static void test_workqueue(void)
{
    wq_sum = 0;

    SYN_WorkItem items[4];
    SYN_WorkQueue wq;
    syn_workqueue_init(&wq, items, 4);

    TEST_ASSERT_TRUE(syn_workqueue_empty(&wq));
    TEST_ASSERT_EQUAL_INT(0, syn_workqueue_pending(&wq));

    static int v1 = 10, v2 = 20, v3 = 30;

    TEST_ASSERT_TRUE(syn_workqueue_post(&wq, wq_handler, &v1));
    TEST_ASSERT_TRUE(syn_workqueue_post(&wq, wq_handler, &v2));
    TEST_ASSERT_TRUE(syn_workqueue_post(&wq, wq_handler, &v3));
    /* Queue capacity = 4, but SPSC ring uses one slot as sentinel,
       so max usable = 3 */
    TEST_ASSERT_FALSE(syn_workqueue_post(&wq, wq_handler, &v1));
    TEST_ASSERT_EQUAL_INT(1, syn_workqueue_overflows(&wq));

    TEST_ASSERT_FALSE(syn_workqueue_empty(&wq));
    TEST_ASSERT_EQUAL_INT(3, syn_workqueue_pending(&wq));

    size_t processed = syn_workqueue_process(&wq);
    TEST_ASSERT_EQUAL_INT(3, processed);
    TEST_ASSERT_EQUAL_INT(60, wq_sum);
    TEST_ASSERT_TRUE(syn_workqueue_empty(&wq));

    /* Process empty queue = 0 */
    processed = syn_workqueue_process(&wq);
    TEST_ASSERT_EQUAL_INT(0, processed);
}

static void test_workqueue_null_params(void)
{
    SYN_WorkItem items[4];
    SYN_WorkQueue wq;
    syn_workqueue_init(&wq, items, 4);

    /* Post with NULL wq or NULL func */
    static int val = 5;
    TEST_ASSERT_FALSE(syn_workqueue_post(NULL, wq_handler, &val));
    TEST_ASSERT_FALSE(syn_workqueue_post(&wq, NULL, &val));

    /* Process with NULL wq */
    TEST_ASSERT_EQUAL_size_t(0, syn_workqueue_process(NULL));
}

static void wq_null_ctx_handler(void *ctx)
{
    if (ctx != NULL) {
        wq_sum += *(int *)ctx;
    } else {
        wq_sum += 1;
    }
}

static void test_workqueue_null_context(void)
{
    SYN_WorkItem items[4];
    SYN_WorkQueue wq;

    syn_workqueue_init(&wq, items, 4);

    /* Post with NULL context (allowed) */
    wq_sum = 0;
    TEST_ASSERT_TRUE(syn_workqueue_post(&wq, wq_null_ctx_handler, NULL));
    TEST_ASSERT_EQUAL_INT(1, syn_workqueue_pending(&wq));

    size_t processed = syn_workqueue_process(&wq);
    TEST_ASSERT_EQUAL_INT(1, processed);
    TEST_ASSERT_EQUAL_INT(1, wq_sum);
}

void run_workqueue_tests(void)
{
    RUN_TEST(test_workqueue);
    RUN_TEST(test_workqueue_null_params);
    RUN_TEST(test_workqueue_null_context);
}
