/**
 * @file test_ocpp.c
 * @brief Unit tests for Open Charge Point Protocol (OCPP-J 1.6 / 2.0.1) Client Engine.
 */

#include "syntropic/proto/syn_ocpp.h"
#include "unity/unity.h"

#include <string.h>

static SYN_OCPP_Client g_ocpp_client;
static bool g_reg_cb_called = false;
static bool g_auth_cb_called = false;
static bool g_start_tx_cb_called = false;
static bool g_remote_start_called = false;
static bool g_remote_stop_called = false;

static void on_ocpp_reg(SYN_OCPP_RegistrationStatus status, uint32_t interval, void *ctx)
{
    (void)status;
    (void)interval;
    (void)ctx;
    g_reg_cb_called = true;
}

static void on_ocpp_auth(const char *id_tag, SYN_OCPP_AuthorizationStatus status, void *ctx)
{
    (void)id_tag;
    (void)status;
    (void)ctx;
    g_auth_cb_called = true;
}

static void on_ocpp_start_tx(int32_t tx_id, SYN_OCPP_AuthorizationStatus status, void *ctx)
{
    (void)tx_id;
    (void)status;
    (void)ctx;
    g_start_tx_cb_called = true;
}

static bool on_ocpp_remote_start(uint32_t conn_id, const char *id_tag, void *ctx)
{
    (void)conn_id;
    (void)id_tag;
    (void)ctx;
    g_remote_start_called = true;
    return true;
}

static bool on_ocpp_remote_stop(int32_t tx_id, void *ctx)
{
    (void)tx_id;
    (void)ctx;
    g_remote_stop_called = true;
    return true;
}

void test_ocpp_init_and_null_checks(void)
{
    char buf[128];
    size_t len = 0;
    SYN_OCPP_ChargePointInfo info = {"Vendor", "Model", "SN123", "v1.0"};

    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_ocpp_init(NULL));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM,
                      syn_ocpp_format_boot_notification(NULL, &info, buf, sizeof(buf), &len));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_ocpp_format_heartbeat(NULL, buf, sizeof(buf), &len));
}

void test_ocpp_boot_notification_formatting(void)
{
    syn_ocpp_init(&g_ocpp_client);
    SYN_OCPP_ChargePointInfo info = {"SyntropicVendor", "EVSE-v2", "SN-9999", "2.0.1"};
    char buf[256];
    size_t len = 0;

    TEST_ASSERT_EQUAL(
        SYN_OK, syn_ocpp_format_boot_notification(&g_ocpp_client, &info, buf, sizeof(buf), &len));
    TEST_ASSERT_TRUE(len > 0);
    TEST_ASSERT_NOT_NULL(strstr(buf, "BootNotification"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "SyntropicVendor"));
}

void test_ocpp_heartbeat_and_status_formatting(void)
{
    syn_ocpp_init(&g_ocpp_client);
    char buf[256];
    size_t len = 0;

    TEST_ASSERT_EQUAL(SYN_OK, syn_ocpp_format_heartbeat(&g_ocpp_client, buf, sizeof(buf), &len));
    TEST_ASSERT_NOT_NULL(strstr(buf, "Heartbeat"));

    TEST_ASSERT_EQUAL(
        SYN_OK, syn_ocpp_format_status_notification(&g_ocpp_client, 1, SYN_OCPP_STATUS_CHARGING,
                                                    "NoError", buf, sizeof(buf), &len));
    TEST_ASSERT_NOT_NULL(strstr(buf, "StatusNotification"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "Charging"));
}

void test_ocpp_transaction_formatting(void)
{
    syn_ocpp_init(&g_ocpp_client);
    char buf[256];
    size_t len = 0;

    TEST_ASSERT_EQUAL(
        SYN_OK, syn_ocpp_format_authorize(&g_ocpp_client, "RFID-TAG-123", buf, sizeof(buf), &len));
    TEST_ASSERT_NOT_NULL(strstr(buf, "Authorize"));

    TEST_ASSERT_EQUAL(SYN_OK, syn_ocpp_format_start_transaction(&g_ocpp_client, 1, "RFID-TAG-123",
                                                                1000, buf, sizeof(buf), &len));
    TEST_ASSERT_NOT_NULL(strstr(buf, "StartTransaction"));

    TEST_ASSERT_EQUAL(SYN_OK,
                      syn_ocpp_format_stop_transaction(&g_ocpp_client, 42, 12500, "EVDisconnected",
                                                       buf, sizeof(buf), &len));
    TEST_ASSERT_NOT_NULL(strstr(buf, "StopTransaction"));
}

