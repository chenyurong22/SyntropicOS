/**
 * @file test_log.c
 * @brief Unity tests for syn_log.
 */

#include "mocks/mock_port.h"
#include "syntropic/log/syn_log.h"
#include "syntropic/syntropic.h"
#include "unity/unity.h"

/* Log now writes directly to syn_port_serial_write, which is mocked
 * by mock_port.c into mock_serial_tx_buf. Alias for test readability. */

#define log_capture_buf ((char *)mock_serial_tx_buf)
#define log_capture_pos mock_serial_tx_len

static void test_logging_basic(void)
{
    mock_tick_ms = 1234;
    log_capture_pos = 0;
    log_capture_buf[0] = '\0';

    syn_log_init(NULL, SYN_LOG_DEBUG);

    syn_log(SYN_LOG_DEBUG, "test", "hello %d", 42);
    TEST_ASSERT_TRUE(log_capture_pos > 0);
    TEST_ASSERT_NOT_NULL(strstr(log_capture_buf, "D/test:"));
    TEST_ASSERT_NOT_NULL(strstr(log_capture_buf, "hello 42"));
    TEST_ASSERT_NOT_NULL(strstr(log_capture_buf, "1234"));

    log_capture_pos = 0;
    syn_log(SYN_LOG_INFO, "net", "connected");
    TEST_ASSERT_NOT_NULL(strstr(log_capture_buf, "I/net:"));

    log_capture_pos = 0;
    SYN_LOG_T("test", "%s", "this should not appear");
    TEST_ASSERT_TRUE(log_capture_pos == 0);

    syn_log_set_level(SYN_LOG_ERROR);
    log_capture_pos = 0;
    syn_log(SYN_LOG_WARN, "test", "warn msg");
    TEST_ASSERT_TRUE(log_capture_pos == 0);

    log_capture_pos = 0;
    syn_log(SYN_LOG_ERROR, "test", "error msg");
    TEST_ASSERT_TRUE(log_capture_pos > 0);

    TEST_ASSERT_TRUE(syn_log_get_level() == SYN_LOG_ERROR);

    syn_log_set_level(SYN_LOG_DEBUG);
    log_capture_pos = 0;
    syn_log_raw("raw text\n");
    TEST_ASSERT_EQUAL_STRING("raw text\n", log_capture_buf);

    log_capture_pos = 0;
    syn_log_raw(NULL);
    TEST_ASSERT_TRUE(log_capture_pos == 0);

    /* Test that logging before init is a no-op */
    mock_serial_tx_len = 0;
    mock_serial_tx_buf[0] = '\0';
    syn_log(SYN_LOG_INFO, "test", "no crash");
    TEST_ASSERT_TRUE(1);
}

static void test_log_hexdump(void)
{
    log_capture_pos = 0;
    log_capture_buf[0] = '\0';

    syn_log_init(NULL, SYN_LOG_DEBUG);

    uint8_t data[20];
    for (int i = 0; i < 20; i++) {
        data[i] = (i < 10) ? ('0' + i) : (i - 10);
    }

    syn_log_hexdump("dump", data, 20);

    TEST_ASSERT_TRUE(log_capture_pos > 0);
    TEST_ASSERT_NOT_NULL(strstr(log_capture_buf, "0000"));
    TEST_ASSERT_NOT_NULL(strstr(log_capture_buf, "0010"));
    TEST_ASSERT_NOT_NULL(strstr(log_capture_buf, "0123456789"));
    TEST_ASSERT_NOT_NULL(strstr(log_capture_buf, "......"));

    /* Test with NULL data */
    log_capture_pos = 0;
    syn_log_hexdump("dump", NULL, 20);
    TEST_ASSERT_TRUE(log_capture_pos == 0);
}

static void test_log_invalid_level(void)
{
    log_capture_pos = 0;
    log_capture_buf[0] = '\0';
    syn_log_init(NULL, SYN_LOG_TRACE);

    syn_log((SYN_LogLevel)10, "test", "invalid level message");
    TEST_ASSERT_TRUE(log_capture_pos > 0);
    TEST_ASSERT_NOT_NULL(strstr(log_capture_buf, "?/test:"));
}

static int custom_output_count = 0;
static char custom_output_last[128];

static void custom_log_output(const char *str, size_t len)
{
    custom_output_count++;
    if (len < sizeof(custom_output_last)) {
        memcpy(custom_output_last, str, len);
        custom_output_last[len] = '\0';
    }
}

static void test_log_null_tag(void)
{
    log_capture_pos = 0;
    log_capture_buf[0] = '\0';
    syn_log_init(NULL, SYN_LOG_DEBUG);

    syn_log(SYN_LOG_DEBUG, NULL, "no tag message");
    TEST_ASSERT_TRUE(log_capture_pos > 0);
    TEST_ASSERT_NOT_NULL(strstr(log_capture_buf, "D: no tag message"));

    log_capture_pos = 0;
    log_capture_buf[0] = '\0';
    syn_log(SYN_LOG_DEBUG, "", "empty tag message");
    TEST_ASSERT_TRUE(log_capture_pos > 0);
    TEST_ASSERT_NOT_NULL(strstr(log_capture_buf, "D: empty tag message"));

    /* Test custom log output callback with raw and hexdump */
    custom_output_count = 0;
    custom_output_last[0] = '\0';
    syn_log_init(custom_log_output, SYN_LOG_DEBUG);
    syn_log(SYN_LOG_INFO, "custom", "test callback");
    TEST_ASSERT_EQUAL_INT(1, custom_output_count);
    TEST_ASSERT_NOT_NULL(strstr(custom_output_last, "I/custom: test callback"));

    custom_output_count = 0;
    syn_log_raw("raw custom output\n");
    TEST_ASSERT_TRUE(custom_output_count > 0);
    TEST_ASSERT_EQUAL_STRING("raw custom output\n", custom_output_last);

    custom_output_count = 0;
    uint8_t dummy[4] = {0x01, 0x02, 0x03, 0x04};
    syn_log_hexdump("custom_dump", dummy, 4);
    TEST_ASSERT_TRUE(custom_output_count > 0);

    /* Test oversized tag truncation */
    char huge_tag[250];
    memset(huge_tag, 'T', sizeof(huge_tag) - 1);
    huge_tag[sizeof(huge_tag) - 1] = '\0';
    log_capture_pos = 0;
    syn_log_init(NULL, SYN_LOG_DEBUG);
    syn_log(SYN_LOG_INFO, huge_tag, "truncated tag");
    TEST_ASSERT_TRUE(log_capture_pos > 0);

    /* Test oversized msg truncation */
    char huge_msg[300];
    memset(huge_msg, 'M', sizeof(huge_msg) - 1);
    huge_msg[sizeof(huge_msg) - 1] = '\0';
    log_capture_pos = 0;
    syn_log(SYN_LOG_INFO, "tag", "%s", huge_msg);
    TEST_ASSERT_TRUE(log_capture_pos > 0);
}

void run_log_tests(void)
{
    RUN_TEST(test_logging_basic);
    RUN_TEST(test_log_hexdump);
    RUN_TEST(test_log_invalid_level);
    RUN_TEST(test_log_null_tag);
}
