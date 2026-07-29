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

    /* Extended Session (Allowed from DEFAULT) */
    req[1] = SYN_UDS_SESSION_EXTENDED;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x50, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_SESSION_EXTENDED, resp[1]);
    TEST_ASSERT_EQUAL(SYN_UDS_SESSION_EXTENDED, g_uds.session);
    TEST_ASSERT_EQUAL_UINT32(5000, g_uds.s3_timer_ms);

    /* Programming Session (Allowed from EXTENDED) */
    req[1] = SYN_UDS_SESSION_PROGRAMMING;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x50, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_SESSION_PROGRAMMING, resp[1]);
    TEST_ASSERT_EQUAL(SYN_UDS_SESSION_PROGRAMMING, g_uds.session);

    /* Extended Session (REJECTED from PROGRAMMING -> NRC 0x22) */
    req[1] = SYN_UDS_SESSION_EXTENDED;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp[2]);

    /* Default Session resets security lock and S3 timer */
    g_uds.security_state = SYN_UDS_SECURITY_UNLOCKED;
    req[1] = SYN_UDS_SESSION_DEFAULT;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x50, resp[0]);
    TEST_ASSERT_EQUAL(SYN_UDS_SESSION_DEFAULT, g_uds.session);
    TEST_ASSERT_EQUAL(SYN_UDS_SECURITY_LOCKED, g_uds.security_state);
    TEST_ASSERT_EQUAL_UINT32(0, g_uds.s3_timer_ms);

    /* Programming Session (REJECTED directly from DEFAULT -> NRC 0x22) */
    req[1] = SYN_UDS_SESSION_PROGRAMMING;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp[2]);
}

static void test_uds_s3_timer_tick(void)
{
    syn_uds_init(&g_uds);
    uint8_t req[16] = {0};
    uint8_t resp[16] = {0};
    uint16_t resp_len = 0;

    /* Transition to EXTENDED session */
    req[0] = SYN_UDS_SID_DIAGNOSTIC_SESSION_CONTROL;
    req[1] = SYN_UDS_SESSION_EXTENDED;
    syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len);
    g_uds.security_state = SYN_UDS_SECURITY_UNLOCKED;
    TEST_ASSERT_EQUAL(SYN_UDS_SESSION_EXTENDED, g_uds.session);

    /* Advance S3 timer by 2000ms -> timer remains active */
    syn_uds_tick(&g_uds, 2000);
    TEST_ASSERT_EQUAL_UINT32(3000, g_uds.s3_timer_ms);
    TEST_ASSERT_EQUAL(SYN_UDS_SESSION_EXTENDED, g_uds.session);

    /* TesterPresent resets S3 timer back to 5000ms */
    req[0] = SYN_UDS_SID_TESTER_PRESENT;
    req[1] = 0x00;
    syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len);
    TEST_ASSERT_EQUAL_UINT32(5000, g_uds.s3_timer_ms);

    /* Advance S3 timer past 5000ms -> expires, drops to DEFAULT and locks security */
    syn_uds_tick(&g_uds, 5000);
    TEST_ASSERT_EQUAL(SYN_UDS_SESSION_DEFAULT, g_uds.session);
    TEST_ASSERT_EQUAL(SYN_UDS_SECURITY_LOCKED, g_uds.security_state);
    TEST_ASSERT_EQUAL_UINT32(0, g_uds.s3_timer_ms);

    /* Tick when already DEFAULT or NULL is a no-op */
    syn_uds_tick(&g_uds, 1000);
    syn_uds_tick(NULL, 1000);
}

