/**
 * @file syn_uds.c
 * @brief ISO 14229 Unified Diagnostic Services (UDS) Server Implementation.
 */

#include "syn_uds.h"

#include "../util/syn_pack.h"

#include <string.h>

bool syn_uds_init(SYN_UDS_Server *server)
{
    if (server == NULL) {
        return false;
    }

    memset(server, 0, sizeof(*server));
    server->session = SYN_UDS_SESSION_DEFAULT;
    server->security_state = SYN_UDS_SECURITY_LOCKED;
    server->current_seed = 0x12345678U;
    server->did_count = 0U;
    server->reset_type_requested = 0U;

    return true;
}

bool syn_uds_register_did(SYN_UDS_Server *server, uint16_t did, uint8_t *data, uint16_t len,
                          bool writable)
{
    if ((server == NULL) || (data == NULL) || (len == 0U)) {
        return false;
    }

    if (server->did_count >= SYN_UDS_MAX_DIDS) {
        return false;
    }

    SYN_UDS_DIDEntry *entry = &server->did_table[server->did_count];
    entry->did = did;
    entry->data = data;
    entry->len = len;
    entry->writable = writable;

    server->did_count++;
    return true;
}

static bool make_negative_response(uint8_t sid, uint8_t nrc, uint8_t *resp_buf, uint16_t *resp_len)
{
    resp_buf[0] = SYN_UDS_RESPONSE_NEGATIVE;
    resp_buf[1] = sid;
    resp_buf[2] = nrc;
    *resp_len = 3U;
    return true;
}

