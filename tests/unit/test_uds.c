/**
 * @file test_uds.c
 * @brief Unit tests for ISO 14229 UDS server protocol implementation.
 */

#include "syntropic/proto/syn_uds.h"
#include "unity/unity.h"

#include <string.h>

static SYN_UDS_Server g_uds;
static uint8_t g_test_did_data[4] = {0xAA, 0xBB, 0xCC, 0xDD};

static void test_uds_init_and_sessions(void)
{
    TEST_ASSERT_FALSE(syn_uds_init(NULL));
    TEST_ASSERT_TRUE(syn_uds_init(&g_uds));
    TEST_ASSERT_EQUAL(SYN_UDS_SESSION_DEFAULT, g_uds.session);
    TEST_ASSERT_EQUAL(SYN_UDS_SECURITY_LOCKED, g_uds.security_state);

    uint8_t req[16] = {0};
    uint8_t resp[16] = {0};
    uint16_t resp_len = 0;

    /* Short request -> Incorrect message length */
    req[0] = SYN_UDS_SID_DIAGNOSTIC_SESSION_CONTROL;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    /* Invalid subfunction */
    req[1] = 0x99;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, resp[2]);

    /* Extended Session */
    req[1] = SYN_UDS_SESSION_EXTENDED;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x50, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_SESSION_EXTENDED, resp[1]);
    TEST_ASSERT_EQUAL(SYN_UDS_SESSION_EXTENDED, g_uds.session);

    /* Programming Session */
    req[1] = SYN_UDS_SESSION_PROGRAMMING;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x50, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_SESSION_PROGRAMMING, resp[1]);
    TEST_ASSERT_EQUAL(SYN_UDS_SESSION_PROGRAMMING, g_uds.session);

    /* Default Session resets security lock */
    g_uds.security_state = SYN_UDS_SECURITY_UNLOCKED;
    req[1] = SYN_UDS_SESSION_DEFAULT;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x50, resp[0]);
    TEST_ASSERT_EQUAL(SYN_UDS_SESSION_DEFAULT, g_uds.session);
    TEST_ASSERT_EQUAL(SYN_UDS_SECURITY_LOCKED, g_uds.security_state);
}

static void test_uds_security_access(void)
{
    syn_uds_init(&g_uds);
    uint8_t req[16] = {0};
    uint8_t resp[16] = {0};
    uint16_t resp_len = 0;

    /* Request Seed */
    req[0] = SYN_UDS_SID_SECURITY_ACCESS;
    req[1] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x67, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, resp[1]);
    TEST_ASSERT_EQUAL(SYN_UDS_SECURITY_SEED_SENT, g_uds.security_state);

    /* Send Key (Invalid key) */
    req[1] = 0x02;
    req[2] = 0x00;
    req[3] = 0x00;
    req[4] = 0x00;
    req[5] = 0x00;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 6, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INVALID_KEY, resp[2]);
    TEST_ASSERT_EQUAL(SYN_UDS_SECURITY_LOCKED, g_uds.security_state);

    /* Request Seed again */
    req[1] = 0x01;
    syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len);

    /* Send Key (Correct expected key = seed ^ 0xA5A5A5A5) */
    uint32_t expected_key = g_uds.current_seed ^ 0xA5A5A5A5U;
    req[1] = 0x02;
    req[2] = (uint8_t)(expected_key >> 24);
    req[3] = (uint8_t)(expected_key >> 16);
    req[4] = (uint8_t)(expected_key >> 8);
    req[5] = (uint8_t)(expected_key);
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 6, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x67, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x02, resp[1]);
    TEST_ASSERT_EQUAL(SYN_UDS_SECURITY_UNLOCKED, g_uds.security_state);

    /* Send Key when seed not sent -> Conditions not correct */
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 6, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp[2]);

    /* Invalid subfunction */
    req[1] = 0xFF;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, resp[2]);
}

