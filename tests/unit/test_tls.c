/**
 * @file test_tls.c
 * @brief Unit tests for native zero-dependency TLS 1.3 engine and transport binding.
 */

#include "syntropic/net/syn_tls.h"
#include "unity/unity.h"

#include <string.h>

/* Mock Loopback Transport for testing TLS encapsulation */
typedef struct {
    uint8_t buf[4096];
    size_t len;
} LoopbackTransport;

static bool loopback_send(const uint8_t *data, size_t len, void *ctx)
{
    LoopbackTransport *lb = (LoopbackTransport *)ctx;
    if (lb->len + len > sizeof(lb->buf)) {
        return false;
    }
    memcpy(lb->buf + lb->len, data, len);
    lb->len += len;
    return true;
}

static bool loopback_recv(uint8_t *data, size_t max_len, size_t *out_len, void *ctx)
{
    LoopbackTransport *lb = (LoopbackTransport *)ctx;
    if (lb->len == 0) {
        return false;
    }
    size_t copy_len = lb->len;
    if (copy_len > max_len) {
        copy_len = max_len;
    }
    memcpy(data, lb->buf, copy_len);
    *out_len = copy_len;
    lb->len = 0;
    return true;
}

void test_tls_psk_mode_handshake_and_record_crypto(void)
{
    LoopbackTransport wire = {0};
    SYN_Transport tr = {.send = loopback_send, .recv = loopback_recv, .ctx = &wire};

    static const uint8_t psk[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                                    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                                    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};

    SYN_TLS_Config config = {.mode = SYN_TLS_AUTH_MODE_PSK,
                             .server_name = "device.local",
                             .psk_identity = (const uint8_t *)"device_01",
                             .psk_identity_len = 9,
                             .psk_secret = psk,
                             .psk_secret_len = sizeof(psk)};

    uint8_t rx_record_buf[2560];
    uint8_t tx_record_buf[2560];
    SYN_TLS_Context tls;
    TEST_ASSERT_TRUE(syn_tls_init(&tls, &config, &tr, rx_record_buf, sizeof(rx_record_buf),
                                  tx_record_buf, sizeof(tx_record_buf)));

    /* Handshake completion */
    TEST_ASSERT_TRUE(syn_tls_handshake(&tls));
    TEST_ASSERT_EQUAL(SYN_TLS_STATE_ESTABLISHED, tls.state);
    TEST_ASSERT_TRUE(syn_tls_is_established(&tls));
    TEST_ASSERT_EQUAL(SYN_TLS_STATE_ESTABLISHED, syn_tls_get_state(&tls));

    /* Encrypt & Send record */
    static const char msg[] = "Hello Zero-Dependency TLS 1.3!";
    TEST_ASSERT_TRUE(syn_tls_send(&tls, (const uint8_t *)msg, strlen(msg)));

    /* Receive & Decrypt record */
    uint8_t rx_buf[128];
    size_t rx_len = 0;
    TEST_ASSERT_TRUE(syn_tls_recv(&tls, rx_buf, sizeof(rx_buf), &rx_len));
    TEST_ASSERT_EQUAL(strlen(msg), rx_len);
    TEST_ASSERT_EQUAL_MEMORY(msg, rx_buf, rx_len);
}

void test_tls_transport_binding_interface(void)
{
    LoopbackTransport wire = {0};
    SYN_Transport raw_tr = {.send = loopback_send, .recv = loopback_recv, .ctx = &wire};

    SYN_TLS_Config config = {.mode = SYN_TLS_AUTH_MODE_RAW_PUBKEY};

    uint8_t rx_record_buf[2560];
    uint8_t tx_record_buf[2560];
    SYN_TLS_Context tls;
    TEST_ASSERT_TRUE(syn_tls_init(&tls, &config, &raw_tr, rx_record_buf, sizeof(rx_record_buf),
                                  tx_record_buf, sizeof(tx_record_buf)));

    SYN_Transport tls_tr;
    syn_tls_bind_transport(&tls, &tls_tr);

    static const char payload[] = "Encapsulated via SYN_Transport abstraction";
    TEST_ASSERT_TRUE(syn_transport_send(&tls_tr, (const uint8_t *)payload, strlen(payload)));

    uint8_t rx_buf[128];
    size_t rx_len = 0;
    TEST_ASSERT_TRUE(syn_transport_recv(&tls_tr, rx_buf, sizeof(rx_buf), &rx_len));
    TEST_ASSERT_EQUAL(strlen(payload), rx_len);
    TEST_ASSERT_EQUAL_MEMORY(payload, rx_buf, rx_len);
}

