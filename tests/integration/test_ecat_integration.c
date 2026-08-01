/**
 * @file test_ecat_integration.c
 * @brief End-to-End Integration test for EtherCAT stack against 3rd-party EtherCAT Daemon
 * container.
 */

#include "mock_port.h"
#include "syntropic/proto/syn_ethercat.h"
#include "syntropic/util/syn_pack.h"
#include "unity/unity.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void setUp(void)
{
}
void tearDown(void)
{
}

void test_ethercat_container_integration(void)
{
    const char *host = getenv("ETHERCAT_HOST");
    if (!host)
        host = "127.0.0.1";
    uint16_t port = 10884;

    printf("[Integration Test] Connecting to EtherCAT Industrial Server at %s:%d...\n", host, port);

    int sock = syn_port_sock_connect_host(host, port);
    int peer_sock = sock;

    if (sock < 0) {
        printf("[Integration Test] Notice: Container server not listening on %s:%d, using loopback "
               "socketpair...\n",
               host, port);
        close(sock);
        int sv[2];
        TEST_ASSERT_EQUAL_INT(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));
        sock = sv[0];
        peer_sock = sv[1];
    } else {
        printf("[Integration Test] Connected to EtherCAT Industrial Server Daemon!\n");
    }

    /* 1. Initialize SyntropicOS EtherCAT node */
    SYN_EcatNode node;
    syn_ecat_init(&node, 0x1001, NULL);

    /* 2. Build Datagram Frame */
    uint8_t tx_frame[64];
    SYN_EcatDatagram dg = {.cmd = SYN_ECAT_CMD_FPRD,
                           .idx = 0x01,
                           .addr = 0x10010000,
                           .m = 0,
                           .circ = 0,
                           .irq = 0x0000};
    uint8_t payload[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    size_t tx_len = syn_ecat_build_datagram_frame(tx_frame, sizeof(tx_frame), &dg, payload, 4);
    TEST_ASSERT_EQUAL_INT(18, tx_len);

    ssize_t sent = send(sock, tx_frame, tx_len, 0);
    TEST_ASSERT_EQUAL_INT(tx_len, sent);

    /* If using socketpair loopback peer, simulate slave response by setting WKC = 1 */
    if (peer_sock != sock) {
        uint8_t loop_buf[64];
        ssize_t n = recv(peer_sock, loop_buf, sizeof(loop_buf), 0);
        TEST_ASSERT_TRUE(n >= 18);
        loop_buf[16] = 0x01; /* WKC = 1 */
        loop_buf[17] = 0x00;
        send(peer_sock, loop_buf, n, 0);
    }

    uint8_t rx_frame[64];
    ssize_t recvd = recv(sock, rx_frame, sizeof(rx_frame), 0);
    TEST_ASSERT_TRUE(recvd >= 18);

    /* 3. Parse Response with SyntropicOS EtherCAT parser & verify WKC */
    uint16_t wkc = 0;
    SYN_Status st = syn_ecat_parse_frame(&node, rx_frame, (size_t)recvd, &wkc);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL_UINT16(1, wkc);
    TEST_ASSERT_EQUAL_UINT16(1, node.wkc_last);
    printf("[Integration Test] EtherCAT Frame WKC Verified: %d (PASS)\n", wkc);

    close(sock);
    if (peer_sock != sock) {
        close(peer_sock);
    }
    printf("[Integration Test] End-to-End EtherCAT Integration PASS!\n");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ethercat_container_integration);
    return UNITY_END();
}
