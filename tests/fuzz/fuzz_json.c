/**
 * @file fuzz_json.c
 * @brief libFuzzer target for syn_json reader.
 */

#include "syntropic/util/syn_json_read.h"

#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0)
        return 0;

    SYN_JsonReader reader;
    syn_json_reader_init(&reader, (const char *)data, size);

    int32_t i_val;
    uint32_t u_val;
    bool b_val;
    char str_buf[64];

    /* Attempt to decode various JSON keys */
    syn_json_read_int(&reader, "val", &i_val);
    syn_json_read_uint(&reader, "count", &u_val);
    syn_json_read_bool(&reader, "enable", &b_val);
    syn_json_read_string(&reader, "name", str_buf, sizeof(str_buf));

    return 0;
}
