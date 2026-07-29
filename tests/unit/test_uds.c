/**
 * @file test_uds.c
 * @brief Unit tests for ISO 14229 UDS server protocol implementation.
 */

#include "syntropic/proto/syn_uds.h"
#include "syntropic/util/syn_pack.h"
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
    req[2] = 0xFF;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 3, resp, sizeof(resp), &resp_len));
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
    req[2] = 0x00;
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

static bool mock_memory_cb(bool is_write, uint32_t address, uint32_t size, uint8_t *data_buf,
                           void *ctx)
{
    (void)address;
    (void)size;
    (void)ctx;
    if (is_write) {
        data_buf[0] = 0xAA;
    } else {
        data_buf[0] = 0xBB;
    }
    return true;
}

static bool mock_auth_cb(uint8_t subfunction, const uint8_t *in_data, uint16_t in_len,
                         uint8_t *out_buf, uint16_t max_out_len, uint16_t *out_len, void *ctx)
{
    (void)subfunction;
    (void)in_data;
    (void)in_len;
    (void)max_out_len;
    (void)ctx;
    out_buf[0] = 0x99;
    *out_len = 1;
    return true;
}

static bool mock_file_transfer_cb(uint8_t mode, const char *file_path, uint16_t path_len,
                                  uint8_t *out_buf, uint16_t max_out_len, uint16_t *out_len,
                                  void *ctx)
{
    (void)mode;
    (void)file_path;
    (void)path_len;
    (void)max_out_len;
    (void)ctx;
    out_buf[0] = 0x10;
    *out_len = 1;
    return true;
}

static void test_uds_complete_27_sids(void)
{
    syn_uds_init(&g_uds);
    uint8_t req[32] = {0};
    uint8_t resp[32] = {0};
    uint16_t resp_len = 0;

    /* 1. Null registration checks */
    TEST_ASSERT_FALSE(syn_uds_register_memory_handler(NULL, mock_memory_cb, NULL));
    TEST_ASSERT_FALSE(syn_uds_register_auth_handler(NULL, mock_auth_cb, NULL));
    TEST_ASSERT_FALSE(syn_uds_register_file_transfer(NULL, mock_file_transfer_cb, NULL));

    TEST_ASSERT_TRUE(syn_uds_register_memory_handler(&g_uds, mock_memory_cb, NULL));
    TEST_ASSERT_TRUE(syn_uds_register_auth_handler(&g_uds, mock_auth_cb, NULL));
    TEST_ASSERT_TRUE(syn_uds_register_file_transfer(&g_uds, mock_file_transfer_cb, NULL));

    /* 2. ReadMemoryByAddress (0x23) */
    req[0] = SYN_UDS_SID_READ_MEMORY_BY_ADDRESS;
    req[1] = 0x12; /* 2-byte addr, 1-byte size */
    req[2] = 0x10;
    req[3] = 0x00;
    req[4] = 0x04;
    /* Locked state -> NRC 0x33 */
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 5, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_SECURITY_ACCESS_DENIED, resp[2]);

    g_uds.security_state = SYN_UDS_SECURITY_UNLOCKED;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 5, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x63, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, resp[1]);

    /* 3. ReadScalingDataByIdentifier (0x24) */
    req[0] = SYN_UDS_SID_READ_SCALING_DATA_BY_IDENTIFIER;
    req[1] = 0xF1;
    req[2] = 0x90;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x64, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0xF1, resp[1]);
    TEST_ASSERT_EQUAL_HEX8(0x90, resp[2]);
    TEST_ASSERT_EQUAL_HEX8(0x01, resp[3]);

    /* 4. Authentication (0x29) */
    req[0] = SYN_UDS_SID_AUTHENTICATION;
    req[1] = 0x00; /* deAuthenticate */
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x69, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, resp[1]);
    TEST_ASSERT_EQUAL_HEX8(0x99, resp[2]);

    /* 5. ReadDataByPeriodicIdentifier (0x2A) */
    req[0] = SYN_UDS_SID_READ_DATA_BY_PERIODIC_IDENTIFIER;
    req[1] = 0x01; /* Fast mode */
    req[2] = 0xE0; /* Periodic DID */
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x6A, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0xE0, resp[1]);

    /* 6. DynamicallyDefineDataIdentifier (0x2C) */
    req[0] = SYN_UDS_SID_DYNAMICALLY_DEFINE_DATA_IDENTIFIER;
    req[1] = 0x01; /* defineByIdentifier */
    req[2] = 0xF2;
    req[3] = 0x00;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x6C, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, resp[1]);
    TEST_ASSERT_EQUAL_HEX8(0xF2, resp[2]);
    TEST_ASSERT_EQUAL_HEX8(0x00, resp[3]);

    /* 7. InputOutputControlByIdentifier (0x2F) */
    req[0] = SYN_UDS_SID_INPUT_OUTPUT_CONTROL_BY_IDENTIFIER;
    req[1] = 0xF1;
    req[2] = 0x90;
    req[3] = 0x03; /* Short term adjustment */
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x6F, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0xF1, resp[1]);
    TEST_ASSERT_EQUAL_HEX8(0x90, resp[2]);
    TEST_ASSERT_EQUAL_HEX8(0x03, resp[3]);

    /* 8. RequestFileTransfer (0x38) */
    req[0] = SYN_UDS_SID_REQUEST_FILE_TRANSFER;
    req[1] = 0x01; /* AddFile */
    req[2] = '/';
    req[3] = 'a';
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x78, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, resp[1]);
    TEST_ASSERT_EQUAL_HEX8(0x10, resp[2]);

    /* 9. WriteMemoryByAddress (0x3D) */
    req[0] = SYN_UDS_SID_WRITE_MEMORY_BY_ADDRESS;
    req[1] = 0x11; /* 1-byte addr, 1-byte size */
    req[2] = 0x20;
    req[3] = 0x01;
    req[4] = 0x55;
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 5, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x7D, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x11, resp[1]);

    /* 10. LinkControl (0x87) */
    req[0] = SYN_UDS_SID_LINK_CONTROL;
    req[1] = 0x01; /* verifyBaudrateTransitionWithFixedBaudrate */
    TEST_ASSERT_TRUE(syn_uds_process_request(&g_uds, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0xC7, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, resp[1]);
}

