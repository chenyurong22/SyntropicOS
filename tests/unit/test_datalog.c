#include "syntropic/log/syn_datalog.h"
#include "unity/unity.h"

#include <string.h>

#define BUF_SIZE 64

static SYN_DataLog datalog;
static uint8_t backing[BUF_SIZE];

void test_datalog_init(void)
{
    syn_datalog_init(&datalog, backing, sizeof(backing));
    TEST_ASSERT_EQUAL(0, syn_datalog_get_dropped(&datalog));
}

void test_datalog_write_read(void)
{
    syn_datalog_init(&datalog, backing, sizeof(backing));

    struct {
        int x, y;
    } point = {10, -20};

    // Write
    TEST_ASSERT_TRUE(syn_datalog_write(&datalog, 0x1234, &point, sizeof(point)));

    // Read
    uint16_t out_id = 0;
    struct {
        int x, y;
    } out_point = {0, 0};

    size_t read_bytes = syn_datalog_read(&datalog, &out_id, &out_point, sizeof(out_point));

    TEST_ASSERT_EQUAL(sizeof(out_point), read_bytes);
    TEST_ASSERT_EQUAL_HEX16(0x1234, out_id);
    TEST_ASSERT_EQUAL(10, out_point.x);
    TEST_ASSERT_EQUAL(-20, out_point.y);
}

void test_datalog_overflow(void)
{
    syn_datalog_init(&datalog, backing, sizeof(backing));

    uint8_t payload[16] = {0};

    // Each frame is 4 bytes header + 16 bytes payload = 20 bytes.
    // Buffer size 64, usable 63. 63 / 20 = 3 frames.

    TEST_ASSERT_TRUE(syn_datalog_write(&datalog, 1, payload, sizeof(payload)));
    TEST_ASSERT_TRUE(syn_datalog_write(&datalog, 2, payload, sizeof(payload)));
    TEST_ASSERT_TRUE(syn_datalog_write(&datalog, 3, payload, sizeof(payload)));

    // 4th should fail
    TEST_ASSERT_FALSE(syn_datalog_write(&datalog, 4, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL(1, syn_datalog_get_dropped(&datalog));

    // Read one out
    uint16_t id;
    uint8_t out[16];
    TEST_ASSERT_EQUAL(16, syn_datalog_read(&datalog, &id, out, sizeof(out)));
    TEST_ASSERT_EQUAL(1, id);

    // Now we can write one
    TEST_ASSERT_TRUE(syn_datalog_write(&datalog, 5, payload, sizeof(payload)));
}

void test_datalog_read_buffer_too_small(void)
{
    syn_datalog_init(&datalog, backing, sizeof(backing));

    uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    TEST_ASSERT_TRUE(syn_datalog_write(&datalog, 0xABCD, payload, sizeof(payload)));

    /* Try reading with a buffer that is too small — frame must be left intact */
    uint16_t id = 0;
    uint8_t small_buf[4];
    size_t read_bytes = syn_datalog_read(&datalog, &id, small_buf, sizeof(small_buf));
    TEST_ASSERT_EQUAL(0, read_bytes);
    TEST_ASSERT_EQUAL_HEX16(0xABCD, id); /* id is still populated from peek */

    /* Frame must still be in the buffer — retry with a big enough buffer succeeds */
    uint8_t big_buf[8];
    read_bytes = syn_datalog_read(&datalog, &id, big_buf, sizeof(big_buf));
    TEST_ASSERT_EQUAL(sizeof(payload), read_bytes);
    TEST_ASSERT_EQUAL_HEX16(0xABCD, id);
    TEST_ASSERT_EQUAL_MEMORY(payload, big_buf, sizeof(payload));

    /* Dropped count must not have changed (this was not a data-loss event) */
    TEST_ASSERT_EQUAL(0, syn_datalog_get_dropped(&datalog));
}

/** Read from empty datalog — exercises line 49: return 0 */
void test_datalog_read_empty(void)
{
    syn_datalog_init(&datalog, backing, sizeof(backing));
    uint16_t id = 0;
    uint8_t out[8] = {0};
    size_t n = syn_datalog_read(&datalog, &id, out, sizeof(out));
    TEST_ASSERT_EQUAL_size_t(0, n);
}

void test_datalog_zero_len_payload_and_incomplete_header(void)
{
    syn_datalog_init(&datalog, backing, sizeof(backing));

    /* Write 0-byte payload log entry (header only) */
    TEST_ASSERT_TRUE(syn_datalog_write(&datalog, 0x5555, NULL, 0));

    uint16_t out_id = 0;
    uint8_t out_buf[16];
    size_t read_bytes = syn_datalog_read(&datalog, &out_id, out_buf, sizeof(out_buf));
    TEST_ASSERT_EQUAL_size_t(0, read_bytes);
    TEST_ASSERT_EQUAL_HEX16(0x5555, out_id);

    /* Incomplete header: manually write 2 bytes into ringbuf (less than 4-byte header) */
    uint8_t partial[2] = {0x01, 0x02};
    syn_ringbuf_write(&datalog.rb, partial, 2);
    read_bytes = syn_datalog_read(&datalog, &out_id, out_buf, sizeof(out_buf));
    TEST_ASSERT_EQUAL_size_t(0, read_bytes);
}

void test_datalog_clear(void)
{
    syn_datalog_init(&datalog, backing, sizeof(backing));
    uint8_t payload[4] = {1, 2, 3, 4};
    syn_datalog_write(&datalog, 0x1234, payload, sizeof(payload));
    syn_datalog_reset(&datalog);
    TEST_ASSERT_EQUAL_size_t(0, syn_datalog_available(&datalog));
    TEST_ASSERT_EQUAL_UINT32(0, syn_datalog_get_dropped(&datalog));
}

void test_datalog_null_and_incomplete_payload(void)
{
    /* Incomplete payload: write header specifying 10 bytes payload, but only write 2 payload bytes
     */
    syn_datalog_init(&datalog, backing, sizeof(backing));
    SYN_DataLogHeader header = {.id = 0x7777, .len = 10};
    syn_ringbuf_write(&datalog.rb, (const uint8_t *)&header, sizeof(header));
    uint8_t partial[2] = {0xAA, 0xBB};
    syn_ringbuf_write(&datalog.rb, partial, 2);

    uint16_t out_id = 0;
    uint8_t out_buf[16] = {0};
    TEST_ASSERT_EQUAL_size_t(0, syn_datalog_read(&datalog, &out_id, out_buf, sizeof(out_buf)));
}

void run_datalog_tests(void)
{
    RUN_TEST(test_datalog_init);
    RUN_TEST(test_datalog_write_read);
    RUN_TEST(test_datalog_overflow);
    RUN_TEST(test_datalog_read_buffer_too_small);
    RUN_TEST(test_datalog_read_empty);
    RUN_TEST(test_datalog_zero_len_payload_and_incomplete_header);
    RUN_TEST(test_datalog_clear);
    RUN_TEST(test_datalog_null_and_incomplete_payload);
}