static void test_uds_security_access(void)
{
    syn_uds_init(&g_uds);
    uint8_t req[16] = {0};
    uint8_t resp[16] = {0};
    uint16_t resp_len = 0;

    /* Power-on delay active (10000ms): Request Seed fails with NRC 0x37 */
    req[0] = SYN_UDS_SID_SECURITY_ACCESS;
    req[1] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED, resp[2]);

    /* Tick 10s to expire power-on safety delay */
    syn_uds_tick(&g_uds, 10000);

    /* Request Seed now succeeds */
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x67, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, resp[1]);
    TEST_ASSERT_EQUAL(SYN_UDS_SECURITY_SEED_SENT, g_uds.security_state);

    /* Send Key (Invalid key #1) */
    req[1] = 0x02;
    req[2] = 0x00;
    req[3] = 0x00;
    req[4] = 0x00;
    req[5] = 0x00;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 6, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INVALID_KEY, resp[2]);
    TEST_ASSERT_EQUAL(SYN_UDS_SECURITY_LOCKED, g_uds.security_state);

    /* Seed #2 + Invalid key #2 */
    req[1] = 0x01;
    syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len);
    req[1] = 0x02;
    syn_uds_process_request(&g_uds, req, 6, resp, sizeof(resp), &resp_len);

    /* Seed #3 + Invalid key #3 -> Exceeds max attempts (3) returning NRC 0x36 */
    req[1] = 0x01;
    syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len);
    req[1] = 0x02;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 6, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_EXCEEDED_NUMBER_OF_ATTEMPTS, resp[2]);

    /* Subsequent Request Seed during 10s lockout fails with NRC 0x37 */
    req[1] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED, resp[2]);

    /* Tick 10s to expire lockout timer */
    syn_uds_tick(&g_uds, 10000);

    /* Seed request succeeds after lockout expiration (error count decremented to 2) */
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x67, resp[0]);

    /* Invalid key during this post-lockout attempt -> Error count returns to 3, re-triggering 10s
     * lockout & NRC 0x36 */
    req[1] = 0x02;
    req[2] = 0x00;
    req[3] = 0x00;
    req[4] = 0x00;
    req[5] = 0x00;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 6, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_EXCEEDED_NUMBER_OF_ATTEMPTS, resp[2]);

    /* Request Seed during re-triggered 10s lockout fails with NRC 0x37 */
    req[1] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED, resp[2]);

    /* Expire 10s lockout again */
    syn_uds_tick(&g_uds, 10000);
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x67, resp[0]);

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
    TEST_ASSERT_EQUAL_UINT8(0, g_uds.security_error_count);

    /* Send Key when seed not sent -> Conditions not correct */
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 6, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp[2]);

    /* Programming session mode transition bypasses power-on delay timer */
    syn_uds_init(&g_uds);
    uint8_t sess_ext[2] = {SYN_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, SYN_UDS_SESSION_EXTENDED};
    syn_uds_process_request(&g_uds, sess_ext, 2, resp, sizeof(resp), &resp_len);
    uint8_t sess_prog[2] = {SYN_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, SYN_UDS_SESSION_PROGRAMMING};
    syn_uds_process_request(&g_uds, sess_prog, 2, resp, sizeof(resp), &resp_len);
    req[1] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x67, resp[0]);

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

static bool mock_comm_control_handler(SYN_UDS_CommControlType control_type, uint8_t comm_type,
                                      void *ctx)
{
    (void)ctx;
    if (control_type == SYN_UDS_COMM_DISABLE_RX_AND_TX && comm_type == 0xFF) {
        return false; /* Simulate rejection */
    }
    return true;
}

static void test_uds_communication_control(void)
{
    syn_uds_init(&g_uds);
    uint8_t req[16] = {0};
    uint8_t resp[16] = {0};
    uint16_t resp_len = 0;

    /* 1. Request in DEFAULT session -> Conditions not correct (NRC 0x22) */
    req[0] = SYN_UDS_SID_COMMUNICATION_CONTROL;
    req[1] = SYN_UDS_COMM_ENABLE_RX_AND_TX;
    req[2] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp[2]);

    /* Switch to EXTENDED session */
    uint8_t sess_ext[2] = {SYN_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, SYN_UDS_SESSION_EXTENDED};
    syn_uds_process_request(&g_uds, sess_ext, 2, resp, sizeof(resp), &resp_len);

    /* 2. Short length check (< 3) -> NRC 0x13 */
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    /* 3. Invalid subfunction (> 5) -> NRC 0x12 */
    req[1] = 0x06;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, resp[2]);

    /* 4. Zero comm_type -> NRC 0x31 */
    req[1] = SYN_UDS_COMM_ENABLE_RX_AND_TX;
    req[2] = 0x00;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_REQUEST_OUT_OF_RANGE, resp[2]);

    /* 5. Successful CommunicationControl subfunctions 0x00..0x05 */
    for (uint8_t sub = 0; sub <= 5; sub++) {
        req[1] = sub;
        req[2] = 0x01;
        TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 3, resp, sizeof(resp), &resp_len));
        TEST_ASSERT_EQUAL_HEX8(0x68, resp[0]);
        TEST_ASSERT_EQUAL_HEX8(sub, resp[1]);
        TEST_ASSERT_EQUAL(sub, g_uds.comm_control_state);
        TEST_ASSERT_EQUAL_HEX8(0x01, g_uds.comm_type);
    }

    /* 6. Custom handler registration & failure case */
    TEST_ASSERT_FALSE(syn_uds_register_comm_control(NULL, mock_comm_control_handler, NULL));
    TEST_ASSERT_TRUE(syn_uds_register_comm_control(&g_uds, mock_comm_control_handler, NULL));

    req[1] = SYN_UDS_COMM_DISABLE_RX_AND_TX;
    req[2] = 0xFF;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp[2]);

    /* 7. S3 timeout reverts comm control state back to ENABLE_RX_AND_TX (0x00) */
    syn_uds_tick(&g_uds, 5000);
    TEST_ASSERT_EQUAL(SYN_UDS_SESSION_DEFAULT, g_uds.session);
    TEST_ASSERT_EQUAL(SYN_UDS_COMM_ENABLE_RX_AND_TX, g_uds.comm_control_state);
}