static bool mock_dtc_custom_cb(uint8_t subfunction, const uint8_t *in_data, uint16_t in_len,
                               uint8_t *out_buf, uint16_t max_out_len, uint16_t *out_len, void *ctx)
{
    (void)in_data;
    (void)in_len;
    (void)max_out_len;
    (void)ctx;
    if (subfunction == 0x03) {
        out_buf[0] = 0x01;
        out_buf[1] = 0x23;
        out_buf[2] = 0x45;
        out_buf[3] = 0x01;
        *out_len = 4;
        return true;
    }
    return false;
}

static void test_uds_read_dtc_information_subfunctions(void)
{
    SYN_UDS_Server server;
    TEST_ASSERT_TRUE(syn_uds_init(&server));

    /* Register test DTCs */
    TEST_ASSERT_TRUE(
        syn_uds_register_dtc(&server, 0x012345U, 0x09U, 0x60U)); /* testFailed | confirmedDTC */
    TEST_ASSERT_TRUE(syn_uds_register_dtc(&server, 0x023456U, 0x01U, 0x80U)); /* testFailed */
    TEST_ASSERT_TRUE(syn_uds_register_dtc(&server, 0x034567U, 0x00U, 0x10U)); /* clear */

    uint8_t req[16] = {0};
    uint8_t resp[32] = {0};
    uint16_t resp_len = 0;

    /* 1. reportNumberOfDTCByStatusMask (0x01) */
    req[0] = SYN_UDS_SID_READ_DTC_INFORMATION;
    req[1] = 0x01;
    req[2] = 0x01; /* Mask: testFailed */
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x59, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, resp[1]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, resp[2]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_DTC_FORMAT_ISO14229_1, resp[3]);
    TEST_ASSERT_EQUAL_UINT16(2, ((uint16_t)resp[4] << 8) | resp[5]);

    /* 2. reportDTCByStatusMask (0x02) */
    req[1] = 0x02;
    req[2] = 0x01; /* Mask: testFailed */
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x59, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x02, resp[1]);
    TEST_ASSERT_EQUAL_UINT16(11, resp_len); /* 3 + 4 * 2 */

    /* 3. reportSupportedDTC (0x0A) */
    req[1] = 0x0A;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x59, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x0A, resp[1]);
    TEST_ASSERT_EQUAL_UINT16(15, resp_len); /* 3 + 4 * 3 */

    /* 4. reportFirstTestFailedDTC (0x0B) */
    req[1] = 0x0B;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x59, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x0B, resp[1]);
    TEST_ASSERT_EQUAL_HEX8(0x01, resp[3]);
    TEST_ASSERT_EQUAL_HEX8(0x23, resp[4]);

    /* 5. reportFirstConfirmedDTC (0x0C) */
    req[1] = 0x0C;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x59, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x0C, resp[1]);
    TEST_ASSERT_EQUAL_HEX8(0x01, resp[3]);
    TEST_ASSERT_EQUAL_HEX8(0x23, resp[4]);

    /* 6. reportNumberOfDTCBySeverityMaskRecord (0x07) */
    req[1] = 0x07;
    req[2] = 0x60; /* Severity mask */
    req[3] = 0x01; /* Status mask */
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x59, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x07, resp[1]);
    TEST_ASSERT_EQUAL_UINT16(1, ((uint16_t)resp[4] << 8) | resp[5]);

    /* 7. reportDTCBySeverityMaskRecord (0x08) */
    req[1] = 0x08;
    req[2] = 0x60;
    req[3] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x59, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x08, resp[1]);
    TEST_ASSERT_EQUAL_UINT16(9, resp_len); /* 3 + 6 * 1 */

    /* 8. Custom DTC callback (0x03) */
    syn_uds_register_dtc_handler(&server, mock_dtc_custom_cb, NULL);
    req[1] = 0x03;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x59, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x03, resp[1]);
    TEST_ASSERT_EQUAL_HEX8(0x01, resp[2]);
    TEST_ASSERT_EQUAL_HEX8(0x23, resp[3]);
}

static void test_uds_stateful_data_transfer_sequence(void)
{
    SYN_UDS_Server server;
    TEST_ASSERT_TRUE(syn_uds_init(&server));
    server.security_state = SYN_UDS_SECURITY_UNLOCKED;

    uint8_t req[16] = {0};
    uint8_t resp[32] = {0};
    uint16_t resp_len = 0;

    /* TransferData without RequestDownload -> NRC 0x24 (requestSequenceError) */
    req[0] = SYN_UDS_SID_TRANSFER_DATA;
    req[1] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_REQUEST_SEQUENCE_ERROR, resp[2]);

    /* RequestTransferExit without active transfer -> NRC 0x24 (requestSequenceError) */
    req[0] = SYN_UDS_SID_REQUEST_TRANSFER_EXIT;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_REQUEST_SEQUENCE_ERROR, resp[2]);

    /* RequestDownload (0x34): ALFID 0x11 (1-byte addr 0x20, 1-byte size 0x10) */
    req[0] = SYN_UDS_SID_REQUEST_DOWNLOAD;
    req[1] = 0x00; /* DFI */
    req[2] = 0x11; /* ALFID */
    req[3] = 0x20; /* Addr */
    req[4] = 0x10; /* Size = 16 bytes */
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 5, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x74, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x20, resp[1]);
    TEST_ASSERT_EQUAL(SYN_UDS_TRANSFER_DOWNLOAD, server.transfer_state);
    TEST_ASSERT_EQUAL_UINT32(0x20, server.transfer_address);
    TEST_ASSERT_EQUAL_UINT32(0x10, server.transfer_size);

    /* TransferData (0x36): Sequence 0x02 instead of expected 0x01 -> NRC 0x73 */
    req[0] = SYN_UDS_SID_TRANSFER_DATA;
    req[1] = 0x02; /* Out-of-order sequence counter */
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_WRONG_BLOCK_SEQUENCE_COUNTER, resp[2]);

    /* TransferData (0x36): Sequence 0x01 (Valid 4 bytes payload) */
    req[1] = 0x01;
    req[2] = 0xDE;
    req[3] = 0xAD;
    req[4] = 0xBE;
    req[5] = 0xEF;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 6, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x76, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, resp[1]);
    TEST_ASSERT_EQUAL_UINT32(4, server.transfer_bytes_processed);

    /* RequestTransferExit (0x37): Completes download sequence and resets state to IDLE */
    req[0] = SYN_UDS_SID_REQUEST_TRANSFER_EXIT;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x77, resp[0]);
    TEST_ASSERT_EQUAL(SYN_UDS_TRANSFER_IDLE, server.transfer_state);
}

