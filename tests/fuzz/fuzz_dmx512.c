#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "syntropic/proto/syn_dmx512.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > 4096) return 0;

    SYN_DMX512_Slave slave;
    uint16_t start_addr = (data[0] % 512) + 1;
    uint16_t footprint  = (data[0] % 64) + 1;
    syn_dmx512_slave_init(&slave, start_addr, footprint);

    for (size_t i = 1; i < size; i++) {
        uint8_t byte = data[i];
        if (byte == 0xFF) {
            syn_dmx512_slave_rx_break(&slave);
        } else {
            syn_dmx512_slave_rx_byte(&slave, byte);
        }
        if (syn_dmx512_slave_is_updated(&slave)) {
            syn_dmx512_slave_get_channel(&slave, 0);
        }
    }

    SYN_DMX512_Master master;
    uint16_t num_ch = (data[0] % 512) + 1;
    syn_dmx512_master_init(&master, num_ch);
    syn_dmx512_master_set_channel(&master, 1, data[0]);
    syn_dmx512_master_get_channel(&master, 1);

    uint8_t out_buf[513];
    syn_dmx512_master_build_frame(&master, out_buf, sizeof(out_buf));

    return 0;
}