bool syn_uds_process_request(SYN_UDS_Server *server, const uint8_t *req, uint16_t req_len,
                             uint8_t *resp_buf, uint16_t max_resp_len, uint16_t *resp_len)
{
    if ((server == NULL) || (req == NULL) || (req_len == 0U) || (resp_buf == NULL) ||
        (max_resp_len < 3U) || (resp_len == NULL)) {
        return false;
    }

    uint8_t sid = req[0];

    switch (sid) {
    case SYN_UDS_SID_DIAGNOSTIC_SESSION_CONTROL: {
        if (req_len < 2U) {
            return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                          resp_len);
        }
        uint8_t sub = req[1] & 0x7FU;
        if ((sub != SYN_UDS_SESSION_DEFAULT) && (sub != SYN_UDS_SESSION_PROGRAMMING) &&
            (sub != SYN_UDS_SESSION_EXTENDED)) {
            return make_negative_response(sid, SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, resp_buf,
                                          resp_len);
        }
        server->session = (SYN_UDS_Session)sub;
        if (server->session == SYN_UDS_SESSION_DEFAULT) {
            server->security_state = SYN_UDS_SECURITY_LOCKED;
        }

        resp_buf[0] = sid + 0x40U;
        resp_buf[1] = sub;
        resp_buf[2] = 0x00U; /* P2 Server max high byte */
        resp_buf[3] = 0x32U; /* P2 Server max low byte (50ms) */
        resp_buf[4] = 0x01U; /* P2* Server max high byte */
        resp_buf[5] = 0xF4U; /* P2* Server max low byte (5000ms) */
        *resp_len = 6U;
        break;
    }

    case SYN_UDS_SID_ECU_RESET: {
        if (req_len < 2U) {
            return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                          resp_len);
        }
        uint8_t sub = req[1] & 0x7FU;
        if ((sub != 0x01U) && (sub != 0x02U) && (sub != 0x03U)) {
            return make_negative_response(sid, SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, resp_buf,
                                          resp_len);
        }
        server->reset_type_requested = sub;
        resp_buf[0] = sid + 0x40U;
        resp_buf[1] = sub;
        *resp_len = 2U;
        break;
    }

    case SYN_UDS_SID_SECURITY_ACCESS: {
        if (req_len < 2U) {
            return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                          resp_len);
        }
        uint8_t sub = req[1];
        if (sub == 0x01U) { /* Request Seed */
            server->security_state = SYN_UDS_SECURITY_SEED_SENT;
            resp_buf[0] = sid + 0x40U;
            resp_buf[1] = sub;
            syn_poke_u32(server->current_seed, resp_buf, 2);
            *resp_len = 6U;
        } else if (sub == 0x02U) { /* Send Key */
            if (req_len < 6U) {
                return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                              resp_len);
            }
            if (server->security_state != SYN_UDS_SECURITY_SEED_SENT) {
                return make_negative_response(sid, SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp_buf,
                                              resp_len);
            }
            uint32_t key = syn_peek_u32(req, 2);
            /* Key calculation algorithm: key = seed ^ 0xA5A5A5A5 */
            uint32_t expected_key = server->current_seed ^ 0xA5A5A5A5U;
            if (key == expected_key) {
                server->security_state = SYN_UDS_SECURITY_UNLOCKED;
                resp_buf[0] = sid + 0x40U;
                resp_buf[1] = sub;
                *resp_len = 2U;
            } else {
                server->security_state = SYN_UDS_SECURITY_LOCKED;
                return make_negative_response(sid, SYN_UDS_NRC_INVALID_KEY, resp_buf, resp_len);
            }
        } else {
            return make_negative_response(sid, SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, resp_buf,
                                          resp_len);
        }
        break;
    }

    case SYN_UDS_SID_READ_DATA_BY_IDENTIFIER: {
        if (req_len < 3U) {
            return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                          resp_len);
        }
        uint16_t target_did = syn_peek_u16(req, 1);
        SYN_UDS_DIDEntry *matched_entry = NULL;
        for (uint8_t i = 0U; i < server->did_count; i++) {
            if (server->did_table[i].did == target_did) {
                matched_entry = &server->did_table[i];
                break;
            }
        }
        if (matched_entry == NULL) {
            return make_negative_response(sid, SYN_UDS_NRC_REQUEST_OUT_OF_RANGE, resp_buf,
                                          resp_len);
        }
        if (3U + matched_entry->len > max_resp_len) {
            return make_negative_response(sid, SYN_UDS_NRC_RESPONSE_TOO_LONG, resp_buf, resp_len);
        }
        resp_buf[0] = sid + 0x40U;
        syn_poke_u16(target_did, resp_buf, 1);
        memcpy(&resp_buf[3], matched_entry->data, matched_entry->len);
        *resp_len = 3U + matched_entry->len;
        break;
    }

    case SYN_UDS_SID_WRITE_DATA_BY_IDENTIFIER: {
        if (req_len < 4U) {
            return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                          resp_len);
        }
        uint16_t target_did = syn_peek_u16(req, 1);
        SYN_UDS_DIDEntry *matched_entry = NULL;
        for (uint8_t i = 0U; i < server->did_count; i++) {
            if (server->did_table[i].did == target_did) {
                matched_entry = &server->did_table[i];
                break;
            }
        }
        if (matched_entry == NULL) {
            return make_negative_response(sid, SYN_UDS_NRC_REQUEST_OUT_OF_RANGE, resp_buf,
                                          resp_len);
        }
        if (!matched_entry->writable) {
            return make_negative_response(sid, SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp_buf,
                                          resp_len);
        }
        if (server->security_state != SYN_UDS_SECURITY_UNLOCKED) {
            return make_negative_response(sid, SYN_UDS_NRC_SECURITY_ACCESS_DENIED, resp_buf,
                                          resp_len);
        }
        uint16_t data_len = req_len - 3U;
        if (data_len > matched_entry->len) {
            data_len = matched_entry->len;
        }
        memcpy(matched_entry->data, &req[3], data_len);
        resp_buf[0] = sid + 0x40U;
        syn_poke_u16(target_did, resp_buf, 1);
        *resp_len = 3U;
        break;
    }

    case SYN_UDS_SID_ROUTINE_CONTROL: {
        if (req_len < 4U) {
            return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                          resp_len);
        }
        uint8_t sub = req[1];
        uint16_t routine_id = syn_peek_u16(req, 2);

        resp_buf[0] = sid + 0x40U;
        resp_buf[1] = sub;
        syn_poke_u16(routine_id, resp_buf, 2);
        *resp_len = 4U;
        break;
    }

    case SYN_UDS_SID_TESTER_PRESENT: {
        if (req_len < 2U) {
            return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                          resp_len);
        }
        uint8_t sub = req[1] & 0x7FU;
        resp_buf[0] = sid + 0x40U;
        resp_buf[1] = sub;
        *resp_len = 2U;
        break;
    }

    default: {
        return make_negative_response(sid, SYN_UDS_NRC_SERVICE_NOT_SUPPORTED, resp_buf, resp_len);
    }
    }

    return true;
}