static bool mock_failing_memory_cb(bool is_write, uint32_t address, uint32_t size,
                                   uint8_t *data_buf, void *ctx)
{
    (void)is_write;
    (void)address;
    (void)size;
    (void)data_buf;
    (void)ctx;
    return false;
}

static bool mock_failing_timing_cb(SYN_UDS_AccessTimingType timing_type, uint16_t *p2_max_ms,
                                   uint16_t *p2_star_max_10ms, void *ctx)
{
    (void)timing_type;
    (void)p2_max_ms;
    (void)p2_star_max_10ms;
    (void)ctx;
    return false;
}

static void test_uds_spec_nrc_and_edge_cases(void)
{
    SYN_UDS_Server server;
    TEST_ASSERT_TRUE(syn_uds_init(&server));
    server.security_state = SYN_UDS_SECURITY_UNLOCKED;

    uint8_t req[32] = {0};
    uint8_t resp[32] = {0};
    uint16_t resp_len = 0;

    /* 1. ReadDTCInformation subfunction 0x14 (Fault detection counter) */
    TEST_ASSERT_TRUE(syn_uds_register_dtc(&server, 0x112233U, 0x01U, 0x20U));
    req[0] = SYN_UDS_SID_READ_DTC_INFORMATION;
    req[1] = 0x14;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x59, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x14, resp[1]);

    /* 2. ReadDTCInformation subfunctions 0x0D & 0x0E (Most recent failed / confirmed) */
    req[1] = 0x0D;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x59, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x0D, resp[1]);

    req[1] = 0x0E;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x59, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x0E, resp[1]);

    /* 3. AccessTimingParameter failing callback -> NRC 0x22 */
    syn_uds_register_access_timing(&server, mock_failing_timing_cb, NULL);
    req[0] = SYN_UDS_SID_ACCESS_TIMING_PARAMETER;
    req[1] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp[2]);

    /* 4. RequestDownload length check -> NRC 0x13 */
    req[0] = SYN_UDS_SID_REQUEST_DOWNLOAD;
    req[1] = 0x00;
    req[2] = 0x22; /* 2-byte addr, 2-byte size -> requires 7 bytes */
    req[3] = 0x10;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    /* 5. TransferData failing memory_cb -> NRC 0x72 */
    syn_uds_register_memory_handler(&server, mock_failing_memory_cb, NULL);
    req[0] = SYN_UDS_SID_REQUEST_DOWNLOAD;
    req[1] = 0x00;
    req[2] = 0x11; /* 1-byte addr, 1-byte size */
    req[3] = 0x10;
    req[4] = 0x04;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 5, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x74, resp[0]);

    req[0] = SYN_UDS_SID_TRANSFER_DATA;
    req[1] = 0x01;
    req[2] = 0xFF;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_GENERAL_PROGRAMMING_FAILURE, resp[2]);
}

static void test_uds_upload_sequence_and_nrc_handling(void)
{
    SYN_UDS_Server server;
    TEST_ASSERT_TRUE(syn_uds_init(&server));
    server.security_state = SYN_UDS_SECURITY_UNLOCKED;

    uint8_t req[32] = {0};
    uint8_t resp[64] = {0};
    uint16_t resp_len = 0;

    /* 1. RequestUpload (0x35) -> TransferData upload mode -> RequestTransferExit */
    req[0] = SYN_UDS_SID_REQUEST_UPLOAD;
    req[1] = 0x00; /* DFI */
    req[2] = 0x11; /* ALFID: 1-byte addr, 1-byte size */
    req[3] = 0x10; /* Addr */
    req[4] = 0x08; /* Size = 8 bytes */
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 5, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x75, resp[0]);
    TEST_ASSERT_EQUAL(SYN_UDS_TRANSFER_UPLOAD, server.transfer_state);

    /* TransferData Upload Block 1 */
    req[0] = SYN_UDS_SID_TRANSFER_DATA;
    req[1] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x76, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, resp[1]);
    TEST_ASSERT_EQUAL_UINT16(10, resp_len); /* 2 header + 8 bytes payload */

    /* Retransmit same block sequence counter 0x01 */
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x76, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, resp[1]);

    /* RequestTransferExit */
    req[0] = SYN_UDS_SID_REQUEST_TRANSFER_EXIT;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x77, resp[0]);

    /* 2. ResponseOnEvent (0x86) subfunction 0x04 (reportActivatedEvents) */
    req[0] = SYN_UDS_SID_RESPONSE_ON_EVENT;
    req[1] = 0x04;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0xC6, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x04, resp[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00, resp[2]);

    /* 3. RequestFileTransfer (0x38) without registered callback */
    req[0] = SYN_UDS_SID_REQUEST_FILE_TRANSFER;
    req[1] = 0x01;
    req[2] = 'a';
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x78, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, resp[1]);

    /* 4. ReadMemoryByAddress (0x23) without registered memory_cb */
    req[0] = SYN_UDS_SID_READ_MEMORY_BY_ADDRESS;
    req[1] = 0x11;
    req[2] = 0x10;
    req[3] = 0x04;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x63, resp[0]);
    TEST_ASSERT_EQUAL_UINT16(5, resp_len);

    /* 5. DynamicallyDefineDataIdentifier (0x2C) invalid subfunction -> NRC 0x12 */
    req[0] = SYN_UDS_SID_DYNAMICALLY_DEFINE_DATA_IDENTIFIER;
    req[1] = 0x05;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, resp[2]);

    /* 6. ReadDataByPeriodicIdentifier (0x2A) invalid mode -> NRC 0x31 */
    req[0] = SYN_UDS_SID_READ_DATA_BY_PERIODIC_IDENTIFIER;
    req[1] = 0x05;
    req[2] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_REQUEST_OUT_OF_RANGE, resp[2]);

    /* 7. WriteMemoryByAddress (0x3D) when security is locked -> NRC 0x33 */
    server.security_state = SYN_UDS_SECURITY_LOCKED;
    req[0] = SYN_UDS_SID_WRITE_MEMORY_BY_ADDRESS;
    req[1] = 0x11;
    req[2] = 0x10;
    req[3] = 0x04;
    req[4] = 0x55;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 5, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_SECURITY_ACCESS_DENIED, resp[2]);
}

