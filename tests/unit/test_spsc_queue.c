/**
 * @file test_spsc_queue.c
 * @brief Unit test suite for Lock-Free SPSC Queue (syn_spsc_queue).
 */

#include "unity/unity.h"
#include "syntropic/util/syn_spsc_queue.h"

typedef struct {
    uint32_t id;
    uint16_t val;
} TestMsg;

void test_spsc_queue_push_pop_count(void)
{
    TestMsg buffer[8];
    SYN_SPSC_Queue q;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spsc_queue_init(&q, buffer, sizeof(TestMsg), 8));
    TEST_ASSERT_TRUE(syn_spsc_queue_is_empty(&q));
    TEST_ASSERT_FALSE(syn_spsc_queue_is_full(&q));
    TEST_ASSERT_EQUAL_UINT32(0, syn_spsc_queue_count(&q));

    TestMsg msg1 = {.id = 100, .val = 0xAA};
    TestMsg msg2 = {.id = 101, .val = 0xBB};

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spsc_queue_push(&q, &msg1));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spsc_queue_push(&q, &msg2));
    TEST_ASSERT_FALSE(syn_spsc_queue_is_empty(&q));
    TEST_ASSERT_EQUAL_UINT32(2, syn_spsc_queue_count(&q));

    TestMsg out;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spsc_queue_pop(&q, &out));
    TEST_ASSERT_EQUAL_UINT32(100, out.id);
    TEST_ASSERT_EQUAL_UINT16(0xAA, out.val);

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spsc_queue_pop(&q, &out));
    TEST_ASSERT_EQUAL_UINT32(101, out.id);
    TEST_ASSERT_EQUAL_UINT16(0xBB, out.val);

    TEST_ASSERT_TRUE(syn_spsc_queue_is_empty(&q));
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_spsc_queue_pop(&q, &out));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_spsc_queue_init(NULL, buffer, sizeof(TestMsg), 8));
}

void test_spsc_queue_null_params_and_capacity(void)
{
    TestMsg buffer[4];
    SYN_SPSC_Queue q;

    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_spsc_queue_init(&q, NULL, sizeof(TestMsg), 4));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_spsc_queue_init(&q, buffer, 0, 4));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_spsc_queue_init(&q, buffer, sizeof(TestMsg), 0));

    TEST_ASSERT_TRUE(syn_spsc_queue_is_empty(NULL));
    TEST_ASSERT_FALSE(syn_spsc_queue_is_full(NULL));
    TEST_ASSERT_EQUAL_UINT32(0, syn_spsc_queue_count(NULL));

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spsc_queue_init(&q, buffer, sizeof(TestMsg), 4));

    TestMsg msg = {.id = 1, .val = 10};
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_spsc_queue_push(NULL, &msg));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_spsc_queue_push(&q, NULL));

    TestMsg out;
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_spsc_queue_pop(NULL, &out));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_spsc_queue_pop(&q, NULL));
}

void test_spsc_queue_full_overflow_and_wraparound(void)
{
    TestMsg buffer[4]; /* Capacity = 4 (3 usable elements) */
    SYN_SPSC_Queue q;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spsc_queue_init(&q, buffer, sizeof(TestMsg), 4));

    TestMsg m1 = {.id = 1, .val = 10};
    TestMsg m2 = {.id = 2, .val = 20};
    TestMsg m3 = {.id = 3, .val = 30};
    TestMsg m4 = {.id = 4, .val = 40};

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spsc_queue_push(&q, &m1));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spsc_queue_push(&q, &m2));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spsc_queue_push(&q, &m3));

    /* Queue is now full (3 items) */
    TEST_ASSERT_TRUE(syn_spsc_queue_is_full(&q));
    TEST_ASSERT_EQUAL_UINT32(3, syn_spsc_queue_count(&q));

    /* Pushing 4th item returns SYN_BUSY */
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_spsc_queue_push(&q, &m4));

    /* Pop one item to create space, testing wraparound */
    TestMsg out;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spsc_queue_pop(&q, &out));
    TEST_ASSERT_EQUAL_UINT32(1, out.id);
    TEST_ASSERT_FALSE(syn_spsc_queue_is_full(&q));

    /* Push m4 successfully now */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spsc_queue_push(&q, &m4));
    TEST_ASSERT_EQUAL_UINT32(3, syn_spsc_queue_count(&q));

    /* Drain remainder */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spsc_queue_pop(&q, &out));
    TEST_ASSERT_EQUAL_UINT32(2, out.id);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spsc_queue_pop(&q, &out));
    TEST_ASSERT_EQUAL_UINT32(3, out.id);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_spsc_queue_pop(&q, &out));
    TEST_ASSERT_EQUAL_UINT32(4, out.id);

    TEST_ASSERT_TRUE(syn_spsc_queue_is_empty(&q));
}

void run_spsc_queue_tests(void)
{
    RUN_TEST(test_spsc_queue_push_pop_count);
    RUN_TEST(test_spsc_queue_null_params_and_capacity);
    RUN_TEST(test_spsc_queue_full_overflow_and_wraparound);
}
