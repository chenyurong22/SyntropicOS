/**
 * @file main.c
 * @brief SyntropicOS Dual Modbus TCP Master (Client) & Slave (Server) STM32 HAL Example.
 *
 * Demonstrates simultaneous Modbus TCP Server (port 502 listener for incoming SCADA queries)
 * and Modbus TCP Client (master polling external power meters and sensors) running in a single
 * STM32 firmware binary using non-blocking socket wrappers (`syn_port_sock_*`) and standard C99.
 */

#include "syntropic/syntropic.h"
#include "stm32f4xx_hal.h" /* Replace with target header (stm32f1xx_hal.h / stm32f7xx_hal.h) */

/* Modbus TCP Constants */
#define MODBUS_TCP_PORT 502
#define MBAP_HEADER_LEN 7

/* Server (Slave) Local Register Maps */
static uint16_t server_holding_regs[32] = {0};
static uint16_t server_input_regs[16] = {0};

/* Client (Master) Data Buffer for Polled Slaves */
static uint16_t polled_sensor_regs[8] = {0};

/* Sockets */
static SYN_Socket server_listener_sock = SYN_SOCKET_INVALID;
static SYN_Socket server_client_sock = SYN_SOCKET_INVALID;
static SYN_Socket client_master_sock = SYN_SOCKET_INVALID;

/* Master Query Transaction Counter */
static uint16_t transaction_id_counter = 1;

/**
 * @brief MBAP (Modbus Application Protocol) Header Structure.
 */
typedef struct {
    uint16_t transaction_id; /* Transaction Identifier */
    uint16_t protocol_id;    /* Protocol Identifier (0x0000 = Modbus TCP) */
    uint16_t length;         /* Length of following bytes (Unit ID + PDU) */
    uint8_t  unit_id;        /* Unit Identifier / Slave Address */
} MBAP_Header;

/**
 * @brief Encode 7-byte MBAP Header.
 */
static void mbap_encode(uint8_t *buf, const MBAP_Header *hdr)
{
    buf[0] = (uint8_t)(hdr->transaction_id >> 8);
    buf[1] = (uint8_t)(hdr->transaction_id & 0xFF);
    buf[2] = (uint8_t)(hdr->protocol_id >> 8);
    buf[3] = (uint8_t)(hdr->protocol_id & 0xFF);
    buf[4] = (uint8_t)(hdr->length >> 8);
    buf[5] = (uint8_t)(hdr->length & 0xFF);
    buf[6] = hdr->unit_id;
}

/**
 * @brief Decode 7-byte MBAP Header.
 */
