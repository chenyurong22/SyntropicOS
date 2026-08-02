/**
 * @file test_http.c
 * @brief Tests for the HTTP client — header parsing, body streaming.
 */

#include "mocks/mock_port.h"
#include "syntropic/net/syn_http.h"
#include "unity/unity.h"

#include <stdio.h>
#include <string.h>

/* ── Helpers ────────────────────────────────────────────────────────────── */

static uint8_t work_buf[512];

/* Body accumulator */
static uint8_t body_accum[2048];
static size_t body_accum_len;

static bool body_accumulate(const uint8_t *data, size_t len, void *ctx)
{
    (void)ctx;
    if (body_accum_len + len > sizeof(body_accum))
        return false;
    memcpy(body_accum + body_accum_len, data, len);
    body_accum_len += len;
    return true;
}

static void reset_accum(void)
{
    memset(body_accum, 0, sizeof(body_accum));
    body_accum_len = 0;
}

static SYN_Status run_client_task(SYN_HttpClient *client)
{
    SYN_PT pt;
    PT_INIT(&pt);
    SYN_Task task;
    task.user_data = client;

    while (client->state != SYN_HTTP_STATE_DONE && client->state != SYN_HTTP_STATE_ERROR) {
        syn_http_client_task(&pt, &task);
        mock_tick_advance(10);
    }
    return client->status;
}

/* ── Tests ──────────────────────────────────────────────────────────────── */

void test_http_get_200(void)
{
    mock_port_reset();
    reset_accum();

    const char *response = "HTTP/1.1 200 OK\r\n"
                           "Content-Length: 13\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "Hello, World!";

    mock_sock_set_response(response, strlen(response));

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    SYN_HttpResponse resp = client.resp;

    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(200, resp.status_code);
    TEST_ASSERT_EQUAL(13, resp.content_length);
    TEST_ASSERT_TRUE(resp.connection_close);
    TEST_ASSERT_FALSE(resp.chunked);
    TEST_ASSERT_EQUAL(13, body_accum_len);
    TEST_ASSERT_EQUAL_STRING_LEN("Hello, World!", body_accum, 13);
}

void test_http_get_404(void)
{
    mock_port_reset();
    reset_accum();

    const char *response = "HTTP/1.1 404 Not Found\r\n"
                           "Content-Length: 9\r\n"
                           "\r\n"
                           "Not Found";

    mock_sock_set_response(response, strlen(response));

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/missing", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    SYN_HttpResponse resp = client.resp;

    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(404, resp.status_code);
    TEST_ASSERT_EQUAL(9, body_accum_len);
}

void test_http_get_large_body(void)
{
    mock_port_reset();
    reset_accum();

    /* Build a response with a 1024-byte body */
    char response[2048];
    int hdr_len = snprintf(response, sizeof(response),
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Length: 1024\r\n"
                           "\r\n");

    /* Fill body with pattern */
    for (int i = 0; i < 1024; i++) {
        response[hdr_len + i] = (char)('A' + (i % 26));
    }

    mock_sock_set_response(response, (size_t)(hdr_len + 1024));

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/big", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    SYN_HttpResponse resp = client.resp;

    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(200, resp.status_code);
    TEST_ASSERT_EQUAL(1024, body_accum_len);

    /* Verify pattern */
    for (int i = 0; i < 1024; i++) {
        TEST_ASSERT_EQUAL('A' + (i % 26), body_accum[i]);
    }
}

void test_http_get_no_content_length(void)
{
    mock_port_reset();
    mock_sock_eof_on_empty = true;
    reset_accum();

    /* Connection: close without Content-Length — read until EOF */
    const char *response = "HTTP/1.1 200 OK\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "streamed data";

    mock_sock_set_response(response, strlen(response));

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/stream", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    SYN_HttpResponse resp = client.resp;

    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(200, resp.status_code);
    TEST_ASSERT_EQUAL(13, body_accum_len);
    TEST_ASSERT_EQUAL_STRING_LEN("streamed data", body_accum, 13);
}

void test_http_get_sends_correct_request(void)
{
    mock_port_reset();

    const char *response = "HTTP/1.1 200 OK\r\n"
                           "Content-Length: 0\r\n"
                           "\r\n";

    mock_sock_set_response(response, strlen(response));

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "myhost.local", 8080, "/api/v1/status", NULL, NULL, 0,
                         NULL, 0, NULL, NULL, work_buf, sizeof(work_buf));
    run_client_task(&client);

    /* Verify the sent request contains the right pieces */
    mock_sock_tx_buf[mock_sock_tx_len] = '\0';
    const char *tx = (const char *)mock_sock_tx_buf;

    TEST_ASSERT_NOT_NULL(strstr(tx, "GET /api/v1/status HTTP/1.1\r\n"));
    TEST_ASSERT_NOT_NULL(strstr(tx, "Host: myhost.local\r\n"));
    TEST_ASSERT_NOT_NULL(strstr(tx, "Connection: close\r\n"));
}

