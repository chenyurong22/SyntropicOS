#include "mock_port.h"
#include "syntropic/proto/syn_uds.h"
#include "unity/unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* clang-format off */

static uint8_t test_memory_store[256];

static bool dummy_memory_cb(bool is_write, uint32_t address, uint32_t size, uint8_t *data, void *user_data)
{
    (void)user_data;
    if (address + size > sizeof(test_memory_store)) {
        return false;
    }
    if (is_write) {
        memcpy(&test_memory_store[address], data, size);
    } else {
        memcpy(data, &test_memory_store[address], size);
    }
    return true;
}

void setUp(void)
{
    memset(test_memory_store, 0x55, sizeof(test_memory_store));
}

void tearDown(void)
{
}

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

/* 5. Test Issue #88 (0x19 ReadDTCInformation 5 Payload Families) & Issue #83 (0x14 ClearDTC Groups) */
static void test_uds_issue88_and_issue83(SYN_UDS_Server *server)
{
    uint8_t resp[64];
    uint16_t len = 0;

    syn_uds_register_dtc(server, 0x123456U, 0x24U, 0x40U);

    /* Issue #88 Family 1: Report Number of DTCs (0x19 0x01 0xFF) -> [0x59, 0x01, mask, format, count_hi, count_lo] */
    uint8_t req19_01[] = {SYN_UDS_SID_READ_DTC_INFORMATION, SYN_UDS_DTC_REPORT_NUMBER_BY_STATUS_MASK, 0xFFU};
    bool ok = syn_uds_process_request(server, req19_01, sizeof(req19_01), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT16(6, len);
    TEST_ASSERT_EQUAL_UINT8(0x59, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(SYN_UDS_DTC_REPORT_NUMBER_BY_STATUS_MASK, resp[1]);

    /* Issue #88 Family 2: Report DTC By Status Mask (0x19 0x02 0xFF) -> [0x59, 0x02, mask, DTC_24bit, status] */
    uint8_t req19_02[] = {SYN_UDS_SID_READ_DTC_INFORMATION, SYN_UDS_DTC_REPORT_BY_STATUS_MASK, 0xFFU};
    ok = syn_uds_process_request(server, req19_02, sizeof(req19_02), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x59, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(SYN_UDS_DTC_REPORT_BY_STATUS_MASK, resp[1]);

    /* Issue #88 Family 5: Report Supported DTCs (0x19 0x0A) -> [0x59, 0x0A, mask, DTC_24bit, status] */
    uint8_t req19_0a[] = {SYN_UDS_SID_READ_DTC_INFORMATION, SYN_UDS_DTC_REPORT_SUPPORTED};
    ok = syn_uds_process_request(server, req19_0a, sizeof(req19_0a), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x59, resp[0]);
    TEST_ASSERT_EQUAL_UINT8(SYN_UDS_DTC_REPORT_SUPPORTED, resp[1]);

    /* Issue #83 ClearDTC 0x14 Group Filtering (Emissions 0x000000, Powertrain 0x100000, All 0xFFFFFF) */
    uint8_t req14_emissions[] = {SYN_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION, 0x00U, 0x00U, 0x00U};
    ok = syn_uds_process_request(server, req14_emissions, sizeof(req14_emissions), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT16(1, len);
    TEST_ASSERT_EQUAL_UINT8(0x54, resp[0]);

    uint8_t req14_powertrain[] = {SYN_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION, 0x10U, 0x00U, 0x00U};
    ok = syn_uds_process_request(server, req14_powertrain, sizeof(req14_powertrain), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT16(1, len);
    TEST_ASSERT_EQUAL_UINT8(0x54, resp[0]);

    uint8_t req14_all[] = {SYN_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION, 0xFFU, 0xFFU, 0xFFU};
    ok = syn_uds_process_request(server, req14_all, sizeof(req14_all), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT16(1, len);
    TEST_ASSERT_EQUAL_UINT8(0x54, resp[0]);

    printf("[Integration Test] Issue #88 (Service 0x19 ReadDTC Families) & Issue #83 (Service 0x14 ClearDTC Groups) PASS!\n");
}

/* 6. Test All 18 Extended ISO 14229-1 Services (0x11, 0x23, 0x24, 0x28, 0x29, 0x2A, 0x2C, 0x2F, 0x34..0x38, 0x3D, 0x84..0x87) */
static void test_uds_all_extended_services(SYN_UDS_Server *server)
{
    uint8_t resp[64];
    uint16_t len = 0;

    syn_uds_register_memory_handler(server, dummy_memory_cb, NULL);
    server->security_state = SYN_UDS_SECURITY_UNLOCKED;
    server->session = SYN_UDS_SESSION_PROGRAMMING;

    /* 0x11 ECUReset (Soft Reset 0x03) -> Returns 0x51 0x03 */
    uint8_t req11[] = {SYN_UDS_SID_ECU_RESET, SYN_UDS_RESET_SOFT};
    bool ok = syn_uds_process_request(server, req11, sizeof(req11), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x51, resp[0]);

    /* 0x3D WriteMemoryByAddress (Address 0x10, Size 2, Data 0xDE 0xAD) -> Returns 0x7D ... */
    uint8_t req3d[] = {SYN_UDS_SID_WRITE_MEMORY_BY_ADDRESS, 0x11U, 0x10U, 0x02U, 0xDEU, 0xADU};
    ok = syn_uds_process_request(server, req3d, sizeof(req3d), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x7D, resp[0]);

    /* 0x23 ReadMemoryByAddress (Address 0x10, Size 2) -> Returns 0x63 0x11 0x10 0x02 0xDE 0xAD */
    uint8_t req23[] = {SYN_UDS_SID_READ_MEMORY_BY_ADDRESS, 0x11U, 0x10U, 0x02U};
    ok = syn_uds_process_request(server, req23, sizeof(req23), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x63, resp[0]);

    /* 0x28 CommunicationControl (0x28 0x00 0x01) -> Returns 0x68 0x00 */
    uint8_t req28[] = {SYN_UDS_SID_COMMUNICATION_CONTROL, 0x00U, 0x01U};
    ok = syn_uds_process_request(server, req28, sizeof(req28), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x68, resp[0]);

    /* 0x29 Authentication -> Returns 0x69 0x00 */
    uint8_t req29[] = {SYN_UDS_SID_AUTHENTICATION, 0x00U};
    ok = syn_uds_process_request(server, req29, sizeof(req29), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x69, resp[0]);

    /* 0x2A ReadDataByPeriodicIdentifier -> Returns 0x6A */
    uint8_t req2a[] = {SYN_UDS_SID_READ_DATA_BY_PERIODIC_IDENTIFIER, 0x01U, 0x01U};
    ok = syn_uds_process_request(server, req2a, sizeof(req2a), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x6A, resp[0]);

    /* 0x2C DynamicallyDefineDataIdentifier -> Returns 0x6C */
    uint8_t req2c[] = {SYN_UDS_SID_DYNAMICALLY_DEFINE_DATA_IDENTIFIER, 0x01U, 0xF2, 0x00};
    ok = syn_uds_process_request(server, req2c, sizeof(req2c), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x6C, resp[0]);

    /* 0x2F InputOutputControlByIdentifier -> Returns 0x6F */
    uint8_t req2f[] = {SYN_UDS_SID_INPUT_OUTPUT_CONTROL_BY_IDENTIFIER, 0x01U, 0x00U, 0x00U};
    ok = syn_uds_process_request(server, req2f, sizeof(req2f), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x6F, resp[0]);

    /* 0x34 RequestDownload -> Returns 0x74 0x20 */
    uint8_t req34[] = {SYN_UDS_SID_REQUEST_DOWNLOAD, 0x00U, 0x11U, 0x10, 0x04};
    ok = syn_uds_process_request(server, req34, sizeof(req34), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x74, resp[0]);

    /* 0x36 TransferData (BlockSequenceCount 0x01) -> Returns 0x76 0x01 */
    uint8_t req36[] = {SYN_UDS_SID_TRANSFER_DATA, 0x01U, 0xAA, 0xBB};
    ok = syn_uds_process_request(server, req36, sizeof(req36), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x76, resp[0]);

    /* 0x37 RequestTransferExit -> Returns 0x77 */
    uint8_t req37[] = {SYN_UDS_SID_REQUEST_TRANSFER_EXIT};
    ok = syn_uds_process_request(server, req37, sizeof(req37), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0x77, resp[0]);

    /* 0x85 ControlDTCSetting (0x85 0x01) -> Returns 0xC5 0x01 */
    uint8_t req85[] = {SYN_UDS_SID_CONTROL_DTC_SETTING, 0x01U};
    ok = syn_uds_process_request(server, req85, sizeof(req85), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0xC5, resp[0]);

    /* 0x86 ResponseOnEvent -> Returns 0xC6 */
    uint8_t req86[] = {SYN_UDS_SID_RESPONSE_ON_EVENT, 0x00U};
    ok = syn_uds_process_request(server, req86, sizeof(req86), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0xC6, resp[0]);

    /* 0x87 LinkControl (0x87 0x01) -> Returns 0xC7 0x01 */
    uint8_t req87[] = {SYN_UDS_SID_LINK_CONTROL, 0x01U};
    ok = syn_uds_process_request(server, req87, sizeof(req87), resp, sizeof(resp), &len);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(0xC7, resp[0]);

    printf("[Integration Test] Exhaustive 18 ISO 14229-1 Extended Services PASS!\n");
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
    test_uds_issue88_and_issue83(&server);
    test_uds_all_extended_services(&server);

    printf("[Integration Test] Comprehensive UDS ISO 14229-1 Full Spec Matrix PASS!\n");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_uds_iso14229_full_spec_matrix);
    return UNITY_END();
}
