/**
 * @file test_i2c_queue.c
 * @brief Unity unit tests for asynchronous I2C transaction queue driver (syn_i2c_queue).
 */

#include "mocks/mock_port.h"
#include "syntropic/drivers/syn_i2c_queue.h"
#include "unity/unity.h"

#include <string.h>

static bool callback_executed = false;
static SYN_Status last_callback_status = SYN_ERROR;
static void *last_user_data = NULL;

static void test_i2c_cb(uint8_t bus, SYN_Status result, void *user_data)
{
    (void)bus;
    callback_executed = true;
    last_callback_status = result;
    last_user_data = user_data;
}

static void test_i2c_queue_null_and_init(void)
{
    SYN_I2C_Queue q;
    SYN_I2C_Transaction tx = {0};

    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_i2c_queue_init(NULL, 0));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_i2c_queue_enqueue(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_i2c_queue_process(NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_i2c_queue_cancel_all(NULL));
    TEST_ASSERT_EQUAL_UINT(0, syn_i2c_queue_count(NULL));

    /* Uninitialized handle checks */
    memset(&q, 0, sizeof(q));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_i2c_queue_enqueue(&q, &tx));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_i2c_queue_process(&q));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_i2c_queue_cancel_all(&q));
    TEST_ASSERT_EQUAL_UINT(0, syn_i2c_queue_count(&q));

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_i2c_queue_init(&q, 1));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_i2c_queue_enqueue(&q, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_i2c_queue_process(&q));
    TEST_ASSERT_EQUAL_UINT(0, syn_i2c_queue_count(&q));
}

static void test_i2c_queue_enqueue_and_process(void)
{
    SYN_I2C_Queue q;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_i2c_queue_init(&q, 0));

    uint8_t tx_buf[] = {0x10, 0xAA};
    uint8_t rx_buf[2] = {0};
    int dummy_ctx = 42;

    SYN_I2C_Transaction tx = {
        .bus = 0,
        .addr = 0x50,
        .clock_speed_hz = 400000,
        .tx_data = tx_buf,
        .tx_len = sizeof(tx_buf),
        .rx_data = rx_buf,
        .rx_len = sizeof(rx_buf),
        .callback = test_i2c_cb,
        .user_data = &dummy_ctx,
    };

    callback_executed = false;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_i2c_queue_enqueue(&q, &tx));
    TEST_ASSERT_TRUE(callback_executed);
    TEST_ASSERT_EQUAL_INT(SYN_OK, last_callback_status);
    TEST_ASSERT_EQUAL_PTR(&dummy_ctx, last_user_data);
    TEST_ASSERT_EQUAL_UINT(0, syn_i2c_queue_count(&q));
}

static void test_i2c_queue_overflow_and_cancel(void)
{
    SYN_I2C_Queue q;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_i2c_queue_init(&q, 0));

    SYN_I2C_Transaction tx = {
        .bus = 0,
        .addr = 0x40,
        .callback = NULL,
    };

    q.active = true; /* Lock processing so items stack up */

    for (int i = 0; i < SYN_I2C_QUEUE_MAX_DEPTH; i++) {
        TEST_ASSERT_EQUAL_INT(SYN_OK, syn_i2c_queue_enqueue(&q, &tx));
    }
    TEST_ASSERT_EQUAL_UINT(SYN_I2C_QUEUE_MAX_DEPTH, syn_i2c_queue_count(&q));
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_i2c_queue_enqueue(&q, &tx));

    /* Test recursive queue drain */
    q.active = false;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_i2c_queue_process(&q));
    TEST_ASSERT_EQUAL_UINT(0, syn_i2c_queue_count(&q));

    /* Test cancel all on fresh queue */
    for (int i = 0; i < 3; i++) {
        q.active = true;
        (void)syn_i2c_queue_enqueue(&q, &tx);
    }
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_i2c_queue_cancel_all(&q));
    TEST_ASSERT_EQUAL_UINT(0, syn_i2c_queue_count(&q));
}

void run_i2c_queue_tests(void)
{
    RUN_TEST(test_i2c_queue_null_and_init);
    RUN_TEST(test_i2c_queue_enqueue_and_process);
    RUN_TEST(test_i2c_queue_overflow_and_cancel);
}