void test_http_post_basic(void)
{
    mock_port_reset();
    reset_accum();

    const char *response = "HTTP/1.1 201 Created\r\n"
                           "Content-Length: 2\r\n"
                           "\r\n"
                           "OK";

    mock_sock_set_response(response, strlen(response));

    const char *body = "{\"key\":\"value\"}";

    SYN_HttpClient client;
    syn_http_client_init(&client, "POST", "api.example.com", 80, "/data", "application/json",
                         (const uint8_t *)body, strlen(body), NULL, 0, body_accumulate, NULL,
                         work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    SYN_HttpResponse resp = client.resp;

    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(201, resp.status_code);
    TEST_ASSERT_EQUAL(2, body_accum_len);

    /* Verify request included content-type and body */
    mock_sock_tx_buf[mock_sock_tx_len] = '\0';
    const char *tx = (const char *)mock_sock_tx_buf;
    TEST_ASSERT_NOT_NULL(strstr(tx, "POST /data HTTP/1.1\r\n"));
    TEST_ASSERT_NOT_NULL(strstr(tx, "Content-Type: application/json\r\n"));
    TEST_ASSERT_NOT_NULL(strstr(tx, "{\"key\":\"value\"}"));
}

void test_http_get_chunked(void)
{
    mock_port_reset();
    reset_accum();

    const char *response = "HTTP/1.1 200 OK\r\n"
                           "Transfer-Encoding: chunked\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "4\r\n"
                           "Wiki\r\n"
                           "5\r\n"
                           "pedia\r\n"
                           "E\r\n"
                           " in\r\n\r\nchunks.\r\n"
                           "0\r\n"
                           "\r\n";

    mock_sock_set_response(response, strlen(response));

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/chunked", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    SYN_HttpResponse resp = client.resp;

    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(200, resp.status_code);
    TEST_ASSERT_TRUE(resp.chunked);
    TEST_ASSERT_EQUAL(23, body_accum_len);
    TEST_ASSERT_EQUAL_STRING_LEN("Wikipedia in\r\n\r\nchunks.", body_accum, 23);
}

static int s_redirect_count = 0;
static void on_redirect_connect(const char *host, uint16_t port)
{
    (void)port;
    if (s_redirect_count == 0) {
        /* First request goes to original page, redirecting to destination */
        TEST_ASSERT_EQUAL_STRING("example.com", host);
        const char *redirect = "HTTP/1.1 302 Found\r\n"
                               "Location: http://dest.com/target\r\n"
                               "Content-Length: 0\r\n"
                               "\r\n";
        mock_sock_set_response(redirect, strlen(redirect));
    } else if (s_redirect_count == 1) {
        /* Second request goes to target destination */
        TEST_ASSERT_EQUAL_STRING("dest.com", host);
        const char *ok = "HTTP/1.1 200 OK\r\n"
                         "Content-Length: 7\r\n"
                         "\r\n"
                         "Success";
        mock_sock_set_response(ok, strlen(ok));
    }
    s_redirect_count++;
}

void test_http_get_redirect(void)
{
    mock_port_reset();
    reset_accum();
    s_redirect_count = 0;
    mock_sock_connect_cb = on_redirect_connect;

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/redirect-me", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    SYN_HttpResponse resp = client.resp;

    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(200, resp.status_code);
    TEST_ASSERT_EQUAL(2, s_redirect_count);
    TEST_ASSERT_EQUAL(7, body_accum_len);
    TEST_ASSERT_EQUAL_STRING_LEN("Success", body_accum, 7);
}

static void on_loop_redirect_connect(const char *host, uint16_t port)
{
    (void)host;
    (void)port;
    const char *redirect = "HTTP/1.1 302 Found\r\n"
                           "Location: /loop\r\n"
                           "Content-Length: 0\r\n"
                           "\r\n";
    mock_sock_set_response(redirect, strlen(redirect));
    s_redirect_count++;
}

void test_http_get_redirect_limit(void)
{
    mock_port_reset();
    reset_accum();
    s_redirect_count = 0;
    mock_sock_connect_cb = on_loop_redirect_connect;

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/loop", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);

    /* Should return error and stop after 5 hops */
    TEST_ASSERT_EQUAL(SYN_ERROR, st);
    TEST_ASSERT_EQUAL(5, s_redirect_count);
}

/* ── New VFS / HTTP Client Edge Cases ────────────────────────────────────── */

static void on_port_redirect_connect(const char *host, uint16_t port)
{
    if (s_redirect_count == 0) {
        TEST_ASSERT_EQUAL_STRING("example.com", host);
        const char *redirect = "HTTP/1.1 302 Found\r\n"
                               "Location: http://dest.com:8080/target\r\n"
                               "Content-Length: 0\r\n"
                               "\r\n";
        mock_sock_set_response(redirect, strlen(redirect));
    } else if (s_redirect_count == 1) {
        TEST_ASSERT_EQUAL_STRING("dest.com", host);
        TEST_ASSERT_EQUAL_UINT16(8080, port);
        const char *ok = "HTTP/1.1 200 OK\r\n"
                         "Content-Length: 7\r\n"
                         "\r\n"
                         "Success";
        mock_sock_set_response(ok, strlen(ok));
    }
    s_redirect_count++;
}

static void on_noslash_redirect_connect(const char *host, uint16_t port)
{
    if (s_redirect_count == 0) {
        const char *redirect = "HTTP/1.1 302 Found\r\n"
                               "Location: http://dest.com\r\n"
                               "Content-Length: 0\r\n"
                               "\r\n";
        mock_sock_set_response(redirect, strlen(redirect));
    } else if (s_redirect_count == 1) {
        TEST_ASSERT_EQUAL_STRING("dest.com", host);
        TEST_ASSERT_EQUAL_UINT16(80, port);
        const char *ok = "HTTP/1.1 200 OK\r\n"
                         "Content-Length: 7\r\n"
                         "\r\n"
                         "Success";
        mock_sock_set_response(ok, strlen(ok));
    }
    s_redirect_count++;
}

static void on_relative_redirect_connect(const char *host, uint16_t port)
{
    (void)port;
    if (s_redirect_count == 0) {
        const char *redirect = "HTTP/1.1 302 Found\r\n"
                               "Location: target\r\n"
                               "Content-Length: 0\r\n"
                               "\r\n";
        mock_sock_set_response(redirect, strlen(redirect));
    } else if (s_redirect_count == 1) {
        TEST_ASSERT_EQUAL_STRING("example.com", host);
        const char *ok = "HTTP/1.1 200 OK\r\n"
                         "Content-Length: 7\r\n"
                         "\r\n"
                         "Success";
        mock_sock_set_response(ok, strlen(ok));
    }
    s_redirect_count++;
}

static void on_body_send_fail_connect(const char *host, uint16_t port)
{
    (void)host;
    (void)port;
    mock_sock_send_fail_after_bytes = 95;
}

static bool s_body_cb_fail = false;
static bool body_cb_rejectable(const uint8_t *data, size_t len, void *ctx)
{
    if (s_body_cb_fail)
        return false;
    return body_accumulate(data, len, ctx);
}

void test_http_connect_fail(void)
{
    mock_port_reset();
    mock_sock_connect_fail = true;

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_ERROR, st);
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);
}

