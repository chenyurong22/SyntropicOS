/**
 * @file syn_at_parser.c
 * @brief Implementation of stream-oriented AT command parser.
 * @ingroup syn_protocol
 */

#include "syn_at_parser.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

SYN_Status syn_at_parser_init(SYN_AtParser *parser, char *buf, size_t buf_size)
{
    if (parser == NULL || buf == NULL || buf_size == 0) {
        return SYN_INVALID_PARAM;
    }

    parser->line_buf = buf;
    parser->line_buf_size = buf_size;
    syn_at_parser_reset(parser);

    return SYN_OK;
}

void syn_at_parser_reset(SYN_AtParser *parser)
{
    if (parser == NULL) {
        return;
    }

    parser->line_len = 0;
    parser->last_resp = SYN_AT_RESP_NONE;
    parser->cme_error_code = -1;
    parser->prompt_detected = false;
    if (parser->line_buf != NULL && parser->line_buf_size > 0) {
        parser->line_buf[0] = '\0';
    }
}

/**
 * @brief Check if string starts with given prefix.
 * @param str Input string.
 * @param prefix Prefix string.
 * @return True if str starts with prefix.
 */
static bool starts_with(const char *str, const char *prefix)
{
    if (str == NULL || prefix == NULL)
        return false;
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

SYN_AtRespType syn_at_parser_feed_char(SYN_AtParser *parser, char c)
{
    if (parser == NULL || parser->line_buf == NULL) {
        return SYN_AT_RESP_NONE;
    }

    /* Ignore carriage return characters (\r) in CRLF sequences */
    if (c == '\r') {
        return SYN_AT_RESP_NONE;
    }

    /* Prompt detection: '>' as first non-whitespace character */
    if (c == '>' && parser->line_len == 0) {
        parser->prompt_detected = true;
        parser->last_resp = SYN_AT_RESP_PROMPT;
        return SYN_AT_RESP_PROMPT;
    }

    if (c == '\n') {
        if (parser->line_len == 0) {
            /* Ignore empty lines (leading \n sequences) */
            return SYN_AT_RESP_NONE;
        }

        parser->line_buf[parser->line_len] = '\0';
        const char *line = parser->line_buf;

        /* Classify response line */
        if (strcmp(line, "OK") == 0) {
            parser->last_resp = SYN_AT_RESP_OK;
        } else if (strcmp(line, "ERROR") == 0) {
            parser->last_resp = SYN_AT_RESP_ERROR;
        } else if (starts_with(line, "+CME ERROR:") || starts_with(line, "+CMS ERROR:")) {
            parser->last_resp = SYN_AT_RESP_CME_ERROR;
            const char *p = strchr(line, ':');
            if (p != NULL) {
                parser->cme_error_code = atoi(p + 1);
            } else {
                parser->cme_error_code = -1;
            }
        } else if (starts_with(line, "+RECEIVE,") || starts_with(line, "+IPD,") ||
                   starts_with(line, "CLOSED") || starts_with(line, "SHUT OK") ||
                   starts_with(line, "RING") || starts_with(line, "+CMTI:")) {
            parser->last_resp = SYN_AT_RESP_URC;
        } else {
            parser->last_resp = SYN_AT_RESP_LINE;
        }

        parser->line_len = 0; /* Reset buffer for next line */
        return parser->last_resp;
    }

    /* Accumulate character */
    if (parser->line_len < parser->line_buf_size - 1) {
        parser->line_buf[parser->line_len++] = c;
    }

    return SYN_AT_RESP_NONE;
}

SYN_AtRespType syn_at_parser_feed_stream(SYN_AtParser *parser, SYN_Stream *stream)
{
    if (parser == NULL || stream == NULL) {
        return SYN_AT_RESP_NONE;
    }

    uint8_t byte;
    while (syn_stream_read(stream, &byte, 1) == 1) {
        SYN_AtRespType resp = syn_at_parser_feed_char(parser, (char)byte);
        if (resp != SYN_AT_RESP_NONE) {
            return resp;
        }
    }

    return SYN_AT_RESP_NONE;
}

const char *syn_at_parser_get_line(const SYN_AtParser *parser)
{
    if (parser == NULL || parser->line_buf == NULL) {
        return "";
    }
    return parser->line_buf;
}

int syn_at_parser_get_cme_error(const SYN_AtParser *parser)
{
    if (parser == NULL) {
        return -1;
    }
    return parser->cme_error_code;
}

/**
 * @brief Find starting position of parameter by index.
 * @param line Input line string.
 * @param param_idx 0-based parameter index.
 * @return Pointer to start of parameter, or NULL if index out of range.
 */
static const char *find_param_start(const char *line, size_t param_idx)
{
    if (line == NULL) {
        return NULL;
    }

    /* Skip header prefix if present (e.g. "+CSQ: ") */
    const char *p = strchr(line, ':');
    if (p != NULL) {
        p++;
    } else {
        p = line;
    }

    while (*p == ' ') {
        p++;
    }

    size_t curr_idx = 0;
    while (curr_idx < param_idx) {
        p = strchr(p, ',');
        if (p == NULL) {
            return NULL;
        }
        p++;
        while (*p == ' ') {
            p++;
        }
        curr_idx++;
    }

    return p;
}

bool syn_at_parser_get_param_int(const char *line, size_t param_idx, int *out_val)
{
    if (out_val == NULL) {
        return false;
    }

    const char *p = find_param_start(line, param_idx);
    if (p == NULL || *p == '\0') {
        return false;
    }

    *out_val = atoi(p);
    return true;
}

bool syn_at_parser_get_param_str(const char *line, size_t param_idx, char *out_buf, size_t max_len)
{
    if (out_buf == NULL || max_len == 0) {
        return false;
    }

    const char *p = find_param_start(line, param_idx);
    if (p == NULL) {
        return false;
    }

    /* Skip leading quote */
    if (*p == '"') {
        p++;
    }

    size_t out_idx = 0;
    while (*p != '\0' && *p != ',' && *p != '"' && *p != '\r' && *p != '\n' &&
           out_idx < max_len - 1) {
        out_buf[out_idx++] = *p++;
    }
    out_buf[out_idx] = '\0';

    return true;
}