static bool mock_failing_file_transfer_cb(uint8_t mode, const char *file_path, uint16_t path_len,
                                          uint8_t *out_buf, uint16_t max_out_len, uint16_t *out_len,
                                          void *ctx)
{
    (void)mode;
    (void)file_path;
    (void)path_len;
    (void)out_buf;
    (void)max_out_len;
    (void)out_len;
    (void)ctx;
    return false;
}

static void test_uds_nrc_coverage_sweep(void)
{
    SYN_UDS_Server server;
    TEST_ASSERT_TRUE(syn_uds_init(&server));
    server.security_state = SYN_UDS_SECURITY_UNLOCKED;
    TEST_ASSERT_TRUE(syn_uds_register_dtc(&server, 0x010203U, 0x01U, 0x60U));

    uint8_t req[32] = {0};
    uint8_t resp[64] = {0};
    uint16_t resp_len = 0;

    /* 1. Small response buffer (NRC 0x14) across ReadDTCInformation (0x19) subfunctions */
    req[0] = SYN_UDS_SID_READ_DTC_INFORMATION;
    req[1] = 0x01;
    req[2] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 3, resp, 5, &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_RESPONSE_TOO_LONG, resp[2]);

    req[1] = 0x02;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 3, resp, 5, &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_RESPONSE_TOO_LONG, resp[2]);

    req[1] = 0x0A;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, 5, &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_RESPONSE_TOO_LONG, resp[2]);

    req[1] = 0x0B;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, 5, &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_RESPONSE_TOO_LONG, resp[2]);

    req[1] = 0x08;
    req[2] = 0x60;
    req[3] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, 5, &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_RESPONSE_TOO_LONG, resp[2]);

    req[1] = 0x14;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, 5, &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_RESPONSE_TOO_LONG, resp[2]);

    /* 2. ReadDataByIdentifier (0x22) small response buffer -> NRC 0x14 */
    TEST_ASSERT_TRUE(syn_uds_register_did(&server, 0xF190, req, 4, false));
    req[0] = SYN_UDS_SID_READ_DATA_BY_IDENTIFIER;
    syn_poke_u16(0xF190, req, 1);
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 3, resp, 3, &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_RESPONSE_TOO_LONG, resp[2]);

    /* 3. ReadMemoryByAddress (0x23) small response buffer -> NRC 0x14 */
    req[0] = SYN_UDS_SID_READ_MEMORY_BY_ADDRESS;
    req[1] = 0x11;
    req[2] = 0x10;
    req[3] = 0x10;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, 5, &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_RESPONSE_TOO_LONG, resp[2]);

    /* 4. Incorrect message lengths across services -> NRC 0x13 */
    req[0] = SYN_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    req[0] = SYN_UDS_SID_CONTROL_DTC_SETTING;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    req[0] = SYN_UDS_SID_RESPONSE_ON_EVENT;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    req[0] = SYN_UDS_SID_READ_DATA_BY_PERIODIC_IDENTIFIER;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    req[0] = SYN_UDS_SID_DYNAMICALLY_DEFINE_DATA_IDENTIFIER;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    req[0] = SYN_UDS_SID_WRITE_DATA_BY_IDENTIFIER;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    req[0] = SYN_UDS_SID_INPUT_OUTPUT_CONTROL_BY_IDENTIFIER;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    req[0] = SYN_UDS_SID_ROUTINE_CONTROL;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    req[0] = SYN_UDS_SID_LINK_CONTROL;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    /* 5. Subfunction / Mode bounds errors -> NRC 0x12 / NRC 0x31 */
    req[0] = SYN_UDS_SID_RESPONSE_ON_EVENT;
    req[1] = 0x99;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, resp[2]);

    req[0] = SYN_UDS_SID_ROUTINE_CONTROL;
    req[1] = 0x99;
    syn_poke_u16(0x0201, req, 2);
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_SID_ROUTINE_CONTROL + 0x40, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x99, resp[1]);

    req[0] = SYN_UDS_SID_LINK_CONTROL;
    req[1] = 0x99;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, resp[2]);

    req[0] = SYN_UDS_SID_REQUEST_FILE_TRANSFER;
    req[1] = 0x99;
    req[2] = 'a';
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_REQUEST_OUT_OF_RANGE, resp[2]);

    /* 6. RequestFileTransfer (0x38) failing callback -> NRC 0x22 */
    syn_uds_register_file_transfer(&server, mock_failing_file_transfer_cb, NULL);
    req[1] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp[2]);
}

static bool mock_failing_auth_cb(uint8_t subfunction, const uint8_t *req_payload, uint16_t req_len,
                                 uint8_t *resp_payload, uint16_t max_resp_len, uint16_t *resp_len,
                                 void *ctx)
{
    (void)subfunction;
    (void)req_payload;
    (void)req_len;
    (void)resp_payload;
    (void)max_resp_len;
    (void)resp_len;
    (void)ctx;
    return false;
}