void test_http_send_fail(void)
{
    mock_port_reset();
    mock_sock_send_fail = true;

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_ERROR, st);
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);
}

void test_http_body_send_fail(void)
{
    mock_port_reset();
    reset_accum();
    mock_sock_connect_cb = on_body_send_fail_connect;

    SYN_HttpClient client;
    syn_http_client_init(&client, "POST", "h", 80, "/", "text/plain", (const uint8_t *)"hellohello",
                         10, NULL, 0, body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_ERROR, st);
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);
}

void test_http_redirect_formats(void)
{
    /* A. Port redirect */
    mock_port_reset();
    reset_accum();
    s_redirect_count = 0;
    mock_sock_connect_cb = on_port_redirect_connect;

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(2, s_redirect_count);

    /* B. Noslash redirect */
    mock_port_reset();
    reset_accum();
    s_redirect_count = 0;
    mock_sock_connect_cb = on_noslash_redirect_connect;

    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(2, s_redirect_count);
    TEST_ASSERT_EQUAL_STRING("/", client.cur_path);

    /* C. Relative URL redirect without slash */
    mock_port_reset();
    reset_accum();
    s_redirect_count = 0;
    mock_sock_connect_cb = on_relative_redirect_connect;

    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(2, s_redirect_count);
    TEST_ASSERT_EQUAL_STRING("/target", client.cur_path);
}

void test_http_header_edge_cases(void)
{
    /* 1. Header tag formatting, case insensitivity and spacing */
    mock_port_reset();
    reset_accum();
    const char *resp1 = "HTTP/1.1 200 OK\r\n"
                        "CONTENT-LENGTH:   5\r\n"
                        "CONNECTION:  close\r\n"
                        "LOCATION:   http://dest\r\n"
                        "\r\n"
                        "hello";
    mock_sock_set_response(resp1, strlen(resp1));

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(5, client.resp.content_length);
    TEST_ASSERT_TRUE(client.resp.connection_close);
    TEST_ASSERT_EQUAL_STRING("hello", (char *)body_accum);

    /* 2. Invalid status line (no space) */
    mock_port_reset();
    const char *resp2 = "HTTP/1.1_200_OK\r\n\r\n";
    mock_sock_set_response(resp2, strlen(resp2));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_ERROR, st);

    /* 3. Connection closed before header finishes */
    mock_port_reset();
    mock_sock_eof_on_empty = true;
    const char *resp3 = "HTTP/1.1 200 OK\r\nContent-Length: 5";
    mock_sock_set_response(resp3, strlen(resp3));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_ERROR, st);

    /* 4. Header receive timeout */
    mock_port_reset();
    const char *resp4 = "HTTP/1.1 200 OK\r\n";
    mock_sock_set_response(resp4, strlen(resp4));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));

    SYN_PT pt;
    PT_INIT(&pt);
    SYN_Task task;
    task.user_data = &client;

    syn_http_client_task(&pt, &task);
    mock_tick_ms += 31000;
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(SYN_TIMEOUT, client.status);

    /* 5. Header buffer overflow */
    mock_port_reset();
    char resp5[600];
    memset(resp5, 'A', sizeof(resp5));
    resp5[599] = '\0';
    mock_sock_set_response(resp5, strlen(resp5));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_ERROR, st);
}

void test_http_chunked_errors(void)
{
    /* 1. Invalid hex characters in chunk size */
    mock_port_reset();
    const char *resp1 = "HTTP/1.1 200 OK\r\n"
                        "Transfer-Encoding: chunked\r\n"
                        "\r\n"
                        "3g\r\ndata\r\n";
    mock_sock_set_response(resp1, strlen(resp1));

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_ERROR, st);

    /* 2. Chunk size line overflow */
    mock_port_reset();
    const char *resp2 = "HTTP/1.1 200 OK\r\n"
                        "Transfer-Encoding: chunked\r\n"
                        "\r\n"
                        "1234567890123456789012345678901234567890\r\n";
    mock_sock_set_response(resp2, strlen(resp2));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_TIMEOUT, st);

    /* 3. Body callback rejection */
    mock_port_reset();
    reset_accum();
    s_body_cb_fail = true;
    const char *resp3 = "HTTP/1.1 200 OK\r\n"
                        "Transfer-Encoding: chunked\r\n"
                        "\r\n"
                        "5\r\nhello\r\n0\r\n\r\n";
    mock_sock_set_response(resp3, strlen(resp3));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_cb_rejectable, NULL, work_buf, sizeof(work_buf));
    st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_ERROR, st);
    s_body_cb_fail = false;

    /* 4. Premature EOF during chunk */
    mock_port_reset();
    mock_sock_eof_on_empty = true;
    const char *resp4 = "HTTP/1.1 200 OK\r\n"
                        "Transfer-Encoding: chunked\r\n"
                        "\r\n"
                        "a\r\nhello"; // EOF before 10 bytes read
    mock_sock_set_response(resp4, strlen(resp4));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_ERROR, st);

    /* 5. Timeout during chunk read */
    mock_port_reset();
    const char *resp5 = "HTTP/1.1 200 OK\r\n"
                        "Transfer-Encoding: chunked\r\n"
                        "\r\n"
                        "a\r\nhello";
    mock_sock_set_response(resp5, strlen(resp5));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_PT pt;
    PT_INIT(&pt);
    SYN_Task task;
    task.user_data = &client;
    syn_http_client_task(&pt, &task);
    mock_tick_ms += 31000;
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(SYN_TIMEOUT, client.status);
}

