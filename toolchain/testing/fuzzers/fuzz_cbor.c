/**
 * @file fuzz_cbor.c
 * @brief libFuzzer target for syn_cbor decoder.
 */

#include "syntropic/util/syn_cbor_read.h"
#include <stdint.h>
#include <stddef.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0) return 0;

    SYN_CborReader reader;
    syn_cbor_reader_init(&reader, data, size);

    uint64_t u_val;
    int64_t i_val;
    bool b_val;
    float f_val;
    uint8_t buf[64];
    size_t out_len = 0;

    /* Attempt to decode various CBOR types */
    syn_cbor_read_uint(&reader, &u_val);
    syn_cbor_read_int(&reader, &i_val);
    syn_cbor_read_bool(&reader, &b_val);
    syn_cbor_read_float(&reader, &f_val);
    syn_cbor_read_text(&reader, (char *)buf, sizeof(buf), &out_len);
    syn_cbor_read_bytes(&reader, buf, sizeof(buf), &out_len);

    return 0;
}
