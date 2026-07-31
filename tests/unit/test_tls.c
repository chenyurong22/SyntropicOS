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

    SYN_TLS_Context tls;
    TEST_ASSERT_TRUE(syn_tls_init(&tls, &config, &tr));

    /* Handshake completion */
    TEST_ASSERT_TRUE(syn_tls_handshake(&tls));
    TEST_ASSERT_EQUAL(SYN_TLS_STATE_ESTABLISHED, tls.state);

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

    SYN_TLS_Context tls;
    TEST_ASSERT_TRUE(syn_tls_init(&tls, &config, &raw_tr));

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

void test_tls_null_and_bounds_checks(void)
{
    TEST_ASSERT_FALSE(syn_tls_init(NULL, NULL, NULL));
    TEST_ASSERT_FALSE(syn_tls_handshake(NULL));
    TEST_ASSERT_FALSE(syn_tls_send(NULL, NULL, 0));
    TEST_ASSERT_FALSE(syn_tls_recv(NULL, NULL, 0, NULL));

    syn_tls_bind_transport(NULL, NULL);

    LoopbackTransport wire = {0};
    SYN_Transport tr = {.send = loopback_send, .recv = loopback_recv, .ctx = &wire};

    uint8_t long_psk[64];
    memset(long_psk, 0xAA, sizeof(long_psk));
    SYN_TLS_Config psk_cfg = {
        .mode = SYN_TLS_AUTH_MODE_PSK, .psk_secret = long_psk, .psk_secret_len = sizeof(long_psk)};
    SYN_TLS_Context psk_tls;
    TEST_ASSERT_TRUE(syn_tls_init(&psk_tls, &psk_cfg, &tr));
    TEST_ASSERT_TRUE(syn_tls_handshake(&psk_tls));
    TEST_ASSERT_TRUE(syn_tls_handshake(&psk_tls)); /* Repeat call when established */

    /* Zero length send */
    TEST_ASSERT_TRUE(syn_tls_send(&psk_tls, NULL, 0));
    TEST_ASSERT_FALSE(syn_tls_send(&psk_tls, NULL, 10));

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
    TEST_ASSERT_TRUE(syn_tls_init(&auto_tls, &psk_cfg, &tr));
    TEST_ASSERT_TRUE(syn_tls_send(&auto_tls, (const uint8_t *)"test", 4));

    SYN_TLS_Context auto_tls2;
    TEST_ASSERT_TRUE(syn_tls_init(&auto_tls2, &psk_cfg, &tr));
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
}

void run_tls_tests(void)
{
    RUN_TEST(test_tls_psk_mode_handshake_and_record_crypto);
    RUN_TEST(test_tls_transport_binding_interface);
    RUN_TEST(test_tls_null_and_bounds_checks);
}
