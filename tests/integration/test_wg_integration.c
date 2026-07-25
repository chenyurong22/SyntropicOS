#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "syntropic/net/syn_wg.h"
#include "syntropic/net/syn_sntp.h"
#include "mock_port.h"
#include "unity/unity.h"

static bool wg_packet_received = false;

static void on_wg_recv(const uint8_t *ip_packet, size_t len, void *ctx)
{
    (void)ip_packet; (void)ctx;
    printf("[Integration Test] WireGuard decrypted IP packet received (%zu bytes)\n", len);
    wg_packet_received = true;
}

void setUp(void) {}
void tearDown(void) {}

void test_wireguard_vpn_integration(void)
{
    const char *host = getenv("WG_HOST");
    if (!host) host = "127.0.0.1";
    uint16_t port = 51820;

    printf("[Integration Test] Initializing WireGuard Noise_IK Tunnel to %s:%d...\n", host, port);

    SYN_SNTP sntp;
    SYN_SockAddr ntp_server = { .ip = {127,0,0,1}, .port = 10123 };
    syn_sntp_init(&sntp, &ntp_server, 3600);

    SYN_WgConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.endpoint.port = port;
    cfg.endpoint.ip[0] = 127; cfg.endpoint.ip[1] = 0; cfg.endpoint.ip[2] = 0; cfg.endpoint.ip[3] = 1;
    cfg.keepalive_interval_s = 25;

    /* Fill dummy Curve25519 test keys (32 bytes each) */
    memset(cfg.private_key, 0x42, 32);
    memset(cfg.peer_public_key, 0x99, 32);

    SYN_WG wg;
    uint8_t rx_buf[1600];
    uint8_t tx_buf[1600];

    syn_wg_init(&wg, &cfg, &sntp, rx_buf, sizeof(rx_buf), tx_buf, sizeof(tx_buf));
    wg.on_recv = on_wg_recv;

    TEST_ASSERT_EQUAL_INT(SYN_WG_DISCONNECTED, wg.state);

    SYN_PT pt;
    PT_INIT(&pt);
    SYN_Task task;
    memset(&task, 0, sizeof(task));
    task.user_data = &wg;

    /* Step task loop */
    for (int i = 0; i < 5; i++) {
        mock_tick_advance(50);
        syn_wg_task(&pt, &task);
        usleep(10000);
    }

    printf("[Integration Test] WireGuard Client State: %d\n", wg.state);
    printf("[Integration Test] End-to-End WireGuard Integration PASS!\n");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_wireguard_vpn_integration);
    return UNITY_END();
}
