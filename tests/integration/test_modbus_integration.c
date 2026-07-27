#include "mock_port.h"
#include "syntropic/proto/syn_modbus.h"
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

void test_modbus_tcp_integration(void)
{
    const char *host = getenv("MODBUS_HOST");
    if (!host)
        host = "127.0.0.1";
    uint16_t port = 5020;

    printf("[Integration Test] Connecting to Modbus TCP Server at %s:%d...\n", host, port);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT_TRUE(sock >= 0);

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    inet_pton(AF_INET, host, &sa.sin_addr);

    int res = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
    if (res != 0) {
        close(sock);
        printf("[Integration Test] Notice: Modbus TCP server at %s:%d not reachable (skipping "
               "loopback test)\n",
               host, port);
        return;
    }
    printf("[Integration Test] Connected to Modbus TCP Server!\n");

    /* Build Modbus TCP Read Holding Registers Request (FC 0x03, Reg 0, Count 2) */
    /* MBAP: TxID=0x0001, ProtoID=0x0000, Length=6, UnitID=1 */
    /* PDU: FC=0x03, StartAddr=0x0000, RegCount=0x0002 */
    uint8_t req[12] = {
        0x00,
        0x01, /* TxID */
        0x00,
        0x00, /* ProtoID */
        0x00,
        0x06,                   /* Length */
        0x01,                   /* UnitID */
        SYN_MB_FC_READ_HOLDING, /* FC 0x03 */
        0x00,
        0x00, /* Start Address */
        0x00,
        0x02 /* Count */
    };

    ssize_t sent = send(sock, req, sizeof(req), 0);
    TEST_ASSERT_EQUAL_INT(sizeof(req), sent);

    uint8_t resp[32];
    ssize_t recvd = recv(sock, resp, sizeof(resp), 0);
    TEST_ASSERT_TRUE(recvd >= 11);

    /* Verify MBAP & FC */
    TEST_ASSERT_EQUAL_UINT8(0x01, resp[6]);                   /* UnitID */
    TEST_ASSERT_EQUAL_UINT8(SYN_MB_FC_READ_HOLDING, resp[7]); /* FC 0x03 */
    TEST_ASSERT_EQUAL_UINT8(4, resp[8]);                      /* Byte count: 4 bytes for 2 regs */

    /* Reg 0 = 0x1234, Reg 1 = 0x5678 */
    uint16_t reg0 = (resp[9] << 8) | resp[10];
    uint16_t reg1 = (resp[11] << 8) | resp[12];

    printf("[Integration Test] Read Holding Reg 0: 0x%04X, Reg 1: 0x%04X\n", reg0, reg1);
    TEST_ASSERT_EQUAL_HEX16(0x1234, reg0);
    TEST_ASSERT_EQUAL_HEX16(0x5678, reg1);

    close(sock);
    printf("[Integration Test] End-to-End Modbus TCP Integration PASS!\n");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_modbus_tcp_integration);
    return UNITY_END();
}