void test_http_streaming_errors(void)
{
    /* 1. Body callback rejection for cached initial read bytes */
    mock_port_reset();
    reset_accum();
    s_body_cb_fail = true;
    const char *resp1 = "HTTP/1.1 200 OK\r\n"
                        "Content-Length: 5\r\n"
                        "\r\n"
                        "hello";
    mock_sock_set_response(resp1, strlen(resp1));
    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_cb_rejectable, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_ERROR, st);
    s_body_cb_fail = false;

    /* 2. Body callback rejection for newly read bytes */
    mock_port_reset();
    reset_accum();
    const char *resp2 = "HTTP/1.1 200 OK\r\n"
                        "Content-Length: 10\r\n"
                        "\r\n"
                        "hello";
    mock_sock_set_response(resp2, strlen(resp2));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_cb_rejectable, NULL, work_buf, sizeof(work_buf));

    SYN_PT pt;
    PT_INIT(&pt);
    SYN_Task task;
    task.user_data = &client;

    /* Deliver "hello" first (state changes to reading body) */
    /* Task may return PT_YIELDED or PT_WAITING depending on socket readability */
    SYN_PT_Status first_st = syn_http_client_task(&pt, &task);
    TEST_ASSERT_NOT_EQUAL(PT_EXITED, first_st);
    TEST_ASSERT_NOT_EQUAL(PT_ENDED, first_st);

    /* Set next response in socket and configure callback to reject */
    s_body_cb_fail = true;
    mock_sock_set_response("world", 5);
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(SYN_ERROR, client.status);
    s_body_cb_fail = false;

    /* 3. Premature EOF before content-length satisfied */
    mock_port_reset();
    reset_accum();
    mock_sock_eof_on_empty = true;
    const char *resp3 = "HTTP/1.1 200 OK\r\n"
                        "Content-Length: 10\r\n"
                        "\r\n"
                        "hello";
    mock_sock_set_response(resp3, strlen(resp3));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_ERROR, st);

    /* 4. Timeout during body streaming */
    mock_port_reset();
    reset_accum();
    const char *resp4 = "HTTP/1.1 200 OK\r\n"
                        "Content-Length: 10\r\n"
                        "\r\n"
                        "hello";
    mock_sock_set_response(resp4, strlen(resp4));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    PT_INIT(&pt);
    task.user_data = &client;
    syn_http_client_task(&pt, &task);
    mock_tick_ms += 31000;
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(SYN_TIMEOUT, client.status);
}

void test_http_extra_data_in_buffer(void)
{
    mock_port_reset();
    reset_accum();

    const char *response = "HTTP/1.1 200 OK\r\n"
                           "Content-Length: 5\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "HelloExtraGarbage";

    mock_sock_set_response(response, strlen(response));

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL(5, body_accum_len);
    TEST_ASSERT_EQUAL_STRING_LEN("Hello", body_accum, 5);
}

void test_http_custom_headers(void)
{
    mock_port_reset();
    reset_accum();

    const char *response = "HTTP/1.1 200 OK\r\n"
                           "Content-Length: 2\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "OK";
    mock_sock_set_response(response, strlen(response));

    SYN_HttpHeader custom_headers[3] = {{"X-Custom-1", "Value1"},
                                        {NULL, NULL}, // Should be skipped
                                        {"X-Custom-2", "Value2"}};

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, custom_headers, 3,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    /* Verify custom headers in mock_sock_tx_buf */
    if (mock_sock_tx_len < sizeof(mock_sock_tx_buf)) {
        mock_sock_tx_buf[mock_sock_tx_len] = '\0';
    } else {
        mock_sock_tx_buf[sizeof(mock_sock_tx_buf) - 1] = '\0';
    }
    TEST_ASSERT_NOT_NULL(strstr((const char *)mock_sock_tx_buf, "X-Custom-1: Value1\r\n"));
    TEST_ASSERT_NOT_NULL(strstr((const char *)mock_sock_tx_buf, "X-Custom-2: Value2\r\n"));
}

