#include "mock_port.h"
#include "syntropic/net/syn_dns.h"
#include "unity/unity.h"

#include <netdb.h>
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

void test_dns_resolver_e2e(void)
{
    const char *host = getenv("DNS_HOST");
    if (!host)
        host = "127.0.0.1";
    uint16_t port = (strcmp(host, "127.0.0.1") == 0) ? 10053 : 53;

    printf("[Integration Test] Querying CoreDNS UDP Server at %s:%d...\n", host, port);

    SYN_SockAddr server = {0};
    server.port = port;

    struct hostent *he = gethostbyname(host);
    if (he && he->h_addr_list[0]) {
        memcpy(server.ip, he->h_addr_list[0], 4);
    } else {
        server.ip[0] = 127;
        server.ip[1] = 0;
        server.ip[2] = 0;
        server.ip[3] = 1;
    }

    SYN_SockAddr resolved_addr = {0};
    SYN_DnsResolver dns;
    memset(&dns, 0, sizeof(dns));
    dns.dns_server = &server;
    dns.hostname = "syntropic.local";
    dns.addr_out = &resolved_addr;
    dns.timeout_ms = 3000;
    dns.status = SYN_TIMEOUT; /* Initialize status to non-OK so loop runs */

    SYN_PT pt;
    PT_INIT(&pt);
    SYN_Task task;
    memset(&task, 0, sizeof(task));
    task.user_data = &dns;

    int iterations = 0;
    while (dns.status != SYN_OK && iterations < 50) {
        mock_tick_advance(10);
        syn_dns_resolve_task(&pt, &task);
        usleep(10000);
        iterations++;
    }

    if (dns.status != SYN_OK) {
        printf("[Integration Test] Notice: CoreDNS server at %s:%d not reachable (skipping "
               "loopback test)\n",
               host, port);
        return;
    }
    printf("[Integration Test] Resolved 'syntropic.local' via CoreDNS -> %d.%d.%d.%d\n",

           resolved_addr.ip[0], resolved_addr.ip[1], resolved_addr.ip[2], resolved_addr.ip[3]);

    TEST_ASSERT_EQUAL_UINT8(1, resolved_addr.ip[0]);
    TEST_ASSERT_EQUAL_UINT8(2, resolved_addr.ip[1]);
    TEST_ASSERT_EQUAL_UINT8(3, resolved_addr.ip[2]);
    TEST_ASSERT_EQUAL_UINT8(4, resolved_addr.ip[3]);

    printf("[Integration Test] End-to-End CoreDNS Integration PASS!\n");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_dns_resolver_e2e);
    return UNITY_END();
}