static bool mbap_decode(const uint8_t *buf, MBAP_Header *hdr)
{
    hdr->transaction_id = ((uint16_t)buf[0] << 8) | buf[1];
    hdr->protocol_id    = ((uint16_t)buf[2] << 8) | buf[3];
    hdr->length         = ((uint16_t)buf[4] << 8) | buf[5];
    hdr->unit_id        = buf[6];
    return (hdr->protocol_id == 0); /* 0 = Modbus TCP */
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* MODBUS TCP SERVER (SLAVE) IMPLEMENTATION                                   */
/* ─────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Initialize Modbus TCP Server socket listening on port 502.
 */
void modbus_tcp_server_init(void)
{
    server_holding_regs[0] = 100; /* System Status */
    server_holding_regs[1] = 250; /* Target Setpoint */

    server_listener_sock = syn_port_sock_listen(MODBUS_TCP_PORT, 2);
}

/**
 * @brief Process incoming request on Server socket and format Modbus TCP response.
 */
static void modbus_tcp_server_process_pdu(const MBAP_Header *req_hdr, const uint8_t *pdu, size_t pdu_len)
{
    if (pdu_len < 5) return;

    uint8_t func_code = pdu[0];
    uint16_t start_addr = ((uint16_t)pdu[1] << 8) | pdu[2];
    uint16_t count      = ((uint16_t)pdu[3] << 8) | pdu[4];

    uint8_t tx_buf[260];
    size_t resp_pdu_len = 0;

    if (func_code == SYN_MB_FC_READ_HOLDING) {
        /* FC 0x03 Read Holding Registers */
        if (start_addr + count <= 32) {
            tx_buf[MBAP_HEADER_LEN] = SYN_MB_FC_READ_HOLDING;
            tx_buf[MBAP_HEADER_LEN + 1] = (uint8_t)(count * 2);
            resp_pdu_len = 2 + (count * 2);

            for (uint16_t i = 0; i < count; i++) {
                uint16_t val = server_holding_regs[start_addr + i];
                tx_buf[MBAP_HEADER_LEN + 2 + (i * 2)]     = (uint8_t)(val >> 8);
                tx_buf[MBAP_HEADER_LEN + 2 + (i * 2) + 1] = (uint8_t)(val & 0xFF);
            }
        }
    } else if (func_code == SYN_MB_FC_WRITE_SINGLE) {
        /* FC 0x06 Write Single Register */
        if (start_addr < 32) {
            server_holding_regs[start_addr] = count; /* Value to write */

            /* Echo back request PDU */
            for (size_t i = 0; i < pdu_len; i++) {
                tx_buf[MBAP_HEADER_LEN + i] = pdu[i];
            }
            resp_pdu_len = pdu_len;
        }
    }

    if (resp_pdu_len > 0) {
        MBAP_Header resp_hdr = {
            .transaction_id = req_hdr->transaction_id,
            .protocol_id = 0,
            .length = (uint16_t)(1 + resp_pdu_len),
            .unit_id = req_hdr->unit_id
        };

        mbap_encode(tx_buf, &resp_hdr);
        syn_port_sock_send_all(server_client_sock, tx_buf, MBAP_HEADER_LEN + resp_pdu_len);
    }
}

/**
 * @brief Server task polling client connections and requests.
 */
void modbus_tcp_server_task(void)
{
    if (server_listener_sock == SYN_SOCKET_INVALID) return;

    /* Accept incoming connection if idle */
    if (server_client_sock == SYN_SOCKET_INVALID) {
        server_client_sock = syn_port_sock_accept(server_listener_sock, 10);
    }

    if (server_client_sock != SYN_SOCKET_INVALID) {
        uint8_t rx_buf[260];
        int bytes = syn_port_sock_recv(server_client_sock, rx_buf, sizeof(rx_buf), 10);
        if (bytes >= MBAP_HEADER_LEN) {
            MBAP_Header req_hdr;
            if (mbap_decode(rx_buf, &req_hdr)) {
                modbus_tcp_server_process_pdu(&req_hdr, &rx_buf[MBAP_HEADER_LEN], (size_t)(bytes - MBAP_HEADER_LEN));
            }
        } else if (bytes == 0) {
            /* Client disconnected */
            syn_port_sock_close(server_client_sock);
            server_client_sock = SYN_SOCKET_INVALID;
        }
    }
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* MODBUS TCP CLIENT (MASTER) IMPLEMENTATION                                  */
/* ─────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Issue a Modbus TCP Read Holding Registers query to remote field device IP.
 */
bool modbus_tcp_client_read_remote_holding(const char *target_ip, uint16_t reg_addr, uint16_t reg_count)
{
    if (client_master_sock == SYN_SOCKET_INVALID) {
        client_master_sock = syn_port_sock_connect_host(target_ip, MODBUS_TCP_PORT);
    }

    if (client_master_sock == SYN_SOCKET_INVALID) return false;

    uint8_t tx_buf[MBAP_HEADER_LEN + 5];
    MBAP_Header hdr = {
        .transaction_id = transaction_id_counter++,
        .protocol_id = 0,
        .length = 6, /* Unit ID (1) + PDU (5) */
        .unit_id = 1
    };

    mbap_encode(tx_buf, &hdr);
    tx_buf[MBAP_HEADER_LEN]     = SYN_MB_FC_READ_HOLDING;
    tx_buf[MBAP_HEADER_LEN + 1] = (uint8_t)(reg_addr >> 8);
    tx_buf[MBAP_HEADER_LEN + 2] = (uint8_t)(reg_addr & 0xFF);
    tx_buf[MBAP_HEADER_LEN + 3] = (uint8_t)(reg_count >> 8);
    tx_buf[MBAP_HEADER_LEN + 4] = (uint8_t)(reg_count & 0xFF);

    if (syn_port_sock_send_all(client_master_sock, tx_buf, sizeof(tx_buf)) > 0) {
        uint8_t rx_buf[260];
        int bytes = syn_port_sock_recv(client_master_sock, rx_buf, sizeof(rx_buf), 200);
        if (bytes >= MBAP_HEADER_LEN + 2) {
            MBAP_Header resp_hdr;
            if (mbap_decode(rx_buf, &resp_hdr) && rx_buf[MBAP_HEADER_LEN] == SYN_MB_FC_READ_HOLDING) {
                uint8_t byte_count = rx_buf[MBAP_HEADER_LEN + 1];
                uint16_t read_cnt = byte_count / 2;

                for (uint16_t i = 0; i < read_cnt && i < 8; i++) {
                    polled_sensor_regs[i] = ((uint16_t)rx_buf[MBAP_HEADER_LEN + 2 + (i * 2)] << 8) |
                                             rx_buf[MBAP_HEADER_LEN + 2 + (i * 2) + 1];
                }
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Periodic 1000ms Client polling task.
 */
void modbus_tcp_client_task_1000ms(void)
{
    /* Query external Modbus TCP Field Device at 192.168.1.50 */
    modbus_tcp_client_read_remote_holding("192.168.1.50", 0, 4);
}

/**
 * @brief Main application entry point.
 */
void app_init(void)
{
    modbus_tcp_server_init();
}