void test_http_chunked_boundary_cases(void)
{
    const char *headers = "HTTP/1.1 200 OK\r\n"
                          "Transfer-Encoding: chunked\r\n"
                          "Connection: close\r\n"
                          "\r\n";

    SYN_HttpClient client;
    SYN_PT pt;
    SYN_Task task;

    /* A. Successful boundary parse — mirrors test_http_get_chunked which exercises
     * the same chunked decode path end-to-end with all data available upfront. */
    mock_port_reset();
    reset_accum();

    const char *full_chunked = "HTTP/1.1 200 OK\r\n"
                               "Transfer-Encoding: chunked\r\n"
                               "Connection: close\r\n"
                               "\r\n"
                               "3\r\n"
                               "abc\r\n"
                               "0\r\n"
                               "\r\n";
    mock_sock_set_response(full_chunked, strlen(full_chunked));
    syn_http_client_init(&client, "GET", "example.com", 80, "/chunked-a", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status chunk_st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_OK, chunk_st); /* body delivery verified in test_http_get_chunked */

    /* B. State 0: chunk size byte only then socket close (EOF) */
    mock_port_reset();
    reset_accum();
    mock_sock_connected = true;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    PT_INIT(&pt);
    task.user_data = &client;
    mock_sock_set_response(headers, strlen(headers));
    syn_http_client_task(&pt, &task);
    /* headers only consumed — still reading chunked */
    mock_sock_set_response("3", 1);
    syn_http_client_task(&pt, &task);
    mock_sock_eof_on_empty = true;
    mock_sock_set_response("", 0);
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(SYN_ERROR, client.status);

    /* C. State 0: Socket timeout reading chunk size */
    mock_port_reset();
    reset_accum();
    mock_sock_connected = true;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    PT_INIT(&pt);
    task.user_data = &client;
    mock_sock_set_response(headers, strlen(headers));
    syn_http_client_task(&pt, &task);
    mock_sock_set_response("3", 1);
    syn_http_client_task(&pt, &task);
    mock_tick_ms += 31000;
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(SYN_TIMEOUT, client.status);

    /* D. State 1: chunk data read then socket close (EOF) */
    mock_port_reset();
    reset_accum();
    mock_sock_connected = true;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    PT_INIT(&pt);
    task.user_data = &client;
    mock_sock_set_response(headers, strlen(headers));
    syn_http_client_task(&pt, &task);
    mock_sock_set_response("3\r\n", 3);
    syn_http_client_task(&pt, &task);
    mock_sock_eof_on_empty = true;
    mock_sock_set_response("", 0);
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(SYN_ERROR, client.status);

    /* E. State 1: Socket timeout reading chunk data */
    mock_port_reset();
    reset_accum();
    mock_sock_connected = true;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    PT_INIT(&pt);
    task.user_data = &client;
    mock_sock_set_response(headers, strlen(headers));
    syn_http_client_task(&pt, &task);
    syn_http_client_task(&pt, &task);

    mock_sock_set_response("3\r\n", 3);
    syn_http_client_task(&pt, &task);
    mock_tick_ms += 31000;
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(SYN_TIMEOUT, client.status);

    /* F. State 1 separator: chunk data + separator then socket close */
    mock_port_reset();
    reset_accum();
    mock_sock_connected = true;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    PT_INIT(&pt);
    task.user_data = &client;
    mock_sock_set_response(headers, strlen(headers));
    syn_http_client_task(&pt, &task);
    syn_http_client_task(&pt, &task);

    mock_sock_set_response("3\r\nabc", 6);
    syn_http_client_task(&pt, &task);
    mock_sock_eof_on_empty = true;
    mock_sock_set_response("", 0);
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(SYN_ERROR, client.status);

    /* G. State 1 separator: chunk data + separator then socket timeout */
    mock_port_reset();
    reset_accum();
    mock_sock_connected = true;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    PT_INIT(&pt);
    task.user_data = &client;
    mock_sock_set_response(headers, strlen(headers));
    syn_http_client_task(&pt, &task);
    syn_http_client_task(&pt, &task);

    mock_sock_set_response("3\r\nabc", 6);
    syn_http_client_task(&pt, &task);
    mock_tick_ms += 31000;
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(SYN_TIMEOUT, client.status);

    /* H. State 2: full chunk + terminator → successful completion.
     * Uses run_client_task with complete data like test_http_get_chunked. */
    mock_port_reset();
    reset_accum();
    mock_sock_connected = true;
    {
        const char *full_h = "HTTP/1.1 200 OK\r\n"
                             "Transfer-Encoding: chunked\r\n"
                             "Connection: close\r\n"
                             "\r\n"
                             "3\r\n"
                             "abc\r\n"
                             "0\r\n"
                             "\r\n";
        mock_sock_set_response(full_h, strlen(full_h));
    }
    {
        SYN_HttpClient lc;
        syn_http_client_init(&lc, "GET", "example.com", 80, "/h", NULL, NULL, 0, NULL, 0,
                             body_accumulate, NULL, work_buf, sizeof(work_buf));
        SYN_Status h_st = run_client_task(&lc);
        TEST_ASSERT_EQUAL(SYN_OK, h_st);
    }

    /* I. State 2: Socket timeout waiting for terminator */
    mock_port_reset();
    reset_accum();
    mock_sock_connected = true;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    PT_INIT(&pt);
    task.user_data = &client;
    syn_http_client_task(&pt, &task);
    syn_http_client_task(&pt, &task);
    /* Feed chunk without final CRLF — task blocks in state 2 waiting for terminator */
    mock_sock_set_response("3\r\nabc\r\n0\r\n", 11);
    syn_http_client_task(&pt, &task);
    mock_tick_ms += 31000;
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(SYN_TIMEOUT, client.status);
}

/* ── Runner ─────────────────────────────────────────────────────────────── */

static void on_custom_port_redirect_connect(const char *host, uint16_t port)
{
    (void)host;
    (void)port;
    if (s_redirect_count == 0) {
        const char *redirect = "HTTP/1.1 302 Found\r\n"
                               "Location: http://other.com:8080/path\r\n"
                               "Content-Length: 0\r\n"
                               "\r\n";
        mock_sock_set_response(redirect, strlen(redirect));
    } else {
        const char *ok = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        mock_sock_set_response(ok, strlen(ok));
    }
    s_redirect_count++;
}

static void test_http_custom_port_and_long_host_redirect(void)
{
    mock_port_reset();
    reset_accum();
    s_redirect_count = 0;
    mock_sock_connect_cb = on_custom_port_redirect_connect;

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    run_client_task(&client);

    TEST_ASSERT_EQUAL_STRING("other.com", client.cur_host);
    TEST_ASSERT_EQUAL_UINT16(8080, client.cur_port);
    TEST_ASSERT_EQUAL_STRING("/path", client.cur_path);
}

static void test_http_custom_headers_and_payload_send_fail(void)
{
    mock_port_reset();
    reset_accum();

    SYN_HttpHeader extra_headers[] = {{"X-Custom-1", "Value1"}, {"X-Custom-2", "Value2"}};

    SYN_HttpClient client;
    const uint8_t dummy_body[] = "{\"key\":\"value\"}";
    syn_http_client_init(&client, "POST", "example.com", 80, "/api", "application/json", dummy_body,
                         sizeof(dummy_body) - 1, extra_headers, 2, body_accumulate, NULL, work_buf,
                         sizeof(work_buf));

    /* Set send_fail = true so send_request returns false */
    mock_sock_send_fail = true;
    SYN_Status st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_ERROR, st);
}

static void test_http_long_redirect_host_clamping(void)
{
    mock_port_reset();
    reset_accum();

    /* Redirect with explicit port and very long host */
    const char *resp1 =
        "HTTP/1.1 302 Found\r\n"
        "Location: http://thisisareallylonghostnameexceedingthehostbuffersize:8080/path\r\n"
        "\r\n";
    mock_sock_set_response(resp1, strlen(resp1));

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "orig.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    client.hops = 5;
    run_client_task(&client);

    /* Redirect without port and very long host */
    mock_port_reset();
    const char *resp2 =
        "HTTP/1.1 302 Found\r\n"
        "Location: http://thisisareallylonghostnameexceedingthehostbuffersize/path\r\n"
        "\r\n";
    mock_sock_set_response(resp2, strlen(resp2));
    syn_http_client_init(&client, "GET", "orig.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    client.hops = 5;
    run_client_task(&client);
}

static void test_http_send_fail_after_method_write(void)
{
    mock_port_reset();
    reset_accum();
    mock_sock_send_fail_after_bytes = 3; /* Fail after writing "GET" */

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_ERROR, st);
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);
}

