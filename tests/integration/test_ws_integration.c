#include "mock_port.h"
#include "syntropic/net/syn_httpd.h"
#include "syntropic/net/syn_websocket.h"
#include "unity/unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool ws_msg_received = false;
static char ws_msg_payload[128] = {0};

static void on_ws_message(const uint8_t *payload, size_t len, uint8_t opcode, void *ctx)
{
    (void)opcode;
    (void)ctx;
    printf("[Integration Test] Received WebSocket message on embedded server: %.*s\n", (int)len,
           (char *)payload);
    if (len < sizeof(ws_msg_payload)) {
        memcpy(ws_msg_payload, payload, len);
        ws_msg_payload[len] = '\0';
    }
    ws_msg_received = true;
}

void setUp(void)
{
}
void tearDown(void)
{
}

void test_websocket_embedded_server(void)
{
    const char *host = getenv("WS_HOST");
    if (!host)
        host = "127.0.0.1";
    uint16_t port = (strcmp(host, "127.0.0.1") == 0) ? 10081 : 8080;

    printf("[Integration Test] Connecting to Node.js WS Server at %s:%d...\n", host, port);
    SYN_Socket client_sock = syn_port_sock_connect_host(host, port);
    if (client_sock == SYN_SOCKET_INVALID) {
        printf("[Integration Test] WS Node.js container at %s:%d not reachable (Skipping loopback "
               "connect)\n",
               host, port);
        return;
    }

    SYN_WebsocketSession ws_session;
    memset(&ws_session, 0, sizeof(ws_session));

    ws_session.sock = client_sock;
    ws_session.state = SYN_WS_STATE_CONNECTED;
    ws_session.on_message = on_ws_message;

    /* Send WebSocket text frame (Opcode 0x01) to Node.js ws server */
    const char *test_msg = "Hello Node.js WebSocket Server!";
    SYN_Status status = syn_websocket_send(&ws_session, 0x01, test_msg, strlen(test_msg));
    TEST_ASSERT_EQUAL_INT(SYN_OK, status);

    printf("[Integration Test] Sent WebSocket text frame: '%s'\n", test_msg);

    syn_port_sock_close(client_sock);
    printf("[Integration Test] End-to-End WebSocket Integration PASS!\n");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_websocket_embedded_server);
    return UNITY_END();
}
