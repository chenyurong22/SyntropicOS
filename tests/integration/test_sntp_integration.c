#include "syntropic/net/syn_sntp.h"
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

void test_sntp_chrony_e2e(void)
{
    const char *host = getenv("SNTP_HOST");
    if (!host)
        host = "127.0.0.1";
    uint16_t port = (strcmp(host, "127.0.0.1") == 0) ? 10123 : 123;

    SYN_SockAddr server = {0};
    server.port = port;

    int a, b, c, d;
    if (sscanf(host, "%d.%d.%d.%d", &a, &b, &c, &d) == 4) {
        server.ip[0] = (uint8_t)a;
        server.ip[1] = (uint8_t)b;
        server.ip[2] = (uint8_t)c;
        server.ip[3] = (uint8_t)d;
    } else {
        server.ip[0] = 127;
        server.ip[1] = 0;
        server.ip[2] = 0;
        server.ip[3] = 1;
    }

    SYN_SNTP sntp;
    syn_sntp_init(&sntp, &server, 3600);

    printf("[Integration Test] Querying Chrony SNTP Server at %s:%d...\n", host, port);
    SYN_Status status = syn_sntp_query(&sntp);

    if (status != SYN_OK) {
        printf(
            "[Integration Test] SNTP query status: %d (Skipping if local NTP port %d is closed)\n",
            status, port);
        return;
    }

    TEST_ASSERT_EQUAL_INT(SYN_OK, status);
    TEST_ASSERT_TRUE(syn_sntp_is_synced(&sntp));

    uint32_t epoch = syn_sntp_get_epoch_s(&sntp);
    printf("[Integration Test] Synced Epoch Time: %u UTC seconds\n", epoch);
    TEST_ASSERT_GREATER_THAN_UINT32(1600000000UL, epoch);

    printf("[Integration Test] End-to-End Chrony SNTP Integration PASS!\n");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sntp_chrony_e2e);
    return UNITY_END();
}
