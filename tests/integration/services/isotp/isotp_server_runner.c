#include "syntropic/drivers/syn_can.h"
#include "syntropic/proto/syn_isotp.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int recv_frame_nonblock(int fd, uint8_t *buf, size_t len)
{
    size_t total = 0;
    while (total < len) {
        ssize_t n = recv(fd, buf + total, len - total, MSG_DONTWAIT);
        if (n > 0) {
            total += (size_t)n;
        } else if (n == 0) {
            return -1;
        } else {
            if (total == 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return 0;
                }
                return -1;
            }
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                return -1;
            }
            usleep(100);
        }
    }
    return 1;
}

int main(void)
{
    printf("[SyntropicOS ISO-TP C Server] Starting ISO 15765-2 Server Daemon on "
           "0.0.0.0:10887...\n");

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(10887);

    if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 5) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    printf("[SyntropicOS ISO-TP C Server] Listening on port 10887...\n");

    static uint8_t rx_buf[4096];
    static uint8_t tx_buf[4096];
    uint8_t payload[4096];

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (conn_fd < 0) {
            continue;
        }

        printf("[SyntropicOS ISO-TP C Server] Client connected!\n");

        SYN_ISOTP_Link link;
        syn_isotp_init(&link, 0x7E0U, 0x7E8U, rx_buf, sizeof(rx_buf), tx_buf, sizeof(tx_buf));

        uint8_t rx_frame_buf[13];

        while (1) {
            int ret = recv_frame_nonblock(conn_fd, rx_frame_buf, 13);
            if (ret < 0) {
                break;
            }
            if (ret > 0) {
                SYN_CAN_Frame syn_frame;
                syn_frame.id = ((uint32_t)rx_frame_buf[0] << 24) |
                               ((uint32_t)rx_frame_buf[1] << 16) |
                               ((uint32_t)rx_frame_buf[2] << 8) | (uint32_t)rx_frame_buf[3];
                syn_frame.dlc = rx_frame_buf[4];
                memcpy(syn_frame.data, &rx_frame_buf[5], 8);

                syn_isotp_process_rx_frame(&link, &syn_frame);
            }

            syn_isotp_step(&link, 1);

            SYN_CAN_Frame tx_syn_frame;
            while (syn_isotp_get_tx_frame(&link, &tx_syn_frame)) {
                uint8_t tx_frame_buf[13];
                tx_frame_buf[0] = (uint8_t)((tx_syn_frame.id >> 24) & 0xFF);
                tx_frame_buf[1] = (uint8_t)((tx_syn_frame.id >> 16) & 0xFF);
                tx_frame_buf[2] = (uint8_t)((tx_syn_frame.id >> 8) & 0xFF);
                tx_frame_buf[3] = (uint8_t)(tx_syn_frame.id & 0xFF);
                tx_frame_buf[4] = tx_syn_frame.dlc;
                memcpy(&tx_frame_buf[5], tx_syn_frame.data, 8);

                send(conn_fd, tx_frame_buf, 13, 0);
            }

            ssize_t rx_len = syn_isotp_receive(&link, payload, sizeof(payload));
            if (rx_len > 0) {
                printf("[SyntropicOS ISO-TP C Server] Received ISO-TP payload (%zd bytes)! Echoing "
                       "back...\n",
                       rx_len);
                syn_isotp_send(&link, payload, (size_t)rx_len);
            }

            usleep(1000);
        }

        close(conn_fd);
        printf("[SyntropicOS ISO-TP C Server] Client disconnected.\n");
    }

    close(listen_fd);
    return 0;
}