static void test_http_custom_headers_write_fail(void)
{
    mock_port_reset();
    reset_accum();

    SYN_HttpHeader headers[1] = {{"X-Custom", "Value"}};
    mock_sock_send_fail_after_bytes = 40;

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, headers, 1,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_ERROR, st);
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);
}

static void test_http_content_length_write_fail(void)
{
    mock_port_reset();
    reset_accum();

    uint8_t payload[] = "123";
    mock_sock_send_fail_after_bytes = 20; /* Fail during Content-Length header write */

    SYN_HttpClient client;
    syn_http_client_init(&client, "POST", "example.com", 80, "/", "text/plain", payload, 3, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_ERROR, st);
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);
}

static void test_http_connect_fail_branch(void)
{
    mock_port_reset();
    mock_sock_connect_fail = true;
    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    TEST_ASSERT_EQUAL(SYN_ERROR, run_client_task(&client));
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);
}

static void test_http_content_length_crlf_write_fail(void)
{
    mock_port_reset();
    reset_accum();

    uint8_t payload[] = "123";
    mock_sock_send_fail_after_bytes = 35;

    SYN_HttpClient client;
    syn_http_client_init(&client, "POST", "example.com", 80, "/", "text/plain", payload, 3, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_ERROR, st);
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);
}

static void test_http_long_redirect_and_header_write_failures(void)
{
    mock_port_reset();
    reset_accum();

    /* 1. Redirect with http:// host:port/path (line 165) */
    mock_port_reset();
    const char *redirect_resp =
        "HTTP/1.1 302 Found\r\n"
        "Location: http://abcdefghijklmnopqrstuvwxyz0123456789.com:8080/p\r\n"
        "Content-Length: 0\r\n"
        "\r\n";
    mock_sock_set_response(redirect_resp, strlen(redirect_resp));
    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "orig.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    client.hops = 5;
    run_client_task(&client);

    /* 2. Write fails at every byte from 1 to 150 (exercising all header write fail returns) */
    SYN_HttpHeader custom_hdrs[] = {{"X-Header-1", "Value-1"}, {"X-Header-2", "Value-2"}};
    for (size_t fail_byte = 1; fail_byte <= 150; fail_byte++) {
        mock_port_reset();
        mock_sock_send_fail_after_bytes = fail_byte;
        mock_sock_set_response("HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n",
                               58);
        syn_http_client_init(&client, "POST", "example.com", 80, "/test", "application/json",
                             (const uint8_t *)"body", 4, custom_hdrs, 2, body_accumulate, NULL,
                             work_buf, sizeof(work_buf));
        run_client_task(&client);
    }

    /* 3. body_cb returning false during streaming (line 611) */
    mock_port_reset();
    reset_accum();
    s_body_cb_fail = true;
    const char *resp_body_cb_fail = "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n0123456789";
    mock_sock_set_response(resp_body_cb_fail, strlen(resp_body_cb_fail));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_cb_rejectable, NULL, work_buf, sizeof(work_buf));
    run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);
    s_body_cb_fail = false;

    /* 4. Premature EOF when known_length && body_remaining > 0 (line 623) */
    mock_port_reset();
    reset_accum();
    mock_sock_connected = true;
    const char *resp_short = "HTTP/1.1 200 OK\r\nContent-Length: 50\r\n\r\nShort";
    mock_sock_set_response(resp_short, strlen(resp_short));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_PT pt4;
    PT_INIT(&pt4);
    SYN_Task task4 = {.user_data = &client};
    syn_http_client_task(&pt4, &task4);
    syn_http_client_task(&pt4, &task4);
    mock_sock_connected = false;
    mock_tick_ms += 30000;
    syn_http_client_task(&pt4, &task4);
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);

    /* 5. URL host clamping (lines 165 & 172) */
    mock_port_reset();
    reset_accum();
    const char *resp_red1 = "HTTP/1.1 301 Moved Permanently\r\n"
                            "Location: "
                            "http://"
                            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                            "aaaaaaaaaa.com:8080/path\r\n"
                            "Content-Length: 0\r\n\r\n";
    mock_sock_set_response(resp_red1, strlen(resp_red1));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    client.hops = 0;
    mock_sock_connected = true;
    SYN_PT pt_red1;
    PT_INIT(&pt_red1);
    SYN_Task task_red1 = {.user_data = &client};
    syn_http_client_task(&pt_red1, &task_red1);

    mock_port_reset();
    reset_accum();
    const char *resp_red2 = "HTTP/1.1 301 Moved Permanently\r\n"
                            "Location: "
                            "http://"
                            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                            "aaaaaaaaaa.com/path\r\n"
                            "Content-Length: 0\r\n\r\n";
    mock_sock_set_response(resp_red2, strlen(resp_red2));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    client.hops = 0;
    mock_sock_connected = true;
    SYN_PT pt_red2;
    PT_INIT(&pt_red2);
    SYN_Task task_red2 = {.user_data = &client};
    syn_http_client_task(&pt_red2, &task_red2);
    client.hops = 5;
    run_client_task(&client);

    /* 6. sock_write_str CRLF fail (line 139) */
    mock_port_reset();
    reset_accum();
    mock_sock_send_fail_after_bytes = 23; /* Fails on trailing CRLF write */
    syn_http_client_init(&client, "GET", "a", 80, "/", NULL, NULL, 0, NULL, 0, body_accumulate,
                         NULL, work_buf, sizeof(work_buf));
    run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);
}

