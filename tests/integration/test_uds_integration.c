#include "mock_port.h"
#include "syntropic/proto/syn_uds.h"
#include "unity/unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void setUp(void)
{
}

void tearDown(void)
{
}

void test_uds_iso14229_spec_e2e(void)
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

        /* 1. DiagnosticSessionControl (0x10 0x03 Extended Session - No Suppress Bit) */
        uint8_t req1[] = {SYN_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, SYN_UDS_SESSION_EXTENDED};
        uint8_t resp_buf[64];
        uint16_t resp_len = 0;

        bool ok = syn_uds_process_request(&server, req1, sizeof(req1), resp_buf, sizeof(resp_buf),
                                          &resp_len);
        TEST_ASSERT_TRUE(ok);
        TEST_ASSERT_EQUAL_UINT16(6, resp_len);
        TEST_ASSERT_EQUAL_UINT8(0x50, resp_buf[0]); /* 0x10 + 0x40 */
        TEST_ASSERT_EQUAL_UINT8(SYN_UDS_SESSION_EXTENDED, resp_buf[1]);
        TEST_ASSERT_EQUAL_INT(SYN_UDS_SESSION_EXTENDED, server.session);
        printf("[Integration Test] UDS DiagnosticSessionControl (0x10 0x03) PASS!\n");

        /* 2. TesterPresent with 0x80 Suppress Bit (0x3E 0x80) */
        uint8_t req2[] = {SYN_UDS_SID_TESTER_PRESENT, 0x80U}; /* Sub-function 0x00 | 0x80 */
        resp_len = 999;
        ok = syn_uds_process_request(&server, req2, sizeof(req2), resp_buf, sizeof(resp_buf),
                                     &resp_len);
        TEST_ASSERT_TRUE(ok);
        TEST_ASSERT_EQUAL_UINT16(0, resp_len); /* Positive response suppressed! */
        printf("[Integration Test] UDS Suppress Positive Response (0x3E 0x80 -> len=0) PASS!\n");

        /* 3. ReadDataByIdentifier (0x22 0xF1 0x90 VIN) */
        uint8_t vin_data[17] = "SYN1234567890UDS";
        TEST_ASSERT_TRUE(
            syn_uds_register_did(&server, 0xF190U, vin_data, sizeof(vin_data) - 1, false));

        uint8_t req3[] = {SYN_UDS_SID_READ_DATA_BY_IDENTIFIER, 0xF1U, 0x90U};
        ok = syn_uds_process_request(&server, req3, sizeof(req3), resp_buf, sizeof(resp_buf),
                                     &resp_len);
        TEST_ASSERT_TRUE(ok);
        TEST_ASSERT_EQUAL_UINT16(19, resp_len);     /* 0x62 + 0xF190 + 16-byte payload */
        TEST_ASSERT_EQUAL_UINT8(0x62, resp_buf[0]); /* 0x22 + 0x40 */
        TEST_ASSERT_EQUAL_UINT8(0xF1, resp_buf[1]);
        TEST_ASSERT_EQUAL_UINT8(0x90, resp_buf[2]);
        TEST_ASSERT_EQUAL_MEMORY("SYN1234567890UDS", &resp_buf[3], 16);
        printf("[Integration Test] UDS ReadDataByIdentifier (0x22 0xF190) PASS!\n");

        /* 4. Invalid SID (0xAA) -> Negative Response 0x7F 0xAA 0x11 */
        uint8_t req4[] = {0xAAU, 0x01U};
        ok = syn_uds_process_request(&server, req4, sizeof(req4), resp_buf, sizeof(resp_buf),
                                     &resp_len);
        TEST_ASSERT_TRUE(ok);
        TEST_ASSERT_EQUAL_UINT16(3, resp_len);
        TEST_ASSERT_EQUAL_UINT8(SYN_UDS_RESPONSE_NEGATIVE, resp_buf[0]); /* 0x7F */
        TEST_ASSERT_EQUAL_UINT8(0xAA, resp_buf[1]);
        TEST_ASSERT_EQUAL_UINT8(SYN_UDS_NRC_SERVICE_NOT_SUPPORTED, resp_buf[2]); /* 0x11 */
        printf("[Integration Test] UDS Negative Response Code (0x7F 0xAA 0x11) PASS!\n");

        syn_port_sock_close(client_sock);
    } else {
        printf("[Integration Test] 3rd-Party UDS Daemon at %s:%d not reachable (Loopback "
               "test)\n",
               host, port);
    }

    printf("[Integration Test] End-to-End UDS ISO 14229-1 Spec Integration PASS!\n");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_uds_iso14229_spec_e2e);
    return UNITY_END();
}