static bool mock_failing_secured_data_cb(const uint8_t *req_payload, uint16_t req_len,
                                         uint8_t *resp_payload, uint16_t max_resp_len,
                                         uint16_t *resp_len, void *ctx)
{
    (void)req_payload;
    (void)req_len;
    (void)resp_payload;
    (void)max_resp_len;
    (void)resp_len;
    (void)ctx;
    return false;
}

static void test_uds_nrc_coverage_sweep_part2(void)
{
    SYN_UDS_Server server;
    TEST_ASSERT_TRUE(syn_uds_init(&server));

    uint8_t req[32] = {0};
    uint8_t resp[64] = {0};
    uint16_t resp_len = 0;

    /* 1. AccessTimingParameter (0x83) subfunctions */
    req[0] = SYN_UDS_SID_ACCESS_TIMING_PARAMETER;
    req[1] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, 4, &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_RESPONSE_TOO_LONG, resp[2]);

    req[1] = 0x02;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    syn_uds_register_access_timing(&server, mock_failing_timing_cb, NULL);
    req[1] = 0x02;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp[2]);

    req[1] = 0x03;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, 4, &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_RESPONSE_TOO_LONG, resp[2]);

    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp[2]);

    /* 2. ReadDTCInformation fallback default subfunction without registered callback */
    req[0] = SYN_UDS_SID_READ_DTC_INFORMATION;
    req[1] = 0x7E;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_SID_READ_DTC_INFORMATION + 0x40, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x7E, resp[1]);

    /* 3. RequestDownload (0x34) when security is locked -> NRC 0x33 */
    server.security_state = SYN_UDS_SECURITY_LOCKED;
    req[0] = SYN_UDS_SID_REQUEST_DOWNLOAD;
    req[1] = 0x00;
    req[2] = 0x11;
    req[3] = 0x10;
    req[4] = 0x04;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 5, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_SECURITY_ACCESS_DENIED, resp[2]);

    server.security_state = SYN_UDS_SECURITY_UNLOCKED;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 5, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_SID_REQUEST_DOWNLOAD + 0x40, resp[0]);

    /* 4. ReadMemoryByAddress (0x23) failing callback -> NRC 0x22 */
    syn_uds_register_memory_handler(&server, mock_failing_memory_cb, NULL);
    req[0] = SYN_UDS_SID_READ_MEMORY_BY_ADDRESS;
    req[1] = 0x11;
    req[2] = 0x10;
    req[3] = 0x04;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp[2]);

    /* 5. WriteMemoryByAddress (0x3D) failing callback -> NRC 0x22 */
    req[0] = SYN_UDS_SID_WRITE_MEMORY_BY_ADDRESS;
    req[1] = 0x11;
    req[2] = 0x10;
    req[3] = 0x04;
    req[4] = 0xAA;
    req[5] = 0xBB;
    req[6] = 0xCC;
    req[7] = 0xDD;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 8, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp[2]);

    /* 6. Authentication (0x29) length error / invalid subfunction / failing callback */
    req[0] = SYN_UDS_SID_AUTHENTICATION;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    req[1] = 0x99;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, resp[2]);

    syn_uds_register_auth_handler(&server, mock_failing_auth_cb, NULL);
    req[1] = 0x01;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp[2]);

    /* 7. SecuredDataTransmission (0x84) length error / failing callback */
    req[0] = SYN_UDS_SID_SECURED_DATA_TRANSMISSION;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    syn_uds_register_secured_data(&server, mock_failing_secured_data_cb, NULL);
    req[1] = 0xAA;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp[2]);
}

static void test_uds_dtc_overflow_and_short_msg_nrcs(void)
{
    SYN_UDS_Server server;
    TEST_ASSERT_TRUE(syn_uds_init(&server));

    /* 1. Register max (32) DTCs and verify 33rd registration fails */
    for (uint32_t i = 0; i < 32; i++) {
        TEST_ASSERT_TRUE(syn_uds_register_dtc(&server, 0x100000 + i, 0x09, 0x01));
    }
    TEST_ASSERT_FALSE(syn_uds_register_dtc(&server, 0x100099, 0x09, 0x01));

    uint8_t req[16] = {0};
    uint8_t resp[64] = {0};
    uint16_t resp_len = 0;

    /* 2. ReadDTCInformation short request (<2 bytes) */
    req[0] = SYN_UDS_SID_READ_DTC_INFORMATION;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    /* 3. ReportNumberByStatusMask short request (<3 bytes) */
    req[1] = SYN_UDS_DTC_REPORT_NUMBER_BY_STATUS_MASK;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    /* 4. ReportDTCByStatusMask short request (<3 bytes) */
    req[1] = SYN_UDS_DTC_REPORT_BY_STATUS_MASK;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    /* 5. ReportNumberBySeverityMask short request (<4 bytes) */
    req[1] = SYN_UDS_DTC_REPORT_NUMBER_BY_SEVERITY_MASK;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    /* 6. ReportDTCBySeverityMask short request (<4 bytes) */
    req[1] = SYN_UDS_DTC_REPORT_BY_SEVERITY_MASK;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    /* 7. ControlDTCSetting unsupported subfunction */
    req[0] = SYN_UDS_SID_CONTROL_DTC_SETTING;
    req[1] = 0x05;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, resp[2]);

    /* 8. RequestDownload short request (<3 bytes) */
    req[0] = SYN_UDS_SID_REQUEST_DOWNLOAD;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    /* 9. TransferData short request (<2 bytes) */
    req[0] = SYN_UDS_SID_TRANSFER_DATA;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 1, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    /* 10. ReadMemoryByAddress short request (<3 bytes) and header mismatch */
    server.security_state = SYN_UDS_SECURITY_UNLOCKED;
    req[0] = SYN_UDS_SID_READ_MEMORY_BY_ADDRESS;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    req[1] = 0x22; /* 2 bytes addr, 2 bytes size -> total header = 2 + 2 + 2 = 6 */
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    /* 11. WriteMemoryByAddress short request (<3 bytes) and header mismatch */
    req[0] = SYN_UDS_SID_WRITE_MEMORY_BY_ADDRESS;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    req[1] = 0x22; /* header = 6, but payload length = 5 (< 6) */
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 5, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);
}

