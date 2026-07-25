/**
 * @file fuzz_cbor.c
 * @brief libFuzzer target for syn_cbor_reader decoder.
 */

#include "syntropic/util/syn_cbor_read.h"
#include <stdint.h>
#include <stddef.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0) return 0;

    SYN_CborReader reader;
    syn_cbor_reader_init(&reader, data, size);

    (void)syn_cbor_read_uint(&reader);
    (void)syn_cbor_read_int(&reader);
    (void)syn_cbor_read_bool(&reader);
    (void)syn_cbor_read_float(&reader);

    uint8_t buf[64];
    (void)syn_cbor_read_text(&reader, (char *)buf, sizeof(buf));
    (void)syn_cbor_read_bytes(&reader, buf, sizeof(buf));

    return 0;
}
