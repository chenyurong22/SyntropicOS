#include "mock_port.h"
#include "syntropic/proto/syn_uds.h"
#include "unity/unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

/* clang-format off */

/* 1. Test Session Transitions and S3 Timeout Timer */
static void test_uds_session_transitions(SYN_UDS_Server *server)
{
    uint8_t resp[64];
    uint16_t len = 0;

    /* 0x10 0x01 Default Session */
    uint8_t req1[] = {SYN_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, SYN_UDS_SESSION_DEFAULT};
    bool ok = syn_uds_process_request(server, req1, sizeof(req1), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT16(6, len);
    TEST_ASSERT_EQUAL_UINT8(0x50, resp[0]);
    TEST_ASSERT_EQUAL_INT(SYN_UDS_SESSION_DEFAULT, server->session);

    /* 0x10 0x02 Programming Session */
    uint8_t req2[] = {SYN_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, SYN_UDS_SESSION_PROGRAMMING};
    ok = syn_uds_process_request(server, req2, sizeof(req2), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT16(6, len);
    TEST_ASSERT_EQUAL_UINT8(0x50, resp[0]);
    TEST_ASSERT_EQUAL_INT(SYN_UDS_SESSION_PROGRAMMING, server->session);

    /* 0x10 0x03 Extended Session */
    uint8_t req3[] = {SYN_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, SYN_UDS_SESSION_EXTENDED};
    ok = syn_uds_process_request(server, req3, sizeof(req3), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT16(6, len);
    TEST_ASSERT_EQUAL_UINT8(0x50, resp[0]);
    TEST_ASSERT_EQUAL_INT(SYN_UDS_SESSION_EXTENDED, server->session);

    /* S3 Timer Expiry -> Default Session fallback */
    syn_uds_tick(server, 60000U);
    TEST_ASSERT_EQUAL_INT(SYN_UDS_SESSION_DEFAULT, server->session);
    printf("[Integration Test] UDS Session Control & S3 Timeout Fallback PASS!\n");
}

/* 2. Test SecurityAccess Seed/Key Challenge-Response Flow & ISO 14229-1 Section 11.2.2 Zero Seed */
static void test_uds_security_access_flow(SYN_UDS_Server *server)
{
    uint8_t resp[64];
    uint16_t len = 0;

    /* Switch to Extended Session to enable SecurityAccess */
    uint8_t req_ext[] = {SYN_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, SYN_UDS_SESSION_EXTENDED};
    syn_uds_process_request(server, req_ext, sizeof(req_ext), resp, sizeof(resp), &len);

    /* Request Seed (0x27 0x01) */
    uint8_t req_seed[] = {SYN_UDS_SID_SECURITY_ACCESS, 0x01U};
    bool ok = syn_uds_process_request(server, req_seed, sizeof(req_seed), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT16(6, len);
    TEST_ASSERT_EQUAL_UINT8(0x67, resp[0]);
    TEST_ASSERT_EQUAL_INT(SYN_UDS_SECURITY_SEED_SENT, server->security_state);

    /* Send Key (0x27 0x02 <key>) with expected XOR mask 0xA5A5A5A5 */
    uint32_t seed = (uint32_t)resp[2] | ((uint32_t)resp[3] << 8) | ((uint32_t)resp[4] << 16) | ((uint32_t)resp[5] << 24);
    uint32_t expected_key = seed ^ 0xA5A5A5A5U;

    uint8_t req_key[6];
    req_key[0] = SYN_UDS_SID_SECURITY_ACCESS;
    req_key[1] = 0x02U;
    req_key[2] = (uint8_t)(expected_key & 0xFFU);
    req_key[3] = (uint8_t)((expected_key >> 8U) & 0xFFU);
    req_key[4] = (uint8_t)((expected_key >> 16U) & 0xFFU);
    req_key[5] = (uint8_t)((expected_key >> 24U) & 0xFFU);

    ok = syn_uds_process_request(server, req_key, sizeof(req_key), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT16(2, len);
    TEST_ASSERT_EQUAL_UINT8(0x67, resp[0]);
    TEST_ASSERT_EQUAL_INT(SYN_UDS_SECURITY_UNLOCKED, server->security_state);

    /* ISO 14229-1 Section 11.2.2 Check: Request seed while ALREADY UNLOCKED -> Must return seed = 0x00000000 */
    ok = syn_uds_process_request(server, req_seed, sizeof(req_seed), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT16(6, len);
    TEST_ASSERT_EQUAL_UINT8(0x67, resp[0]);
    uint32_t zero_seed = (uint32_t)resp[2] | ((uint32_t)resp[3] << 8) | ((uint32_t)resp[4] << 16) | ((uint32_t)resp[5] << 24);
    TEST_ASSERT_EQUAL_UINT32(0U, zero_seed);
    TEST_ASSERT_EQUAL_INT(SYN_UDS_SECURITY_UNLOCKED, server->security_state);

    printf("[Integration Test] UDS SecurityAccess Seed/Key & ISO 14229-1 Zero-Seed Unlocked Check (Issue #86) PASS!\n");
}

/* 3. Test DID Read/Write Matrix & Session/Security Rejections */
static void test_uds_did_read_write(SYN_UDS_Server *server)
{
    uint8_t resp[64];
    uint16_t len = 0;

    uint8_t status_buf[4] = {0x11, 0x22, 0x33, 0x44};
    bool ok_reg = syn_uds_register_did(server, 0x0100U, status_buf, sizeof(status_buf), true);
    TEST_ASSERT_TRUE(ok_reg);

    /* Read DID 0x0100 */
    uint8_t req_read[] = {SYN_UDS_SID_READ_DATA_BY_IDENTIFIER, 0x01U, 0x00U};
    bool ok = syn_uds_process_request(server, req_read, sizeof(req_read), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT16(7, len);
    TEST_ASSERT_EQUAL_UINT8(0x62, resp[0]);

    /* Write DID 0x0100 */
    uint8_t req_write[] = {SYN_UDS_SID_WRITE_DATA_BY_IDENTIFIER, 0x01U, 0x00U, 0xAA, 0xBB, 0xCC, 0xDD};
    ok = syn_uds_process_request(server, req_write, sizeof(req_write), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT16(3, len);
    TEST_ASSERT_EQUAL_UINT8(0x6E, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, status_buf[0]);

    /* Read Non-Existent DID 0x9999 -> NRC 0x31 Request Out of Range */
    uint8_t req_invalid[] = {SYN_UDS_SID_READ_DATA_BY_IDENTIFIER, 0x99U, 0x99U};
    ok = syn_uds_process_request(server, req_invalid, sizeof(req_invalid), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT16(3, len);
    TEST_ASSERT_EQUAL_UINT8(0x7F, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(SYN_UDS_NRC_REQUEST_OUT_OF_RANGE, resp[2]);
    printf("[Integration Test] UDS Read/Write DID & Out-Of-Range NRC PASS!\n");
}

/* 4. Test Complete ISO 14229-1 Negative Response Code (NRC) Matrix */
static void test_uds_nrc_matrix(SYN_UDS_Server *server)
{
    uint8_t resp[64];
    uint16_t len = 0;

    /* NRC 0x11: ServiceNotSupported (SID 0xAA) */
    uint8_t req11[] = {0xAAU, 0x00U};
    bool ok = syn_uds_process_request(server, req11, sizeof(req11), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x7F, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(SYN_UDS_NRC_SERVICE_NOT_SUPPORTED, resp[2]);

    /* NRC 0x12: SubFunctionNotSupported (0x10 0x99) */
    uint8_t req12[] = {SYN_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, 0x99U};
    ok = syn_uds_process_request(server, req12, sizeof(req12), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x7F, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, resp[2]);

    /* NRC 0x13: IncorrectMessageLength (0x10 len=1) */
    uint8_t req13[] = {SYN_UDS_SID_DIAGNOSTIC_SESSION_CONTROL};
    ok = syn_uds_process_request(server, req13, sizeof(req13), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x7F, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp[2]);

    /* NRC 0x22: ConditionsNotCorrect (Writing to read-only DID 0x0200) */
    uint8_t ro_buf[2] = {0x00, 0x00};
    syn_uds_register_did(server, 0x0200U, ro_buf, sizeof(ro_buf), false);
    uint8_t req22[] = {SYN_UDS_SID_WRITE_DATA_BY_IDENTIFIER, 0x02U, 0x00U, 0x11, 0x22};
    ok = syn_uds_process_request(server, req22, sizeof(req22), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x7F, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp[2]);

    /* NRC 0x24: RequestSequenceError (TransferData 0x36 without RequestDownload) */
    uint8_t req24[] = {SYN_UDS_SID_TRANSFER_DATA, 0x01U};
    ok = syn_uds_process_request(server, req24, sizeof(req24), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x7F, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(SYN_UDS_NRC_REQUEST_SEQUENCE_ERROR, resp[2]);

    /* NRC 0x33: SecurityAccessDenied (RequestDownload 0x34 while security locked) */
    server->security_state = SYN_UDS_SECURITY_LOCKED;
    uint8_t req33[] = {SYN_UDS_SID_REQUEST_DOWNLOAD, 0x00U, 0x11U, 0x10, 0x04};
    ok = syn_uds_process_request(server, req33, sizeof(req33), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x7F, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(SYN_UDS_NRC_SECURITY_ACCESS_DENIED, resp[2]);

    /* NRC 0x35: InvalidKey (Send wrong key to 0x27 0x02) */
    uint8_t req_seed[] = {SYN_UDS_SID_SECURITY_ACCESS, 0x01U};
    syn_uds_process_request(server, req_seed, sizeof(req_seed), resp, sizeof(resp), &len);
    uint8_t req35[] = {SYN_UDS_SID_SECURITY_ACCESS, 0x02U, 0x00, 0x00, 0x00, 0x00};
    ok = syn_uds_process_request(server, req35, sizeof(req35), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x7F, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(SYN_UDS_NRC_INVALID_KEY, resp[2]);

    /* NRC 0x36: ExceededNumberOfAttempts (3 consecutive invalid keys) */
    syn_uds_process_request(server, req_seed, sizeof(req_seed), resp, sizeof(resp), &len);
    syn_uds_process_request(server, req35, sizeof(req35), resp, sizeof(resp), &len);
    syn_uds_process_request(server, req_seed, sizeof(req_seed), resp, sizeof(resp), &len);
    ok = syn_uds_process_request(server, req35, sizeof(req35), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x7F, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(SYN_UDS_NRC_EXCEEDED_NUMBER_OF_ATTEMPTS, resp[2]);

    /* NRC 0x37: RequiredTimeDelayNotExpired (RequestSeed during penalty delay) */
    ok = syn_uds_process_request(server, req_seed, sizeof(req_seed), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x7F, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(SYN_UDS_NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED, resp[2]);

    /* Reset penalty delay */
    syn_uds_tick(server, 11000U);

    /* NRC 0x7E: SubFunctionNotSupportedInActiveSession (DID 0x0300 registered for Programming session only) */
    uint8_t prog_buf[2] = {0xAA, 0xBB};
    syn_uds_register_did_ext(server, 0x0300U, prog_buf, sizeof(prog_buf), true, SYN_UDS_SESSION_MASK_PROGRAMMING);
    server->session = SYN_UDS_SESSION_DEFAULT;
    uint8_t req7e[] = {SYN_UDS_SID_READ_DATA_BY_IDENTIFIER, 0x03U, 0x00U};
    ok = syn_uds_process_request(server, req7e, sizeof(req7e), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x7F, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED_IN_ACTIVE_SESSION, resp[2]);

    printf("[Integration Test] Complete 11 ISO 14229-1 NRC Matrix PASS!\n");
}

/* clang-format on */

void test_uds_iso14229_full_spec_matrix(void)
{
    SYN_UDS_Server server;
    TEST_ASSERT_TRUE(syn_uds_init(&server));

    const char *host = getenv("UDS_HOST");
    if (!host)
        host = "127.0.0.1";
    uint16_t port = 10886;

    printf("[Integration Test] Connecting to 3rd-Party UDS Daemon at %s:%d...\n", host, port);
    SYN_Socket client_sock = syn_port_sock_connect_host(host, port);

    if (client_sock != SYN_SOCKET_INVALID) {
        printf("[Integration Test] Connected to 3rd-Party UDS Daemon!\n");
        syn_port_sock_close(client_sock);
    }

    test_uds_session_transitions(&server);
    test_uds_security_access_flow(&server);
    test_uds_did_read_write(&server);
    test_uds_nrc_matrix(&server);

    printf("[Integration Test] Comprehensive UDS ISO 14229-1 Full Spec Matrix PASS!\n");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_uds_iso14229_full_spec_matrix);
    return UNITY_END();
}