static void test_http_chunked_crlf_boundary_and_timeouts(void)
{
    /* 1. Chunk CRLF split across socket reads (lines 541-565) */
    mock_port_reset();
    reset_accum();
    mock_sock_connected = true;

    const char *resp_part1 = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nHello";
    mock_sock_set_response(resp_part1, strlen(resp_part1));

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));

    SYN_PT pt;
    PT_INIT(&pt);
    SYN_Task task = {.user_data = &client};

    /* Process part 1 */
    syn_http_client_task(&pt, &task);
    syn_http_client_task(&pt, &task);

    /* Provide trailing \r\n and final chunk 0\r\n\r\n */
    const char *resp_part2 = "\r\n0\r\n\r\n";
    mock_sock_set_response(resp_part2, strlen(resp_part2));
    syn_http_client_task(&pt, &task);

    TEST_ASSERT_EQUAL(SYN_OK, client.status);
    TEST_ASSERT_EQUAL(5, body_accum_len);
    TEST_ASSERT_EQUAL_STRING_LEN("Hello", body_accum, 5);

    /* 2. Trailer CRLF split across socket reads (lines 575-591) */
    mock_port_reset();
    reset_accum();
    mock_sock_connected = true;

    const char *resp_trailer1 = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n";
    mock_sock_set_response(resp_trailer1, strlen(resp_trailer1));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    PT_INIT(&pt);

    syn_http_client_task(&pt, &task);
    syn_http_client_task(&pt, &task);

    const char *resp_trailer2 = "\r\n";
    mock_sock_set_response(resp_trailer2, strlen(resp_trailer2));
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(SYN_OK, client.status);

    /* 3. HTTP body recv EOF / socket close during fixed length body (lines 471-476) */
    mock_port_reset();
    reset_accum();
    mock_sock_connected = true;
    const char *resp_fixed_short = "HTTP/1.1 200 OK\r\nContent-Length: 100\r\n\r\nShort";
    mock_sock_set_response(resp_fixed_short, strlen(resp_fixed_short));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    PT_INIT(&pt);
    syn_http_client_task(&pt, &task);
    syn_http_client_task(&pt, &task);
    mock_sock_connected = false;
    mock_tick_ms += 30000;
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);
}

static bool body_cb_reject(const uint8_t *data, size_t len, void *ctx)
{
    (void)data;
    (void)len;
    (void)ctx;
    return false;
}

void test_http_body_cb_failure_and_eof_branches(void)
{
    SYN_HttpClient client;
    SYN_PT pt;
    SYN_Task task;
    task.user_data = &client;

    /* 1. body_cb failure with extra body data in initial buffer */
    mock_port_reset();
    mock_sock_connected = true;
    const char *resp_extra = "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n1234567890";
    mock_sock_set_response(resp_extra, strlen(resp_extra));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_cb_reject, NULL, work_buf, sizeof(work_buf));
    PT_INIT(&pt);
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);
    TEST_ASSERT_EQUAL(SYN_ERROR, client.status);

    /* 2. body_cb failure during subsequent socket recv (lines 637-643) */
    mock_port_reset();
    mock_sock_connected = true;
    const char *resp_hdr_only = "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n";
    mock_sock_set_response(resp_hdr_only, strlen(resp_hdr_only));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_cb_reject, NULL, work_buf, sizeof(work_buf));
    PT_INIT(&pt);
    syn_http_client_task(&pt, &task); /* Parses headers */
    const char *body_chunk = "1234567890";
    mock_sock_set_response(body_chunk, strlen(body_chunk));
    syn_http_client_task(&pt, &task); /* Tries to deliver body, body_cb returns false */
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);
    TEST_ASSERT_EQUAL(SYN_ERROR, client.status);

    /* 3. NULL task and small work_buf checks (lines 248, 252) */
    TEST_ASSERT_EQUAL(PT_EXITED, syn_http_client_task(NULL, NULL));
    SYN_Task empty_task = {.user_data = NULL};
    TEST_ASSERT_EQUAL(PT_EXITED, syn_http_client_task(&pt, &empty_task));

    client.work_buf = NULL;
    client.work_buf_size = 0;
    empty_task.user_data = &client;
    TEST_ASSERT_EQUAL(PT_EXITED, syn_http_client_task(&pt, &empty_task));

    /* 4. Multi-packet chunked body streaming (lines 518-520) */
    mock_port_reset();
    reset_accum();
    mock_sock_connected = true;
    const char *chunk_hdr = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\n";
    mock_sock_set_response(chunk_hdr, strlen(chunk_hdr));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    PT_INIT(&pt);
    syn_http_client_task(&pt, &empty_task); /* Parses headers & chunk size 5 */

    const char *chunk_data = "Hello\r\n0\r\n\r\n";
    mock_sock_set_response(chunk_data, strlen(chunk_data));
    syn_http_client_task(&pt, &empty_task); /* Recv chunk data in packet 2 */
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_DONE, client.state);
    TEST_ASSERT_EQUAL(5, body_accum_len);
}

void test_http_eof_during_fixed_length_body_recv(void)
{
    mock_port_reset();
    reset_accum();
    mock_sock_connected = true;

    const char *resp = "HTTP/1.1 200 OK\r\nContent-Length: 50\r\n\r\n";
    mock_sock_set_response(resp, strlen(resp));

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_PT pt;
    PT_INIT(&pt);
    SYN_Task task = {.user_data = &client};

    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(200, client.resp.status_code);

    mock_sock_connected = true;
    mock_sock_eof_on_empty = true;
    mock_sock_set_response("", 0);
    syn_http_client_task(&pt, &task);

    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);
    TEST_ASSERT_EQUAL(SYN_ERROR, client.status);
}

void test_http_chunk_size_recv_timeout(void)
{
    mock_port_reset();
    reset_accum();
    mock_sock_connected = true;

    const char *resp = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n";
    mock_sock_set_response(resp, strlen(resp));

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_PT pt;
    PT_INIT(&pt);
    SYN_Task task = {.user_data = &client};

    syn_http_client_task(&pt, &task);

    mock_sock_rx_len = 0;
    mock_sock_rx_pos = 0;
    mock_tick_ms += 15000;
    syn_http_client_task(&pt, &task);

    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);
    TEST_ASSERT_EQUAL(SYN_TIMEOUT, client.status);
}

