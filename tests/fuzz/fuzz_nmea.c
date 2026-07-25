/**
 * @file fuzz_nmea.c
 * @brief libFuzzer target for syn_nmea sentence parser.
 */

#include "syntropic/proto/syn_nmea.h"

#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0)
        return 0;

    SYN_NMEA_Parser parser;
    syn_nmea_parser_init(&parser);

    char sentence[SYN_NMEA_MAX_SENTENCE_LEN + 1];
    for (size_t i = 0; i < size; i++) {
        if (syn_nmea_parser_feed(&parser, (char)data[i], sentence)) {
            (void)syn_nmea_validate(sentence);
            SYN_NMEA_SentenceType type = syn_nmea_get_type(sentence);

            if (type == SYN_NMEA_SENTENCE_GGA) {
                SYN_NMEA_GGA gga;
                syn_nmea_parse_gga(sentence, &gga);
            } else if (type == SYN_NMEA_SENTENCE_RMC) {
                SYN_NMEA_RMC rmc;
                syn_nmea_parse_rmc(sentence, &rmc);
            } else if (type == SYN_NMEA_SENTENCE_VTG) {
                SYN_NMEA_VTG vtg;
                syn_nmea_parse_vtg(sentence, &vtg);
            } else if (type == SYN_NMEA_SENTENCE_GSA) {
                SYN_NMEA_GSA gsa;
                syn_nmea_parse_gsa(sentence, &gsa);
            } else if (type == SYN_NMEA_SENTENCE_ZDA) {
                SYN_NMEA_ZDA zda;
                syn_nmea_parse_zda(sentence, &zda);
            }
        }
    }

    return 0;
}
