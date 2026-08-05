/**
 * @file main.c
 * @brief ISO 17987-2 LIN Transport Protocol (LIN TP / UDSonLIN ISO 14229-7) Demo.
 */

#include "syntropic/proto/syn_lintp.h"
#include "syntropic/proto/syn_uds.h"
#include <stdio.h>
#include <string.h>

static uint8_t g_rx_buf[256];
static uint8_t g_tx_buf[256];
static uint8_t g_vin_data[17] = "SYN17987LINTP001";
static SYN_LINTP_Link g_lintp;
static SYN_UDS_Server g_uds;

int main(void)
{
    printf("=== SyntropicOS ISO 17987-2 LIN TP (UDSonLIN) Demo ===\n");

    /* Initialize LIN TP link for Slave NAD 0x02 */
    syn_lintp_init(&g_lintp, 0x02, g_rx_buf, sizeof(g_rx_buf), g_tx_buf, sizeof(g_tx_buf));

    /* Initialize UDS diagnostic server and register VIN Data Identifier (0xF190) */
    syn_uds_init(&g_uds);
    syn_uds_register_did(&g_uds, 0xF190, g_vin_data, (uint16_t)strlen((char *)g_vin_data), false);

    /* Simulate incoming UDS ReadDataByIdentifier (0x22 0xF1 0x90) request from Master (NAD 0x02) */
    uint8_t master_req_frame[8] = {0x02, 0x03, 0x22, 0xF1, 0x90, 0xFF, 0xFF, 0xFF};
    printf("[Master -> Bus] Sending LIN Master Request (ID 0x3C): SF NAD=0x02 UDS Read VIN (0x22 0xF1 0x90)\n");
    syn_lintp_process_rx_frame(&g_lintp, master_req_frame);

    /* Drain & process received UDS diagnostic message */
    uint8_t uds_req[128];
    ssize_t req_len = syn_lintp_receive(&g_lintp, uds_req, sizeof(uds_req));
    if (req_len > 0) {
        printf("[Slave TP] Reassembled UDS Request (%zd bytes): ", req_len);
        for (ssize_t i = 0; i < req_len; i++) {
            printf("%02X ", uds_req[i]);
        }
        printf("\n");

        /* Execute UDS Server processing */
        uint8_t uds_resp[128];
        uint16_t resp_len = 0;
        if (syn_uds_process_request(&g_uds, uds_req, (uint16_t)req_len,
                                    uds_resp, sizeof(uds_resp), &resp_len,
                                    SYN_UDS_ADDR_PHYSICAL)) {
            printf("[Slave UDS] Generated Response (%u bytes): ", resp_len);
            for (uint16_t i = 0; i < resp_len; i++) {
                printf("%02X ", uds_resp[i]);
            }
            printf("\n");

            /* Queue response for LIN TP Transmission on Slave Response ID (0x3D) */
            syn_lintp_send(&g_lintp, 0x02, uds_resp, resp_len);

            /* Drain segmented LIN TP frames (FF & CF) */
            uint8_t tx_frame[8];
            int frame_idx = 1;
            while (syn_lintp_get_tx_frame(&g_lintp, tx_frame)) {
                printf("[Slave -> Bus] LIN Response Frame #%d (ID 0x3D): ", frame_idx++);
                for (int b = 0; b < 8; b++) {
                    printf("%02X ", tx_frame[b]);
                }
                printf("\n");
            }
        }
    }

    printf("=== LIN TP Demo Finished Successfully ===\n");
    return 0;
}