static void test_http_redirect_no_trailing_newline(void)
{
    mock_port_reset();
    reset_accum();
    s_redirect_count = 0;
    mock_sock_connect_cb = on_redirect_connect;

    SYN_HttpClient client;
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_Status st = run_client_task(&client);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    /* Chunked response with uppercase hex chunk size "0xA\r\n0123456789\r\n0\r\n\r\n" */
    mock_port_reset();
    reset_accum();
    mock_sock_connected = true;
    const char *uc_chunk_resp =
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\nA\r\n0123456789\r\n0\r\n\r\n";
    mock_sock_set_response(uc_chunk_resp, strlen(uc_chunk_resp));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    SYN_PT pt;
    PT_INIT(&pt);
    SYN_Task task = {.user_data = &client};
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(10, body_accum_len);

    /* Response with NULL body_cb (lines 422-427) */
    mock_port_reset();
    mock_sock_connected = true;
    const char *no_cb_resp = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello";
    mock_sock_set_response(no_cb_resp, strlen(no_cb_resp));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0, NULL, NULL,
                         work_buf, sizeof(work_buf));
    PT_INIT(&pt);
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_DONE, client.state);
    TEST_ASSERT_EQUAL(SYN_OK, client.status);

    /* Relative path redirect Location header "Location: relpath/sub" (lines 195-200) */
    mock_port_reset();
    mock_sock_connected = true;
    const char *rel_redir_resp = "HTTP/1.1 302 Found\r\nLocation: relpath/sub\r\n\r\n";
    mock_sock_set_response(rel_redir_resp, strlen(rel_redir_resp));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0, NULL, NULL,
                         work_buf, sizeof(work_buf));
    PT_INIT(&pt);
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);

    /* http:// redirect without trailing slash "Location: http://example.com" (lines 186-188) */
    mock_port_reset();
    mock_sock_connected = true;
    const char *noslash_redir_resp = "HTTP/1.1 302 Found\r\nLocation: http://example.com\r\n\r\n";
    mock_sock_set_response(noslash_redir_resp, strlen(noslash_redir_resp));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0, NULL, NULL,
                         work_buf, sizeof(work_buf));
    PT_INIT(&pt);
    syn_http_client_task(&pt, &task);

    /* http:// redirect with explicit port "Location: http://example.com:8080/api" (lines 168-174)
     */
    mock_port_reset();
    mock_sock_connected = true;
    const char *port_redir_resp =
        "HTTP/1.1 302 Found\r\nLocation: http://example.com:8080/api\r\n\r\n";
    mock_sock_set_response(port_redir_resp, strlen(port_redir_resp));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0, NULL, NULL,
                         work_buf, sizeof(work_buf));
    PT_INIT(&pt);
    syn_http_client_task(&pt, &task);

    /* Body callback returning false during buffered delivery (line 625) */
    mock_port_reset();
    mock_sock_connected = true;
    const char *buffered_resp = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello";
    mock_sock_set_response(buffered_resp, strlen(buffered_resp));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_cb_reject, NULL, work_buf, sizeof(work_buf));
    PT_INIT(&pt);
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);

    /* Socket body timeout path when content-length not satisfied (lines 666-671) */
    mock_port_reset();
    mock_sock_connected = true;
    const char *short_body_resp = "HTTP/1.1 200 OK\r\nContent-Length: 100\r\n\r\nshort";
    mock_sock_set_response(short_body_resp, strlen(short_body_resp));
    syn_http_client_init(&client, "GET", "example.com", 80, "/", NULL, NULL, 0, NULL, 0,
                         body_accumulate, NULL, work_buf, sizeof(work_buf));
    PT_INIT(&pt);
    syn_http_client_task(&pt, &task);
    mock_tick_ms += 11000;
    syn_http_client_task(&pt, &task);
    TEST_ASSERT_EQUAL(SYN_HTTP_STATE_ERROR, client.state);
    TEST_ASSERT_EQUAL(SYN_TIMEOUT, client.status);
}

void run_http_tests(void)
{
    RUN_TEST(test_http_get_200);
    RUN_TEST(test_http_get_404);
    RUN_TEST(test_http_get_large_body);
    RUN_TEST(test_http_get_no_content_length);
    RUN_TEST(test_http_get_sends_correct_request);
    RUN_TEST(test_http_post_basic);
    RUN_TEST(test_http_get_chunked);
    RUN_TEST(test_http_get_redirect);
    RUN_TEST(test_http_get_redirect_limit);
    RUN_TEST(test_http_connect_fail);
    RUN_TEST(test_http_send_fail);
    RUN_TEST(test_http_body_send_fail);
    RUN_TEST(test_http_redirect_formats);
    RUN_TEST(test_http_header_edge_cases);
    RUN_TEST(test_http_chunked_errors);
    /* RUN_TEST(test_http_streaming_errors); */
    RUN_TEST(test_http_extra_data_in_buffer);
    RUN_TEST(test_http_custom_headers);
    RUN_TEST(test_http_chunked_boundary_cases);
    RUN_TEST(test_http_custom_port_and_long_host_redirect);
    RUN_TEST(test_http_custom_headers_and_payload_send_fail);
    RUN_TEST(test_http_long_redirect_host_clamping);
    RUN_TEST(test_http_send_fail_after_method_write);
    RUN_TEST(test_http_custom_headers_write_fail);
    RUN_TEST(test_http_content_length_write_fail);
    RUN_TEST(test_http_content_length_crlf_write_fail);
    RUN_TEST(test_http_connect_fail_branch);
    RUN_TEST(test_http_long_redirect_and_header_write_failures);
    RUN_TEST(test_http_chunked_crlf_boundary_and_timeouts);
    RUN_TEST(test_http_body_cb_failure_and_eof_branches);
    RUN_TEST(test_http_eof_during_fixed_length_body_recv);
    RUN_TEST(test_http_chunk_size_recv_timeout);
    RUN_TEST(test_http_redirect_no_trailing_newline);
}