static void test_uds_did_read_write(void)
{
    syn_uds_init(&g_uds);
    TEST_ASSERT_FALSE(syn_uds_register_did(NULL, 0x1234, g_test_did_data, 4, true));
    TEST_ASSERT_TRUE(syn_uds_register_did(&g_uds, 0x1234, g_test_did_data, 4, true));

    uint8_t req[16] = {0};
    uint8_t resp[16] = {0};
    uint16_t resp_len = 0;

    /* Read DID 0x1234 */
    req[0] = SYN_UDS_SID_READ_DATA_BY_IDENTIFIER;
    req[1] = 0x12;
    req[2] = 0x34;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x62, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x12, resp[1]);
    TEST_ASSERT_EQUAL_HEX8(0x34, resp[2]);
    TEST_ASSERT_EQUAL_HEX8(0xAA, resp[3]);
    TEST_ASSERT_EQUAL_HEX8(0xDD, resp[6]);

    /* Read non-existent DID -> Request out of range */
    req[1] = 0x99;
    req[2] = 0x99;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_REQUEST_OUT_OF_RANGE, resp[2]);

    /* Write DID when locked -> Security access denied */
    req[0] = SYN_UDS_SID_WRITE_DATA_BY_IDENTIFIER;
    req[1] = 0x12;
    req[2] = 0x34;
    req[3] = 0x11;
    req[4] = 0x22;
    req[5] = 0x33;
    req[6] = 0x44;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 7, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_SECURITY_ACCESS_DENIED, resp[2]);

    /* SecurityAccess short requests */
    req[0] = SYN_UDS_SID_SECURITY_ACCESS;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);

    req[1] = 0x02;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 5, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);

    /* Unlock security */
    g_uds.security_state = SYN_UDS_SECURITY_UNLOCKED;

    /* Write DID when unlocked -> Success */
    req[0] = SYN_UDS_SID_WRITE_DATA_BY_IDENTIFIER;
    req[1] = 0x12;
    req[2] = 0x34;
    req[3] = 0x11;
    req[4] = 0x22;
    req[5] = 0x33;
    req[6] = 0x44;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 7, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x6E, resp[0]);

    /* Write DID with extra trailing bytes (truncated write) */
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 10, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x6E, resp[0]);

    /* Write non-existent DID -> Request out of range */
    req[1] = 0x99;
    req[2] = 0x99;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 7, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_REQUEST_OUT_OF_RANGE, resp[2]);
    /* Read-only DID write attempt */
    syn_uds_register_did(&g_uds, 0x5555, g_test_did_data, 4, false);
    req[0] = SYN_UDS_SID_WRITE_DATA_BY_IDENTIFIER;
    req[1] = 0x55;
    req[2] = 0x55;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 7, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp[2]);

    /* Response too long for small buffer */
    req[0] = SYN_UDS_SID_READ_DATA_BY_IDENTIFIER;
    req[1] = 0x12;
    req[2] = 0x34;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 3, resp, 5, &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_RESPONSE_TOO_LONG, resp[2]);
}

static void test_uds_ecu_reset_routine_tester_present(void)
{
    syn_uds_init(&g_uds);
    uint8_t req[16] = {0};
    uint8_t resp[16] = {0};
    uint16_t resp_len = 0;

    /* ECUReset short req vs invalid subfunction */
    req[0] = SYN_UDS_SID_ECU_RESET;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);

    req[1] = 0x99;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);

    /* ECUReset Hard reset */
    req[1] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x51, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, resp[1]);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_uds.reset_type_requested);

    /* RoutineControl short req */
    req[0] = SYN_UDS_SID_ROUTINE_CONTROL;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);

    req[1] = 0x01;
    req[2] = 0x02;
    req[3] = 0x03;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x71, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, resp[1]);

    /* TesterPresent short req */
    req[0] = SYN_UDS_SID_TESTER_PRESENT;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);

    req[1] = 0x00;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x7E, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, resp[1]);

    /* RDBI & WDBI short req */
    req[0] = SYN_UDS_SID_READ_DATA_BY_IDENTIFIER;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);

    req[0] = SYN_UDS_SID_WRITE_DATA_BY_IDENTIFIER;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);

    /* Unsupported SID */
    req[0] = 0xBA;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_SERVICE_NOT_SUPPORTED, resp[2]);
}

static void test_uds_bounds_and_null_checks(void)
{
    uint8_t req[16] = {0};
    uint8_t resp[16] = {0};
    uint16_t resp_len = 0;

    TEST_ASSERT_FALSE(syn_uds_process_request(NULL, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_FALSE(syn_uds_process_request(&g_uds, NULL, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_FALSE(syn_uds_process_request(&g_uds, req, 0, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_FALSE(syn_uds_process_request(&g_uds, req, 2, NULL, sizeof(resp), &resp_len));
    TEST_ASSERT_FALSE(syn_uds_process_request(&g_uds, req, 2, resp, 2, &resp_len));
    TEST_ASSERT_FALSE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), NULL));

    /* Register DID overflow */
    syn_uds_init(&g_uds);
    for (size_t i = 0; i < SYN_UDS_MAX_DIDS; i++) {
        TEST_ASSERT_TRUE(syn_uds_register_did(&g_uds, (uint16_t)i, g_test_did_data, 4, true));
    }
    TEST_ASSERT_FALSE(syn_uds_register_did(&g_uds, 0x9999, g_test_did_data, 4, true));
}

void run_uds_tests(void)
{
    RUN_TEST(test_uds_init_and_sessions);
    RUN_TEST(test_uds_security_access);
    RUN_TEST(test_uds_did_read_write);
    RUN_TEST(test_uds_ecu_reset_routine_tester_present);
    RUN_TEST(test_uds_bounds_and_null_checks);
}