void test_ocpp_meter_values_formatting(void)
{
    syn_ocpp_init(&g_ocpp_client);
    SYN_OCPP_MeterValues mv = {12500, 230, 160, 3680, 75};
    char buf[256];
    size_t len = 0;

    TEST_ASSERT_EQUAL(SYN_OK,
                      syn_ocpp_format_meter_values(&g_ocpp_client, 1, &mv, buf, sizeof(buf), &len));
    TEST_ASSERT_NOT_NULL(strstr(buf, "MeterValues"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "12500"));
}

void test_ocpp_process_call_result(void)
{
    syn_ocpp_init(&g_ocpp_client);
    g_reg_cb_called = false;
    syn_ocpp_set_callbacks(&g_ocpp_client, on_ocpp_reg, on_ocpp_auth, on_ocpp_start_tx,
                           on_ocpp_remote_start, on_ocpp_remote_stop, NULL);

    const char *resp_json = "[3,\"1\",{\"status\":\"Accepted\",\"interval\":60}]";
    TEST_ASSERT_EQUAL(SYN_OK, syn_ocpp_process_message(&g_ocpp_client, resp_json, strlen(resp_json),
                                                       NULL, 0, NULL));
    TEST_ASSERT_EQUAL(SYN_OCPP_REGISTRATION_ACCEPTED, g_ocpp_client.registration_status);
    TEST_ASSERT_TRUE(g_reg_cb_called);
}

void test_ocpp_process_remote_start_stop(void)
{
    syn_ocpp_init(&g_ocpp_client);
    g_remote_start_called = false;
    g_remote_stop_called = false;
    syn_ocpp_set_callbacks(&g_ocpp_client, on_ocpp_reg, on_ocpp_auth, on_ocpp_start_tx,
                           on_ocpp_remote_start, on_ocpp_remote_stop, NULL);

    const char *cmd_start =
        "[2,\"10\",\"RemoteStartTransaction\",{\"connectorId\":1,\"idTag\":\"RFID-101\"}]";
    char resp[128];
    size_t resp_len = 0;

    TEST_ASSERT_EQUAL(SYN_OK, syn_ocpp_process_message(&g_ocpp_client, cmd_start, strlen(cmd_start),
                                                       resp, sizeof(resp), &resp_len));
    TEST_ASSERT_TRUE(g_remote_start_called);
    TEST_ASSERT_NOT_NULL(strstr(resp, "Accepted"));

    const char *cmd_stop = "[2,\"11\",\"RemoteStopTransaction\",{\"transactionId\":42}]";
    TEST_ASSERT_EQUAL(SYN_OK, syn_ocpp_process_message(&g_ocpp_client, cmd_stop, strlen(cmd_stop),
                                                       resp, sizeof(resp), &resp_len));
    TEST_ASSERT_TRUE(g_remote_stop_called);

    /* Test without callbacks registered */
    syn_ocpp_init(&g_ocpp_client);
    TEST_ASSERT_EQUAL(SYN_OK, syn_ocpp_process_message(&g_ocpp_client, cmd_start, strlen(cmd_start),
                                                       resp, sizeof(resp), &resp_len));
    TEST_ASSERT_NOT_NULL(strstr(resp, "Accepted"));
    TEST_ASSERT_EQUAL(SYN_OK, syn_ocpp_process_message(&g_ocpp_client, cmd_stop, strlen(cmd_stop),
                                                       resp, sizeof(resp), &resp_len));
    TEST_ASSERT_NOT_NULL(strstr(resp, "Accepted"));
}

void test_ocpp_tick(void)
{
    syn_ocpp_init(&g_ocpp_client);
    g_ocpp_client.heartbeat_timer_ms = 100;

    char hb_buf[128];
    size_t hb_len = 0;

    syn_ocpp_tick(&g_ocpp_client, 50, hb_buf, sizeof(hb_buf), &hb_len);
    TEST_ASSERT_EQUAL(0, hb_len);

    syn_ocpp_tick(&g_ocpp_client, 60, hb_buf, sizeof(hb_buf), &hb_len);
    TEST_ASSERT_TRUE(hb_len > 0);
    TEST_ASSERT_NOT_NULL(strstr(hb_buf, "Heartbeat"));
}