static bool mock_timing_handler(SYN_UDS_AccessTimingType timing_type, uint16_t *p2_max_ms,
                                uint16_t *p2_star_max_10ms, void *ctx)
{
    (void)p2_star_max_10ms;
    (void)ctx;
    if (timing_type == SYN_UDS_TIMING_SET_TO_GIVEN) {
        if (*p2_max_ms > 1000U) {
            return false; /* Reject values out of allowed range */
        }
    }
    return true;
}

static void test_uds_access_timing_parameter(void)
{
    syn_uds_init(&g_uds);
    uint8_t req[16] = {0};
    uint8_t resp[16] = {0};
    uint16_t resp_len = 0;

    /* 1. Null server check */
    TEST_ASSERT_FALSE(syn_uds_register_access_timing(NULL, mock_timing_handler, NULL));

    /* 2. Short request -> NRC 0x13 */
    req[0] = SYN_UDS_SID_ACCESS_TIMING_PARAMETER;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    /* 3. Invalid subfunction -> NRC 0x12 */
    req[1] = 0x05;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, resp[2]);

    /* 4. Subfunction 0x01: readExtendedTimingParameterSet */
    req[1] = SYN_UDS_TIMING_READ_EXTENDED;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0xC3, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_TIMING_READ_EXTENDED, resp[1]);
    TEST_ASSERT_EQUAL_UINT16(6, resp_len);
    TEST_ASSERT_EQUAL_HEX8(0x00, resp[2]);
    TEST_ASSERT_EQUAL_HEX8(0x32, resp[3]); /* 50 ms */
    TEST_ASSERT_EQUAL_HEX8(0x01, resp[4]);
    TEST_ASSERT_EQUAL_HEX8(0xF4, resp[5]); /* 500 (5000 ms) */

    /* 5. Subfunction 0x01 wrong length -> NRC 0x13 */
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    /* 6. Subfunction 0x04: setTimingParametersToGivenValues (P2=100ms, P2*=1000) */
    req[1] = SYN_UDS_TIMING_SET_TO_GIVEN;
    req[2] = 0x00;
    req[3] = 0x64; /* 100 ms */
    req[4] = 0x03;
    req[5] = 0xE8; /* 1000 (10000 ms) */
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 6, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0xC3, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_TIMING_SET_TO_GIVEN, resp[1]);
    TEST_ASSERT_EQUAL_UINT16(2, resp_len);
    TEST_ASSERT_EQUAL_UINT16(100, g_uds.active_p2_max_ms);
    TEST_ASSERT_EQUAL_UINT16(1000, g_uds.active_p2_star_max_10ms);

    /* 7. Subfunction 0x03: readCurrentlyActiveTimingParameterSet */
    req[1] = SYN_UDS_TIMING_READ_ACTIVE;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0xC3, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_TIMING_READ_ACTIVE, resp[1]);
    TEST_ASSERT_EQUAL_UINT16(6, resp_len);
    TEST_ASSERT_EQUAL_HEX8(0x00, resp[2]);
    TEST_ASSERT_EQUAL_HEX8(0x64, resp[3]); /* 100 ms */
    TEST_ASSERT_EQUAL_HEX8(0x03, resp[4]);
    TEST_ASSERT_EQUAL_HEX8(0xE8, resp[5]); /* 1000 */

    /* 8. Subfunction 0x02: setTimingParametersToDefaultValues */
    req[1] = SYN_UDS_TIMING_SET_TO_DEFAULT;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0xC3, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_TIMING_SET_TO_DEFAULT, resp[1]);
    TEST_ASSERT_EQUAL_UINT16(2, resp_len);
    TEST_ASSERT_EQUAL_UINT16(50, g_uds.active_p2_max_ms);
    TEST_ASSERT_EQUAL_UINT16(500, g_uds.active_p2_star_max_10ms);

    /* 9. Custom callback rejection */
    TEST_ASSERT_TRUE(syn_uds_register_access_timing(&g_uds, mock_timing_handler, NULL));
    req[1] = SYN_UDS_TIMING_SET_TO_GIVEN;
    req[2] = 0x05;
    req[3] = 0x00; /* P2 = 1280 ms > 1000 -> Handler rejects */
    req[4] = 0x03;
    req[5] = 0xE8;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 6, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_REQUEST_OUT_OF_RANGE, resp[2]);
}