void test_tls_task_protothread(void)
{
    LoopbackTransport wire = {0};
    SYN_Transport tr = {.send = loopback_send, .recv = loopback_recv, .ctx = &wire};
    SYN_TLS_Config config = {.mode = SYN_TLS_AUTH_MODE_PSK};

    uint8_t rx_record_buf[2560];
    uint8_t tx_record_buf[2560];
    SYN_TLS_Context tls;
    TEST_ASSERT_TRUE(syn_tls_init(&tls, &config, &tr, rx_record_buf, sizeof(rx_record_buf),
                                  tx_record_buf, sizeof(tx_record_buf)));

    SYN_Task task = {.user_data = &tls};
    SYN_PT pt;
    PT_INIT(&pt);

    SYN_PT_Status status = syn_tls_task(&pt, &task);
    TEST_ASSERT_EQUAL(PT_YIELDED, status);
    TEST_ASSERT_EQUAL(SYN_TLS_STATE_ESTABLISHED, tls.state);
}

void test_tls_null_and_bounds_checks(void)
{
    uint8_t rx_small[64];
    uint8_t rx_ok[2560];
    uint8_t tx_ok[2560];
    SYN_TLS_Config config = {.mode = SYN_TLS_AUTH_MODE_PSK};
    SYN_TLS_Context tls;
    LoopbackTransport wire = {0};
    SYN_Transport tr = {.send = loopback_send, .recv = loopback_recv, .ctx = &wire};

    TEST_ASSERT_FALSE(syn_tls_init(NULL, NULL, NULL, NULL, 0, NULL, 0));
    TEST_ASSERT_FALSE(
        syn_tls_init(&tls, &config, &tr, rx_small, sizeof(rx_small), tx_ok, sizeof(tx_ok)));
    TEST_ASSERT_FALSE(syn_tls_handshake(NULL));
    TEST_ASSERT_FALSE(syn_tls_send(NULL, NULL, 0));
    TEST_ASSERT_FALSE(syn_tls_recv(NULL, NULL, 0, NULL));

    syn_tls_bind_transport(NULL, NULL);

    uint8_t long_psk[64];
    memset(long_psk, 0xAA, sizeof(long_psk));
    SYN_TLS_Config psk_cfg = {
        .mode = SYN_TLS_AUTH_MODE_PSK, .psk_secret = long_psk, .psk_secret_len = sizeof(long_psk)};
    SYN_TLS_Context psk_tls;
    TEST_ASSERT_TRUE(
        syn_tls_init(&psk_tls, &psk_cfg, &tr, rx_ok, sizeof(rx_ok), tx_ok, sizeof(tx_ok)));
    TEST_ASSERT_TRUE(syn_tls_handshake(&psk_tls));
    TEST_ASSERT_TRUE(syn_tls_handshake(&psk_tls)); /* Repeat call when established */

    /* Zero length send */
    TEST_ASSERT_TRUE(syn_tls_send(&psk_tls, NULL, 0));
    TEST_ASSERT_FALSE(syn_tls_send(&psk_tls, NULL, 10000));

    /* Recv errors */
    uint8_t rx_buf[64];
    size_t rx_len = 0;
    TEST_ASSERT_FALSE(syn_tls_recv(NULL, rx_buf, sizeof(rx_buf), &rx_len));
    TEST_ASSERT_FALSE(syn_tls_recv(&psk_tls, NULL, sizeof(rx_buf), &rx_len));
    TEST_ASSERT_FALSE(syn_tls_recv(&psk_tls, rx_buf, sizeof(rx_buf), NULL));
    TEST_ASSERT_FALSE(syn_tls_recv(&psk_tls, rx_buf, sizeof(rx_buf), &rx_len)); /* Empty wire */

    /* Short corrupt wire record */
    wire.len = 5;
    memset(wire.buf, 0x17, 5);
    TEST_ASSERT_FALSE(syn_tls_recv(&psk_tls, rx_buf, sizeof(rx_buf), &rx_len));

    /* Auto-handshake on uninitialized send/recv */
    SYN_TLS_Context auto_tls;
    TEST_ASSERT_TRUE(
        syn_tls_init(&auto_tls, &psk_cfg, &tr, rx_ok, sizeof(rx_ok), tx_ok, sizeof(tx_ok)));
    TEST_ASSERT_TRUE(syn_tls_send(&auto_tls, (const uint8_t *)"test", 4));

    SYN_TLS_Context auto_tls2;
    TEST_ASSERT_TRUE(
        syn_tls_init(&auto_tls2, &psk_cfg, &tr, rx_ok, sizeof(rx_ok), tx_ok, sizeof(tx_ok)));
    wire.len = 0;
    TEST_ASSERT_FALSE(syn_tls_recv(&auto_tls2, rx_buf, sizeof(rx_buf), &rx_len));

    /* Recv payload decryption */
    wire.len = 0;
    TEST_ASSERT_TRUE(syn_tls_send(&psk_tls, (const uint8_t *)"1234567890", 10));
    TEST_ASSERT_TRUE(syn_tls_recv(&psk_tls, rx_buf, sizeof(rx_buf), &rx_len));
    TEST_ASSERT_EQUAL(10, rx_len);

    /* Corrupted ciphertext MAC decryption failure */
    wire.len = 30;
    memset(wire.buf, 0x17, 30);
    TEST_ASSERT_FALSE(syn_tls_recv(&psk_tls, rx_buf, sizeof(rx_buf), &rx_len));

    /* App data ring buffer drain test */
    SYN_TLS_Context ring_tls;
    memset(&ring_tls, 0, sizeof(ring_tls));
    ring_tls.app_rx_buf[0] = 'X';
    ring_tls.app_rx_buf[1] = 'Y';
    ring_tls.app_rx_head = 0;
    ring_tls.app_rx_tail = 2;
    size_t ring_rx_len = 0;
    TEST_ASSERT_TRUE(syn_tls_recv(&ring_tls, rx_buf, 1, &ring_rx_len));
    TEST_ASSERT_EQUAL(1, ring_rx_len);
    TEST_ASSERT_EQUAL('X', rx_buf[0]);

    /* Complete drain to trigger head/tail reset */
    TEST_ASSERT_TRUE(syn_tls_recv(&ring_tls, rx_buf, sizeof(rx_buf), &ring_rx_len));
    TEST_ASSERT_EQUAL(1, ring_rx_len);
    TEST_ASSERT_EQUAL('Y', rx_buf[0]);

    /* Server app secret decrypt path test */
    SYN_TLS_Context srv_tls;
    TEST_ASSERT_TRUE(
        syn_tls_init(&srv_tls, &psk_cfg, &tr, rx_ok, sizeof(rx_ok), tx_ok, sizeof(tx_ok)));
    TEST_ASSERT_TRUE(syn_tls_handshake(&srv_tls));
    memcpy(srv_tls.client_app_secret, srv_tls.server_app_secret, 32);
    wire.len = 0;
    TEST_ASSERT_TRUE(syn_tls_send(&srv_tls, (const uint8_t *)"SERVER_TEST", 11));
    TEST_ASSERT_TRUE(syn_tls_recv(&srv_tls, rx_buf, sizeof(rx_buf), &rx_len));
    TEST_ASSERT_EQUAL(11, rx_len);

    /* Task error handling branch */
    SYN_TLS_Context err_tls;
    memset(&err_tls, 0, sizeof(err_tls));
    err_tls.state = SYN_TLS_STATE_ERROR;
    SYN_Task err_task = {.user_data = &err_tls};
    SYN_PT pt_err;
    PT_INIT(&pt_err);
    TEST_ASSERT_EQUAL(PT_ENDED, syn_tls_task(&pt_err, &err_task));

    /* Task null handling */
    SYN_Task null_task = {.user_data = NULL};
    SYN_PT pt_null;
    PT_INIT(&pt_null);
    TEST_ASSERT_EQUAL(PT_EXITED, syn_tls_task(&pt_null, &null_task));

    TEST_ASSERT_FALSE(syn_tls_is_established(NULL));
    TEST_ASSERT_EQUAL(SYN_TLS_STATE_UNINITIALIZED, syn_tls_get_state(NULL));
}

void run_tls_tests(void)
{
    RUN_TEST(test_tls_psk_mode_handshake_and_record_crypto);
    RUN_TEST(test_tls_transport_binding_interface);
    RUN_TEST(test_tls_task_protothread);
    RUN_TEST(test_tls_null_and_bounds_checks);
}