void test_ocpp_all_status_enums_and_small_buffers(void)
{
    syn_ocpp_init(&g_ocpp_client);
    char buf[256];
    size_t len = 0;

    SYN_OCPP_ChargePointStatus statuses[] = {
        SYN_OCPP_STATUS_AVAILABLE,     SYN_OCPP_STATUS_PREPARING,      SYN_OCPP_STATUS_CHARGING,
        SYN_OCPP_STATUS_SUSPENDED_EV,  SYN_OCPP_STATUS_SUSPENDED_EVSE, SYN_OCPP_STATUS_FINISHING,
        SYN_OCPP_STATUS_RESERVED,      SYN_OCPP_STATUS_UNAVAILABLE,    SYN_OCPP_STATUS_FAULTED,
        (SYN_OCPP_ChargePointStatus)99};

    for (size_t i = 0; i < sizeof(statuses) / sizeof(statuses[0]); i++) {
        TEST_ASSERT_EQUAL(SYN_OK,
                          syn_ocpp_format_status_notification(&g_ocpp_client, 1, statuses[i],
                                                              "NoError", buf, sizeof(buf), &len));
    }

    /* Small buffer errors */
    SYN_OCPP_ChargePointInfo info = {"Vendor", "Model", "SN", "v1"};
    SYN_OCPP_MeterValues mv = {100, 230, 16, 3600, 50};

    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM,
                      syn_ocpp_format_boot_notification(&g_ocpp_client, &info, buf, 10, &len));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_ocpp_format_heartbeat(&g_ocpp_client, buf, 10, &len));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM,
                      syn_ocpp_format_status_notification(
                          &g_ocpp_client, 1, SYN_OCPP_STATUS_CHARGING, "NoError", buf, 10, &len));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM,
                      syn_ocpp_format_authorize(&g_ocpp_client, "TAG", buf, 10, &len));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_ocpp_format_start_transaction(&g_ocpp_client, 1, "TAG",
                                                                           100, buf, 10, &len));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_ocpp_format_stop_transaction(&g_ocpp_client, 1, 100,
                                                                          "Reason", buf, 10, &len));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM,
                      syn_ocpp_format_meter_values(&g_ocpp_client, 1, &mv, buf, 10, &len));
}

void test_ocpp_process_message_extended(void)
{
    syn_ocpp_init(&g_ocpp_client);
    g_start_tx_cb_called = false;
    syn_ocpp_set_callbacks(&g_ocpp_client, on_ocpp_reg, on_ocpp_auth, on_ocpp_start_tx, NULL, NULL,
                           NULL);

    /* StartTransaction response containing idTagInfo and transactionId */
    const char *resp_tx =
        "[3,\"100\",{\"idTagInfo\":{\"status\":\"Accepted\"},\"transactionId\":1001}]";
    TEST_ASSERT_EQUAL(
        SYN_OK, syn_ocpp_process_message(&g_ocpp_client, resp_tx, strlen(resp_tx), NULL, 0, NULL));
    TEST_ASSERT_EQUAL(1001, g_ocpp_client.active_transaction_id);
    TEST_ASSERT_TRUE(g_start_tx_cb_called);

    /* Unknown Call Action fallback */
    const char *cmd_unknown = "[2,\"200\",\"DataTransfer\",{\"vendorId\":\"Vendor\"}]";
    char resp[128];
    size_t resp_len = 0;
    TEST_ASSERT_EQUAL(SYN_OK,
                      syn_ocpp_process_message(&g_ocpp_client, cmd_unknown, strlen(cmd_unknown),
                                               resp, sizeof(resp), &resp_len));
    TEST_ASSERT_NOT_NULL(strstr(resp, "[3,\"200\",{}]"));

    /* CallError message (Type 4) */
    const char *err_msg = "[4,\"300\",\"NotSupported\",\"Action not supported\",{}]";
    TEST_ASSERT_EQUAL(
        SYN_OK, syn_ocpp_process_message(&g_ocpp_client, err_msg, strlen(err_msg), NULL, 0, NULL));

    /* Invalid frames */
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_ocpp_process_message(&g_ocpp_client, "123", 3, resp,
                                                                  sizeof(resp), &resp_len));
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_ocpp_process_message(&g_ocpp_client, "INVALID_FRAME", 13, resp,
                                                          sizeof(resp), &resp_len));
}

void run_ocpp_tests(void)
{
    RUN_TEST(test_ocpp_init_and_null_checks);
    RUN_TEST(test_ocpp_boot_notification_formatting);
    RUN_TEST(test_ocpp_heartbeat_and_status_formatting);
    RUN_TEST(test_ocpp_transaction_formatting);
    RUN_TEST(test_ocpp_meter_values_formatting);
    RUN_TEST(test_ocpp_process_call_result);
    RUN_TEST(test_ocpp_process_remote_start_stop);
    RUN_TEST(test_ocpp_tick);
    RUN_TEST(test_ocpp_all_status_enums_and_small_buffers);
    RUN_TEST(test_ocpp_process_message_extended);
}