static void test_uds_read_dtc_additional_subfunctions(void)
{
    SYN_UDS_Server server;
    TEST_ASSERT_TRUE(syn_uds_init(&server));

    uint8_t req[16] = {0};
    uint8_t resp[64] = {0};
    uint16_t resp_len = 0;

    req[0] = SYN_UDS_SID_READ_DTC_INFORMATION;

    /* 1. Subfunctions requiring req_len >= 5 (0x04, 0x06, 0x10, 0x18, 0x19) */
    uint8_t subs5[] = {0x04, 0x06, 0x10, 0x18, 0x19};
    for (size_t i = 0; i < sizeof(subs5); i++) {
        req[1] = subs5[i];
        req[2] = 0x01;
        req[3] = 0x23;
        req[4] = 0x45;
        /* Short request (<5 bytes) -> NRC 0x13 */
        TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, sizeof(resp), &resp_len));
        TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
        TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

        /* Valid request (5 bytes) -> Positive response (0x59, sub) */
        TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 5, resp, sizeof(resp), &resp_len));
        TEST_ASSERT_EQUAL_HEX8(0x59, resp[0]);
        TEST_ASSERT_EQUAL_HEX8(subs5[i], resp[1]);
        TEST_ASSERT_EQUAL_UINT16(6, resp_len);
    }

    /* 2. Subfunction 0x03 (valid at req_len >= 2) */
    req[1] = 0x03;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x59, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(0x03, resp[1]);

    /* Subfunctions requiring req_len >= 3 (0x05, 0x16) */
    uint8_t subs3[] = {0x05, 0x16};
    for (size_t i = 0; i < sizeof(subs3); i++) {
        req[1] = subs3[i];
        req[2] = 0x01;
        /* Short request (<3 bytes) -> NRC 0x13 */
        TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 2, resp, sizeof(resp), &resp_len));
        TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
        TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

        /* Valid request (3 bytes) -> Positive response */
        TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 3, resp, sizeof(resp), &resp_len));
        TEST_ASSERT_EQUAL_HEX8(0x59, resp[0]);
        TEST_ASSERT_EQUAL_HEX8(subs3[i], resp[1]);
        TEST_ASSERT_EQUAL_UINT16(3, resp_len);
    }

    /* 3. Subfunctions requiring req_len >= 4 (0x17, 0x42) */
    uint8_t subs4[] = {0x17, 0x42};
    for (size_t i = 0; i < sizeof(subs4); i++) {
        req[1] = subs4[i];
        req[2] = 0xFF;
        req[3] = 0x01;
        /* Short request (<4 bytes) -> NRC 0x13 */
        TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 3, resp, sizeof(resp), &resp_len));
        TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
        TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

        /* Valid request (4 bytes) -> Positive response */
        TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, sizeof(resp), &resp_len));
        TEST_ASSERT_EQUAL_HEX8(0x59, resp[0]);
        TEST_ASSERT_EQUAL_HEX8(subs4[i], resp[1]);
        TEST_ASSERT_EQUAL_UINT16(3, resp_len);
    }
}

