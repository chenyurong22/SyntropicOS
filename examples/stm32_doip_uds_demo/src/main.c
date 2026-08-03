/**
 * @file main.c
 * @brief SyntropicOS ISO 13400-2 (DoIP) & ISO 14229-1 (UDS) STM32 Example.
 *
 * Demonstrates zero-malloc DoIP transport engine receiving UDP discovery / vehicle identification
 * and TCP diagnostic requests on port 13400, routing UDS requests directly to syn_uds server.
 */

#include "syntropic/proto/syn_doip.h"
#include "syntropic/proto/syn_uds.h"
#include "syntropic/syntropic.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Static DoIP and UDS Instances */
static SYN_DoIP_Server g_doip_server;
static SYN_UDS_Server g_uds_server;

/* Identification data */
static uint8_t g_vin[17] = "SYNTROPICOS123456";
static uint8_t g_eid[6]  = {0x00, 0x80, 0xE1, 0x01, 0x02, 0x03};
static uint8_t g_gid[6]  = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void doip_app_init(void)
{
    /* Initialize UDS diagnostic server (ECU) */
    syn_uds_init(&g_uds_server);
    syn_uds_register_did(&g_uds_server, 0xF190, g_vin, 17, false);

    /* Initialize DoIP server (Logical Address 0x1001) */
    syn_doip_init(&g_doip_server, 0x1001);
    syn_doip_set_identifiers(&g_doip_server, g_vin, g_eid, g_gid);
}

/**
 * @brief Process incoming TCP/UDP network packet received on port 13400.
 *
 * @param rx_data Raw network packet.
 * @param rx_len Packet byte count.
 * @param tx_buf Output buffer for response frame.
 * @param max_tx Capacity of tx_buf.
 * @param tx_len Pointer to store response byte length.
 */
void doip_on_network_packet(const uint8_t *rx_data, uint16_t rx_len,
                            uint8_t *tx_buf, uint16_t max_tx, uint16_t *tx_len)
{
    syn_doip_process_msg(&g_doip_server, &g_uds_server, rx_data, rx_len, tx_buf, max_tx, tx_len);
}

int main(void)
{
    doip_app_init();
    for (;;) {
        /* Servicing Ethernet TCP/UDP 13400 network events */
    }
    return 0;
}
