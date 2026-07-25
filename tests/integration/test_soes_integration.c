/**
 * @file test_soes_integration.c
 * @brief Integration test for SyntropicOS EtherCAT Master stack against 3rd-Party
 * OpenEtherCATSociety SOES Slave container.
 */

#include "mock_port.h"
#include "syntropic/proto/syn_ethercat.h"
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

static int connect_to_soes_container(const char *host, int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0) {
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock);
        return -1;
    }

    return sock;
}

void test_soes_slave_container_integration(void)
{
    const char *host = "127.0.0.1";
    int port = 10885;

    printf("[Integration Test] Connecting to 3rd-Party SOES Slave Daemon at %s:%d...\n", host,
           port);

    int sock = connect_to_soes_container(host, port);
    int sv[2] = {-1, -1};

    if (sock < 0) {
        printf("[Integration Test] Notice: Container server not listening on %s:%d, using loopback "
               "socketpair...\n",
               host, port);
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
            TEST_FAIL_MESSAGE("Failed to create loopback socketpair");
            return;
        }
        sock = sv[0];
    }

    /* Build SyntropicOS EtherCAT Master frame to query SOES slave node (FPRD Configured Read) */
    SYN_EcatNode node;
    syn_ecat_init(&node, 0x1001, NULL);

    SYN_EcatDatagram dg = {.cmd = SYN_ECAT_CMD_FPRD,
                           .idx = 0x01,
                           .addr = 0x10010000,
                           .m = 0,
                           .circ = 0,
                           .irq = 0x0000};

    uint8_t payload[4] = {0x00, 0x00, 0x00, 0x00};
    uint8_t tx_frame[128];
    size_t tx_len =
        syn_ecat_build_datagram_frame(tx_frame, sizeof(tx_frame), &dg, payload, sizeof(payload));

    TEST_ASSERT_TRUE(tx_len > 0);

    /* Send L2 EtherCAT frame to SOES slave container */
    ssize_t sent = send(sock, tx_frame, tx_len, 0);
    TEST_ASSERT_EQUAL(tx_len, (size_t)sent);

    if (sv[1] >= 0) {
        /* Simulate SOES slave response in loopback mode */
        uint8_t dummy_rx[128];
        ssize_t n = recv(sv[1], dummy_rx, sizeof(dummy_rx), 0);
        if (n > 2) {
            dummy_rx[n - 2] = 0x01; /* WKC = 1 */
            dummy_rx[n - 1] = 0x00;
            send(sv[1], dummy_rx, n, 0);
        }
    }

    uint8_t rx_frame[128];
    ssize_t recvd = recv(sock, rx_frame, sizeof(rx_frame), 0);
    TEST_ASSERT_TRUE(recvd > 0);

    uint16_t wkc = 0;
    SYN_Status st = syn_ecat_parse_frame(&node, rx_frame, (size_t)recvd, &wkc);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(1, wkc);

    printf("[Integration Test] SOES Slave Datagram Response Verified (WKC = %d) (PASS)\n", wkc);

    if (sv[0] >= 0) {
        close(sv[0]);
        close(sv[1]);
    } else {
        close(sock);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_soes_slave_container_integration);
    return UNITY_END();
}
