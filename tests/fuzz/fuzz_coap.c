/**
 * @file fuzz_coap.c
 * @brief libFuzzer target for syn_coap message parser.
 */

#include "syntropic/net/syn_coap.h"

#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0)
        return 0;

    SYN_CoapMsg msg;
    SYN_CoapOption options[8];
    size_t option_count = 0;

    syn_coap_parse(&msg, options, 8, &option_count, data, size);

    return 0;
}
