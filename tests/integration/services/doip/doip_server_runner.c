#include "syntropic/proto/syn_doip.h"
#include "syntropic/proto/syn_uds.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

static uint8_t g_vin[17] = "SYN12345678901234";
static uint8_t g_sys_status[4] = {0xA5, 0x00, 0x00, 0x00};
static uint8_t g_prog_did[2] = {0xAA, 0xBB};

static uint8_t g_memory_store[256] = {0xDE, 0xAD, 0xBE, 0xEF};

static bool memory_handler_cb(bool is_write, uint32_t address, uint32_t size, uint8_t *data,
                              void *user_data)
{
    (void)user_data;
    if (address + size > sizeof(g_memory_store)) {
        return false;
    }
    if (is_write) {
        memcpy(&g_memory_store[address], data, size);
    } else {
        memcpy(data, &g_memory_store[address], size);
    }
    return true;
}

static bool comm_control_cb(SYN_UDS_CommControlType ctrl_type, uint8_t comm_type, void *user_data)
{
    (void)ctrl_type;
    (void)comm_type;
    (void)user_data;
    return true;
}

int main(void)
{
    printf("[SyntropicOS DoIP C Server] Starting ISO 13400-2 DoIP Server Daemon on "
           "0.0.0.0:13400...\n");

    SYN_DoIP_Server doip;
    syn_doip_init(&doip, 0x1000); /* ECU Logical Address 0x1000 */

    uint8_t vin[17] = "SYN12345678901234";
    uint8_t eid[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    uint8_t gid[6] = {0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    syn_doip_set_identifiers(&doip, vin, eid, gid);

    SYN_UDS_Server uds;
    if (!syn_uds_init(&uds)) {
        fprintf(stderr, "[SyntropicOS DoIP C Server] Failed to initialize UDS server!\n");
        return 1;
    }

    /* Register standard automotive DIDs */
    syn_uds_register_did(&uds, 0xF190U, g_vin, sizeof(g_vin), false);
    syn_uds_register_did(&uds, 0x0100U, g_sys_status, 1, true);
    syn_uds_register_did_ext(&uds, 0x0300U, g_prog_did, sizeof(g_prog_did), false,
                             SYN_UDS_SESSION_MASK_PROGRAMMING, SYN_UDS_SECURITY_MASK_ALL);

    /* Register DTC 0x012345 */
    syn_uds_register_dtc(&uds, 0x012345U, 0x2FU, 0x09U);

    /* Register Handlers */
    syn_uds_register_memory_handler(&uds, memory_handler_cb, NULL);
    syn_uds_register_comm_control(&uds, comm_control_cb, NULL);

    int tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_fd < 0) {
        perror("socket tcp");
        return 1;
    }
    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0) {
        perror("socket udp");
        return 1;
    }

    int opt = 1;
    setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(13400);

    if (bind(tcp_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind tcp");
        return 1;
    }
    if (bind(udp_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind udp");
        return 1;
    }
    if (listen(tcp_fd, 5) < 0) {
        perror("listen tcp");
        return 1;
    }

    printf("[SyntropicOS DoIP C Server] Listening on TCP/UDP port 13400...\n");

    int active_conn_fd = -1;
    uint8_t rx_buf[4096];
    uint8_t tx_buf[4096];

    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(tcp_fd, &read_fds);
        FD_SET(udp_fd, &read_fds);
        int max_fd = (tcp_fd > udp_fd) ? tcp_fd : udp_fd;

        if (active_conn_fd >= 0) {
            FD_SET(active_conn_fd, &read_fds);
            if (active_conn_fd > max_fd) {
                max_fd = active_conn_fd;
            }
        }

        struct timeval tv = {0, 100000}; /* 100 ms timeout */
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
        if (activity < 0 && errno != EINTR) {
            continue;
        }

        /* 1. Handle incoming UDP DoIP Requests (e.g. Vehicle ID) */
        if (FD_ISSET(udp_fd, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            ssize_t n = recvfrom(udp_fd, rx_buf, sizeof(rx_buf), 0, (struct sockaddr *)&client_addr,
                                 &addr_len);
            if (n > 0) {
                uint16_t tx_len = 0;
                if (syn_doip_process_msg(&doip, &uds, rx_buf, (uint16_t)n, tx_buf, sizeof(tx_buf),
                                         &tx_len) &&
                    tx_len > 0) {
                    sendto(udp_fd, tx_buf, tx_len, 0, (struct sockaddr *)&client_addr, addr_len);
                }
            }
        }

        /* 2. Handle new TCP DoIP Client Connections */
        if (FD_ISSET(tcp_fd, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int new_fd = accept(tcp_fd, (struct sockaddr *)&client_addr, &addr_len);
            if (new_fd >= 0) {
                if (active_conn_fd >= 0) {
                    close(active_conn_fd);
                }
                active_conn_fd = new_fd;
                syn_doip_init(&doip, 0x1000);
                syn_doip_set_identifiers(&doip, vin, eid, gid);
            }
        }

        /* 3. Handle incoming TCP DoIP Messages (Routing Activation & UDS Messages) */
        if (active_conn_fd >= 0 && FD_ISSET(active_conn_fd, &read_fds)) {
            ssize_t n = recv(active_conn_fd, rx_buf, sizeof(rx_buf), 0);
            if (n <= 0) {
                close(active_conn_fd);
                active_conn_fd = -1;
            } else {
                uint16_t tx_len = 0;
                if (syn_doip_process_msg(&doip, &uds, rx_buf, (uint16_t)n, tx_buf, sizeof(tx_buf),
                                         &tx_len) &&
                    tx_len > 0) {
                    send(active_conn_fd, tx_buf, tx_len, 0);
                }
            }
        }
    }

    close(tcp_fd);
    close(udp_fd);
    return 0;
}