static void test_uds_clear_dtc_group_filtering(void)
{
    SYN_UDS_Server server;
    TEST_ASSERT_TRUE(syn_uds_init(&server));

    /* Register DTCs across Powertrain (P), Chassis (C), Body (B), Network (U) */
    TEST_ASSERT_TRUE(syn_uds_register_dtc(&server, 0x010000, 0x09, 0x01)); /* Powertrain */
    TEST_ASSERT_TRUE(syn_uds_register_dtc(&server, 0x450000, 0x09, 0x01)); /* Chassis */
    TEST_ASSERT_TRUE(syn_uds_register_dtc(&server, 0x850000, 0x09, 0x01)); /* Body */
    TEST_ASSERT_TRUE(syn_uds_register_dtc(&server, 0xC50000, 0x09, 0x01)); /* Network */
    TEST_ASSERT_EQUAL_UINT8(4, server.dtc_count);

    uint8_t req[16] = {0};
    uint8_t resp[64] = {0};
    uint16_t resp_len = 0;

    req[0] = SYN_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION;

    /* 1. Incorrect request length (!= 4) -> NRC 0x13 */
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 3, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    /* 2. Unsupported groupOfDTC -> NRC 0x31 */
    req[1] = 0x99;
    req[2] = 0x99;
    req[3] = 0x99;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_RESPONSE_NEGATIVE, resp[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_NRC_REQUEST_OUT_OF_RANGE, resp[2]);

    /* 3. Clear Powertrain group (0x000000) */
    req[1] = 0x00;
    req[2] = 0x00;
    req[3] = 0x00;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x54, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(3, server.dtc_count);

    /* 4. Clear Chassis group (0x400000) */
    req[1] = 0x40;
    req[2] = 0x00;
    req[3] = 0x00;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x54, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(2, server.dtc_count);

    /* 5. Clear Body group (0x800000) */
    req[1] = 0x80;
    req[2] = 0x00;
    req[3] = 0x00;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x54, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(1, server.dtc_count);

    /* 6. Clear Network group (0xC00000) */
    req[1] = 0xC0;
    req[2] = 0x00;
    req[3] = 0x00;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x54, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(0, server.dtc_count);

    /* 7. Clear All DTCs (0xFFFFFF) */
    req[1] = 0xFF;
    req[2] = 0xFF;
    req[3] = 0xFF;
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x54, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(0, server.dtc_count);

    /* 6. Clear All DTCs when no DTCs stored -> Positive response 0x54 */
    TEST_ASSERT_TRUE(syn_uds_process_request(&server, req, 4, resp, sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL_HEX8(0x54, resp[0]);
}

static void test_uds_dtc_iso14229_bit_operations(void)
{
    SYN_UDS_Server server;
    syn_uds_init(&server);

    /* NULL checks */
    TEST_ASSERT_FALSE(syn_uds_dtc_report_test_result(NULL, 0x010203U, true));
    TEST_ASSERT_FALSE(syn_uds_dtc_start_operation_cycle(NULL));
    TEST_ASSERT_FALSE(syn_uds_dtc_get_status(NULL, 0x010203U, NULL));
    uint8_t status = 0;
    TEST_ASSERT_FALSE(syn_uds_dtc_get_status(&server, 0x010203U, NULL));

    /* Register DTC with initial status */
    uint8_t init_status = SYN_UDS_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OP_CYCLE |
                          SYN_UDS_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR;
    TEST_ASSERT_TRUE(syn_uds_register_dtc(&server, 0x010203U, init_status,
                                          SYN_UDS_DTC_SEVERITY_CHECK_IMMEDIATELY));

    /* Check status retrieval */
    TEST_ASSERT_TRUE(syn_uds_dtc_get_status(&server, 0x010203U, &status));
    TEST_ASSERT_EQUAL_HEX8(init_status, status);

    /* Report test failure */
    TEST_ASSERT_TRUE(syn_uds_dtc_report_test_result(&server, 0x010203U, true));
    TEST_ASSERT_TRUE(syn_uds_dtc_get_status(&server, 0x010203U, &status));
    TEST_ASSERT_TRUE((status & SYN_UDS_DTC_STATUS_TEST_FAILED) != 0);
    TEST_ASSERT_TRUE((status & SYN_UDS_DTC_STATUS_TEST_FAILED_THIS_OP_CYCLE) != 0);
    TEST_ASSERT_TRUE((status & SYN_UDS_DTC_STATUS_PENDING_DTC) != 0);
    TEST_ASSERT_TRUE((status & SYN_UDS_DTC_STATUS_CONFIRMED_DTC) != 0);
    TEST_ASSERT_TRUE((status & SYN_UDS_DTC_STATUS_TEST_FAILED_SINCE_LAST_CLEAR) != 0);
    TEST_ASSERT_EQUAL_HEX8(0, status & SYN_UDS_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OP_CYCLE);

    /* Report test pass */
    TEST_ASSERT_TRUE(syn_uds_dtc_report_test_result(&server, 0x010203U, false));
    TEST_ASSERT_TRUE(syn_uds_dtc_get_status(&server, 0x010203U, &status));
    TEST_ASSERT_EQUAL_HEX8(0, status & SYN_UDS_DTC_STATUS_TEST_FAILED);

    /* Start new operation cycle */
    TEST_ASSERT_TRUE(syn_uds_dtc_start_operation_cycle(&server));
    TEST_ASSERT_TRUE(syn_uds_dtc_get_status(&server, 0x010203U, &status));
    TEST_ASSERT_EQUAL_HEX8(0, status & SYN_UDS_DTC_STATUS_TEST_FAILED_THIS_OP_CYCLE);
    TEST_ASSERT_TRUE((status & SYN_UDS_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OP_CYCLE) != 0);

    /* Non-existent DTC search */
    TEST_ASSERT_FALSE(syn_uds_dtc_report_test_result(&server, 0x999999U, true));
    TEST_ASSERT_FALSE(syn_uds_dtc_get_status(&server, 0x999999U, &status));
}

static bool mock_dtc_cb_fail_fn(uint8_t sub, const uint8_t *req, uint16_t req_len, uint8_t *resp,
                                uint16_t max_resp_len, uint16_t *out_len, void *user_ctx)
{
    (void)sub;
    (void)req;
    (void)req_len;
    (void)resp;
    (void)max_resp_len;
    (void)out_len;
    (void)user_ctx;
    return false;
}

static bool mock_dtc_cb_ok_fn(uint8_t sub, const uint8_t *req, uint16_t req_len, uint8_t *resp,
                              uint16_t max_resp_len, uint16_t *out_len, void *user_ctx)
{
    (void)sub;
    (void)req;
    (void)req_len;
    (void)resp;
    (void)max_resp_len;
    (void)out_len;
    (void)user_ctx;
    *out_len = 2;
    resp[0] = 0x11;
    resp[1] = 22;
    return true;
}

static bool mock_mem_cb_fail_fn(bool write, uint32_t addr, uint32_t len, uint8_t *buf,
                                void *user_ctx)
{
    (void)write;
    (void)addr;
    (void)len;
    (void)buf;
    (void)user_ctx;
    return false;
}

static void test_uds_remaining_uncovered_paths(void)
{
    SYN_UDS_Server server;
    syn_uds_init(&server);
    uint8_t resp[256];
    uint16_t resp_len = 0;

    /* 1. Clear DTC group filtering (lines 604, 636-637, 659, 661) */
    syn_uds_register_dtc(&server, 0x812345U, SYN_UDS_DTC_STATUS_CONFIRMED_DTC,
                         SYN_UDS_DTC_SEVERITY_CHECK_IMMEDIATELY); /* Body 0x800000..0x8EFFFF */
    syn_uds_register_dtc(&server, 0x412345U, SYN_UDS_DTC_STATUS_CONFIRMED_DTC,
                         SYN_UDS_DTC_SEVERITY_CHECK_IMMEDIATELY); /* Chassis 0x400000..0x4EFFFF */
    syn_uds_register_dtc(&server, 0xC12345U, SYN_UDS_DTC_STATUS_CONFIRMED_DTC,
                         SYN_UDS_DTC_SEVERITY_CHECK_IMMEDIATELY); /* Network 0xC00000..0xFEFFFF */

    uint8_t req_clear_body[4] = {SYN_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION, 0x81, 0x23, 0x45};
    syn_uds_process_request(&server, req_clear_body, 4, resp, sizeof(resp), &resp_len);

    uint8_t req_clear_chassis[4] = {SYN_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION, 0x41, 0x23, 0x45};
    syn_uds_process_request(&server, req_clear_chassis, 4, resp, sizeof(resp), &resp_len);

    uint8_t req_clear_net[4] = {SYN_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION, 0xC1, 0x23, 0x45};
    syn_uds_process_request(&server, req_clear_net, 4, resp, sizeof(resp), &resp_len);

    uint8_t req_clear_all[4] = {SYN_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION, 0xFF, 0xFF, 0xFF};
    syn_uds_process_request(&server, req_clear_all, 4, resp, sizeof(resp), &resp_len);

    /* 2. Read DTC subfunctions with dtc_cb (lines 894-905, 925, 948-956, 974-982, 996-1004) */
    server.dtc_cb = mock_dtc_cb_ok_fn;
    uint8_t subs[] = {0x06, 0x09, 0x0B, 0x0C, 0x0D, 0x17, 0x18};
    for (size_t i = 0; i < sizeof(subs); i++) {
        uint8_t req_dtc[6] = {SYN_UDS_SID_READ_DTC_INFORMATION, subs[i], 0x01, 0x02, 0x03, 0xFF};
        syn_uds_process_request(&server, req_dtc, 6, resp, sizeof(resp), &resp_len);
        TEST_ASSERT_EQUAL_HEX8(SYN_UDS_SID_READ_DTC_INFORMATION + 0x40, resp[0]);
    }

    server.dtc_cb = mock_dtc_cb_fail_fn;
    uint8_t subs_nrc[] = {0x06, 0x09, 0x17, 0x18};
    for (size_t i = 0; i < sizeof(subs_nrc); i++) {
        uint8_t req_dtc[6] = {
            SYN_UDS_SID_READ_DTC_INFORMATION, subs_nrc[i], 0x01, 0x02, 0x03, 0xFF};
        syn_uds_process_request(&server, req_dtc, 6, resp, sizeof(resp), &resp_len);
        TEST_ASSERT_EQUAL_HEX8(0x7F, resp[0]);
    }
    uint8_t subs_pos[] = {0x0B, 0x0C, 0x0D};
    for (size_t i = 0; i < sizeof(subs_pos); i++) {
        uint8_t req_dtc[6] = {
            SYN_UDS_SID_READ_DTC_INFORMATION, subs_pos[i], 0x01, 0x02, 0x03, 0xFF};
        syn_uds_process_request(&server, req_dtc, 6, resp, sizeof(resp), &resp_len);
        TEST_ASSERT_EQUAL_HEX8(SYN_UDS_SID_READ_DTC_INFORMATION + 0x40, resp[0]);
    }
    server.dtc_cb = NULL;

    /* 3. Subfunction 0x14/0x15 max_resp_len < 4 (line 1046) */
    uint8_t req_sf14[2] = {SYN_UDS_SID_READ_DTC_INFORMATION, 0x14};
    syn_uds_process_request(&server, req_sf14, 2, resp, 3, &resp_len);
    TEST_ASSERT_EQUAL_HEX8(SYN_UDS_SID_READ_DTC_INFORMATION + 0x40, resp[0]);

    /* 4. Transfer data overflow & read mem_cb fail (lines 1135, 1161, 1165-1168) */
    server.transfer_state = SYN_UDS_TRANSFER_DOWNLOAD;
    server.transfer_size = 5;
    server.transfer_bytes_processed = 4;
    uint8_t req_td_overflow[6] = {SYN_UDS_SID_TRANSFER_DATA, 0x01, 0xAA, 0xBB, 0xCC, 0xDD};
    syn_uds_process_request(&server, req_td_overflow, 6, resp, sizeof(resp), &resp_len);
    TEST_ASSERT_EQUAL_HEX8(0x7F, resp[0]);

    server.transfer_state = SYN_UDS_TRANSFER_UPLOAD;
    server.memory_cb = mock_mem_cb_fail_fn;
    uint8_t req_td_read_fail[3] = {SYN_UDS_SID_TRANSFER_DATA, 0x01, 0x02};
    syn_uds_process_request(&server, req_td_read_fail, 3, resp, sizeof(resp), &resp_len);
    TEST_ASSERT_EQUAL_HEX8(0x7F, resp[0]);

    /* 5. Read Scaling Data By Identifier bounds (lines 1237, 1242) */
    uint8_t req_scaling_short[2] = {SYN_UDS_SID_READ_SCALING_DATA_BY_IDENTIFIER, 0x01};
    syn_uds_process_request(&server, req_scaling_short, 2, resp, sizeof(resp), &resp_len);
    TEST_ASSERT_EQUAL_HEX8(0x7F, resp[0]);

    uint8_t req_scaling_ok[3] = {SYN_UDS_SID_READ_SCALING_DATA_BY_IDENTIFIER, 0x01, 0x02};
    syn_uds_process_request(&server, req_scaling_ok, 3, resp, 3, &resp_len);
    TEST_ASSERT_EQUAL_HEX8(0x7F, resp[0]);

    /* 6. Request Download incorrect length (line 1385) */
    uint8_t req_dl_short[4] = {SYN_UDS_SID_REQUEST_DOWNLOAD, 0x00, 0x11, 0x11};
    syn_uds_process_request(&server, req_dl_short, 4, resp, sizeof(resp), &resp_len);
    TEST_ASSERT_EQUAL_HEX8(0x7F, resp[0]);
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
    RUN_TEST(test_uds_complete_27_sids);
    RUN_TEST(test_uds_did_read_write);
    RUN_TEST(test_uds_ecu_reset_routine_tester_present);
    RUN_TEST(test_uds_read_dtc_information_subfunctions);
    RUN_TEST(test_uds_stateful_data_transfer_sequence);
    RUN_TEST(test_uds_spec_nrc_and_edge_cases);
    RUN_TEST(test_uds_upload_sequence_and_nrc_handling);
    RUN_TEST(test_uds_nrc_coverage_sweep);
    RUN_TEST(test_uds_nrc_coverage_sweep_part2);
    RUN_TEST(test_uds_bounds_and_null_checks);
    RUN_TEST(test_uds_dtc_overflow_and_short_msg_nrcs);
    RUN_TEST(test_uds_read_dtc_additional_subfunctions);
    RUN_TEST(test_uds_clear_dtc_group_filtering);
    RUN_TEST(test_uds_dtc_iso14229_bit_operations);
    RUN_TEST(test_uds_remaining_uncovered_paths);
}