static bool mock_secured_data_handler(const uint8_t *in_data, uint16_t in_len, uint8_t *out_buf,
                                      uint16_t max_out_len, uint16_t *out_len, void *ctx)
{
    (void)ctx;
    (void)max_out_len;
    if (in_len == 0 || in_data[0] == 0xFF) {
        return false; /* Rejection case */
    }
    /* Simple XOR cipher for testing */
    for (uint16_t i = 0; i < in_len; i++) {
        out_buf[i] = in_data[i] ^ 0x5AU;
    }
    *out_len = in_len;
    return true;
}

static void test_uds_secured_data_transmission(void)
{
    syn_uds_init(&g_uds);
    uint8_t req[16] = {0};
    uint8_t resp[16] = {0};
    uint16_t resp_len = 0;

    /* 1. Null server check */
    TEST_ASSERT_FALSE(syn_uds_register_secured_data(NULL, mock_secured_data_handler, NULL));

    /* 2. Short request (req_len < 2) -> NRC 0x13 */
    req[0] = SYN_UDS_SID_SECURED_DATA_TRANSMISSION;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    /* 3. Locked state -> NRC 0x33 (Security Access Denied) */
    req[1] = 0x12;
    req[2] = 0x34;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_SECURITY_ACCESS_DENIED, resp[2]);

    /* 4. Unlock security & test default echo response (no callback) */
    g_uds.security_state = SYN_UDS_SECURITY_UNLOCKED;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0xC4, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x12, resp[1]);
    TEST_ASSERT_EQUAL_HEX8(0x34, resp[2]);
    TEST_ASSERT_EQUAL_UINT16(3, resp_len);

    /* 5. Custom handler registration & encrypted payload transformation */
    TEST_ASSERT_TRUE(syn_uds_register_secured_data(&g_uds, mock_secured_data_handler, NULL));
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0xC4, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x12 ^ 0x5A, resp[1]);
    TEST_ASSERT_EQUAL_HEX8(0x34 ^ 0x5A, resp[2]);

    /* 6. Handler rejection -> NRC 0x22 */
    req[1] = 0xFF;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp[2]);
}

static void test_uds_extended_sids(void)
{
    syn_uds_init(&g_uds);
    uint8_t req[16] = {0};
    uint8_t resp[16] = {0};
    uint16_t resp_len = 0;

    /* ClearDiagnosticInformation (0x14) */
    req[0] = SYN_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION;
    req[1] = 0xFF;
    req[2] = 0xFF;
    req[3] = 0xFF;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x54, resp[0]);

    /* ReadDTCInformation (0x19) */
    req[0] = SYN_UDS_SID_READ_DTC_INFORMATION;
    req[1] = 0x02;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x59, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x02, resp[1]);

    /* ControlDTCSetting (0x85) */
    req[0] = SYN_UDS_SID_CONTROL_DTC_SETTING;
    req[1] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0xC5, resp[0]);

    /* ResponseOnEvent (0x86) */
    req[0] = SYN_UDS_SID_RESPONSE_ON_EVENT;
    req[1] = 0x00;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0xC6, resp[0]);

    /* Transfer Data Services (0x34, 0x35, 0x36, 0x37) */
    g_uds.security_state = SYN_UDS_SECURITY_UNLOCKED;
    req[0] = SYN_UDS_SID_REQUEST_DOWNLOAD;
    req[1] = 0x00;
    req[2] = 0x44;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x74, resp[0]);

    req[0] = SYN_UDS_SID_TRANSFER_DATA;
    req[1] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x76, resp[0]);

    req[0] = SYN_UDS_SID_REQUEST_TRANSFER_EXIT;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x77, resp[0]);
}

void run_uds_tests(void)
{
    RUN_TEST(test_uds_init_and_sessions);
    RUN_TEST(test_uds_s3_timer_tick);
    RUN_TEST(test_uds_security_access);
    RUN_TEST(test_uds_communication_control);
    RUN_TEST(test_uds_access_timing_parameter);
    RUN_TEST(test_uds_secured_data_transmission);
    RUN_TEST(test_uds_extended_sids);
    RUN_TEST(test_uds_did_read_write);
    RUN_TEST(test_uds_ecu_reset_routine_tester_present);
    RUN_TEST(test_uds_bounds_and_null_checks);
}
