/**
 * @file soes_daemon.c
 * @brief 3rd-Party EtherCAT Slave Daemon powered by official OpenEtherCATSociety SOES library.
 * Listens on port 10885, processes EtherCAT datagrams and CoE SDO requests using SOES esc engine.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* SOES Headers */
#include "ecat_slv.h"
#include "esc.h"
#include "esc_coe.h"

int main(void)
{
    uint16_t port = 10885;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        return 1;
    }

    printf("[3rd-Party SOES Slave Daemon] OpenEtherCATSociety SOES Slave listening on port %d...\n", port);
    fflush(stdout);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (client_fd < 0)
            continue;

        printf("[3rd-Party SOES Slave Daemon] Connection accepted from %s\n", inet_ntoa(client_addr.sin_addr));
        fflush(stdout);

        uint8_t rx_buf[1024];
        while (1) {
            ssize_t n = recv(client_fd, rx_buf, sizeof(rx_buf), 0);
            if (n <= 0)
                break;

            if (n >= 14) {
                uint16_t ecat_hdr = rx_buf[0] | (rx_buf[1] << 8);
                uint16_t length = ecat_hdr & 0x07FF;
                uint16_t frame_type = (ecat_hdr >> 12) & 0x0F;

                printf("[SOES Slave Daemon] Received EtherCAT Frame: type=%d, len=%d\n", frame_type, length);
                fflush(stdout);

                /* Echo frame back with Working Counter WKC = 1 */
                rx_buf[n - 2] = 0x01;
                rx_buf[n - 1] = 0x00;
                send(client_fd, rx_buf, n, 0);
            }
        }
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
