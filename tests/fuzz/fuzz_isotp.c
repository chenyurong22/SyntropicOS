/**
 * @file fuzz_isotp.c
 * @brief libFuzzer target for syn_isotp ISO 15765-2 CAN transport stack.
 */

#include "mock_port.h"
#include "syntropic/proto/syn_isotp.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 1 || size > 64) {
        return 0;
    }

    mock_port_reset();

    SYN_ISOTP_Link link;
    uint8_t rx_buf[256];
    uint8_t tx_buf[256];

    syn_isotp_init(&link, 0x7E0, 0x7E8, rx_buf, sizeof(rx_buf), tx_buf, sizeof(tx_buf));

    /* Build CAN frame from fuzzer data input */
    SYN_CAN_Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.id = 0x7E0;
    frame.dlc = (uint8_t)((size > 8) ? 8 : size);
    memcpy(frame.data, data, frame.dlc);

    /* Ingest CAN frame into ISO-TP state machine */
    syn_isotp_process_rx_frame(&link, &frame);

    /* Drain any generated TX frame */
    SYN_CAN_Frame tx_frame;
    (void)syn_isotp_get_tx_frame(&link, &tx_frame);

    /* Attempt to receive assembled payload */
    uint8_t out[256];
    (void)syn_isotp_receive(&link, out, sizeof(out));

    /* Step timers */
    syn_isotp_step(&link, (uint32_t)(data[0] & 0x0F));

    return 0;
}
