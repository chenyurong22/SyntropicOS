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
    server->s3_timer_ms = 0U;
    server->security_error_count = 0U;
    server->security_delay_timer_ms = SYN_UDS_SECURITY_DELAY_MS;
    server->comm_control_state = SYN_UDS_COMM_ENABLE_RX_AND_TX;
    server->p2_max_ms = 50U;
    server->p2_star_max_10ms = 500U;
    server->active_p2_max_ms = 50U;
    server->active_p2_star_max_10ms = 500U;
    server->timing_cb = NULL;
    server->timing_ctx = NULL;
    server->did_count = 0U;
    server->reset_type_requested = 0U;

    return true;
}

void syn_uds_tick(SYN_UDS_Server *server, uint32_t dt_ms)
{
    if (server == NULL) {
        return;
    }

    /* 1. Security delay countdown */
    if (server->security_delay_timer_ms > 0U) {
        if (dt_ms >= server->security_delay_timer_ms) {
            server->security_delay_timer_ms = 0U;
            if (server->security_error_count > 0U) {
                server->security_error_count--;
            }
        } else {
            server->security_delay_timer_ms -= dt_ms;
        }
    }

    /* 2. S3 server session timeout countdown */
    if (server->session != SYN_UDS_SESSION_DEFAULT) {
        if (dt_ms >= server->s3_timer_ms) {
            /* S3 timer expired -> revert to DEFAULT session, lock security, and restore comm */
            server->session = SYN_UDS_SESSION_DEFAULT;
            server->security_state = SYN_UDS_SECURITY_LOCKED;
            server->comm_control_state = SYN_UDS_COMM_ENABLE_RX_AND_TX;
            server->s3_timer_ms = 0U;
        } else {
            server->s3_timer_ms -= dt_ms;
        }
    } else {
        server->s3_timer_ms = 0U;
    }
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

bool syn_uds_register_comm_control(SYN_UDS_Server *server, SYN_UDS_CommControlHandler handler,
                                   void *ctx)
{
    if (server == NULL) {
        return false;
    }
    server->comm_control_cb = handler;
    server->comm_control_ctx = ctx;
    return true;
}

bool syn_uds_register_access_timing(SYN_UDS_Server *server, SYN_UDS_AccessTimingHandler handler,
                                    void *ctx)
{
    if (server == NULL) {
        return false;
    }
    server->timing_cb = handler;
    server->timing_ctx = ctx;
    return true;
}

bool syn_uds_register_secured_data(SYN_UDS_Server *server, SYN_UDS_SecuredDataHandler handler,
                                   void *ctx)
{
    if (server == NULL) {
        return false;
    }
    server->secured_data_cb = handler;
    server->secured_data_ctx = ctx;
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
    bool success = false;

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

        /* ISO 14229 Session transition rules:
         * DEFAULT -> PROGRAMMING : Not allowed directly (must enter EXTENDED first).
         * PROGRAMMING -> EXTENDED : Not allowed directly (must return to DEFAULT first).
         */
        if ((server->session == SYN_UDS_SESSION_DEFAULT) && (sub == SYN_UDS_SESSION_PROGRAMMING)) {
            return make_negative_response(sid, SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp_buf,
                                          resp_len);
        }
        if ((server->session == SYN_UDS_SESSION_PROGRAMMING) && (sub == SYN_UDS_SESSION_EXTENDED)) {
            return make_negative_response(sid, SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp_buf,
                                          resp_len);
        }

        server->session = (SYN_UDS_Session)sub;
        if (server->session == SYN_UDS_SESSION_DEFAULT) {
            server->security_state = SYN_UDS_SECURITY_LOCKED;
            server->s3_timer_ms = 0U;
        } else {
            server->s3_timer_ms = SYN_UDS_S3_TIMEOUT_MS;
            if (server->session == SYN_UDS_SESSION_PROGRAMMING) {
                /* ISO 14229: Switching to programming session clears power-on safety delay */
                server->security_delay_timer_ms = 0U;
            }
        }

        resp_buf[0] = sid + 0x40U;
        resp_buf[1] = sub;
        resp_buf[2] = 0x00U; /* P2 Server max high byte */
        resp_buf[3] = 0x32U; /* P2 Server max low byte (50ms) */
        resp_buf[4] = 0x01U; /* P2* Server max high byte */
        resp_buf[5] = 0xF4U; /* P2* Server max low byte (5000ms) */
        *resp_len = 6U;
        success = true;
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
        success = true;
        break;
    }

    case SYN_UDS_SID_SECURITY_ACCESS: {
        if (req_len < 2U) {
            return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                          resp_len);
        }
        uint8_t sub = req[1];
        if (sub == 0x01U) { /* Request Seed */
            if (server->security_delay_timer_ms > 0U) {
                return make_negative_response(sid, SYN_UDS_NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED,
                                              resp_buf, resp_len);
            }
            server->security_state = SYN_UDS_SECURITY_SEED_SENT;
            resp_buf[0] = sid + 0x40U;
            resp_buf[1] = sub;
            syn_poke_u32(server->current_seed, resp_buf, 2);
            *resp_len = 6U;
            success = true;
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
                server->security_error_count = 0U;
                resp_buf[0] = sid + 0x40U;
                resp_buf[1] = sub;
                *resp_len = 2U;
                success = true;
            } else {
                server->security_state = SYN_UDS_SECURITY_LOCKED;
                server->security_error_count++;
                if (server->security_error_count >= SYN_UDS_SECURITY_MAX_ATTEMPTS) {
                    server->security_delay_timer_ms = SYN_UDS_SECURITY_DELAY_MS;
                    return make_negative_response(sid, SYN_UDS_NRC_EXCEEDED_NUMBER_OF_ATTEMPTS,
                                                  resp_buf, resp_len);
                }
                return make_negative_response(sid, SYN_UDS_NRC_INVALID_KEY, resp_buf, resp_len);
            }
        } else {
            return make_negative_response(sid, SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, resp_buf,
                                          resp_len);
        }
        break;
    }

    case SYN_UDS_SID_COMMUNICATION_CONTROL: {
        if (req_len < 3U) {
            return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                          resp_len);
        }
        if (server->session == SYN_UDS_SESSION_DEFAULT) {
            return make_negative_response(sid, SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp_buf,
                                          resp_len);
        }
        uint8_t sub = req[1] & 0x7FU;
        if (sub > 0x05U) {
            return make_negative_response(sid, SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, resp_buf,
                                          resp_len);
        }
        uint8_t comm_type = req[2];
        if (comm_type == 0U) {
            return make_negative_response(sid, SYN_UDS_NRC_REQUEST_OUT_OF_RANGE, resp_buf,
                                          resp_len);
        }

        if (server->comm_control_cb != NULL) {
            if (!server->comm_control_cb((SYN_UDS_CommControlType)sub, comm_type,
                                         server->comm_control_ctx)) {
                return make_negative_response(sid, SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp_buf,
                                              resp_len);
            }
        }

        server->comm_control_state = (SYN_UDS_CommControlType)sub;
        server->comm_type = comm_type;

        resp_buf[0] = sid + 0x40U;
        resp_buf[1] = sub;
        *resp_len = 2U;
        success = true;
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
        success = true;
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
        success = true;
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
        success = true;
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
        success = true;
        break;
    }

    case SYN_UDS_SID_ACCESS_TIMING_PARAMETER: {
        if (req_len < 2U) {
            return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                          resp_len);
        }
        uint8_t sub = req[1] & 0x7FU;
        switch (sub) {
        case SYN_UDS_TIMING_READ_EXTENDED: {
            if (req_len != 2U) {
                return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                              resp_len);
            }
            if (max_resp_len < 6U) {
                return make_negative_response(sid, SYN_UDS_NRC_RESPONSE_TOO_LONG, resp_buf,
                                              resp_len);
            }
            uint16_t p2 = server->p2_max_ms;
            uint16_t p2_star = server->p2_star_max_10ms;
            if (server->timing_cb != NULL) {
                if (!server->timing_cb(SYN_UDS_TIMING_READ_EXTENDED, &p2, &p2_star,
                                       server->timing_ctx)) {
                    return make_negative_response(sid, SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp_buf,
                                                  resp_len);
                }
            }
            resp_buf[0] = sid + 0x40U;
            resp_buf[1] = sub;
            syn_poke_u16(p2, resp_buf, 2);
            syn_poke_u16(p2_star, resp_buf, 4);
            *resp_len = 6U;
            success = true;
            break;
        }

        case SYN_UDS_TIMING_SET_TO_DEFAULT: {
            if (req_len != 2U) {
                return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                              resp_len);
            }
            uint16_t p2 = server->p2_max_ms;
            uint16_t p2_star = server->p2_star_max_10ms;
            if (server->timing_cb != NULL) {
                if (!server->timing_cb(SYN_UDS_TIMING_SET_TO_DEFAULT, &p2, &p2_star,
                                       server->timing_ctx)) {
                    return make_negative_response(sid, SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp_buf,
                                                  resp_len);
                }
            }
            server->active_p2_max_ms = p2;
            server->active_p2_star_max_10ms = p2_star;
            resp_buf[0] = sid + 0x40U;
            resp_buf[1] = sub;
            *resp_len = 2U;
            success = true;
            break;
        }

        case SYN_UDS_TIMING_READ_ACTIVE: {
            if (req_len != 2U) {
                return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                              resp_len);
            }
            if (max_resp_len < 6U) {
                return make_negative_response(sid, SYN_UDS_NRC_RESPONSE_TOO_LONG, resp_buf,
                                              resp_len);
            }
            uint16_t p2 = server->active_p2_max_ms;
            uint16_t p2_star = server->active_p2_star_max_10ms;
            if (server->timing_cb != NULL) {
                if (!server->timing_cb(SYN_UDS_TIMING_READ_ACTIVE, &p2, &p2_star,
                                       server->timing_ctx)) {
                    return make_negative_response(sid, SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp_buf,
                                                  resp_len);
                }
            }
            resp_buf[0] = sid + 0x40U;
            resp_buf[1] = sub;
            syn_poke_u16(p2, resp_buf, 2);
            syn_poke_u16(p2_star, resp_buf, 4);
            *resp_len = 6U;
            success = true;
            break;
        }

        case SYN_UDS_TIMING_SET_TO_GIVEN: {
            if (req_len != 6U) {
                return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                              resp_len);
            }
            uint16_t given_p2 = syn_peek_u16(req, 2);
            uint16_t given_p2_star = syn_peek_u16(req, 4);
            if (server->timing_cb != NULL) {
                if (!server->timing_cb(SYN_UDS_TIMING_SET_TO_GIVEN, &given_p2, &given_p2_star,
                                       server->timing_ctx)) {
                    return make_negative_response(sid, SYN_UDS_NRC_REQUEST_OUT_OF_RANGE, resp_buf,
                                                  resp_len);
                }
            }
            server->active_p2_max_ms = given_p2;
            server->active_p2_star_max_10ms = given_p2_star;
            resp_buf[0] = sid + 0x40U;
            resp_buf[1] = sub;
            *resp_len = 2U;
            success = true;
            break;
        }

        default: {
            return make_negative_response(sid, SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, resp_buf,
                                          resp_len);
        }
        }
        break;
    }

    case SYN_UDS_SID_SECURED_DATA_TRANSMISSION: {
        if (req_len < 2U) {
            return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                          resp_len);
        }
        if (server->security_state != SYN_UDS_SECURITY_UNLOCKED) {
            return make_negative_response(sid, SYN_UDS_NRC_SECURITY_ACCESS_DENIED, resp_buf,
                                          resp_len);
        }
        uint16_t sec_in_len = req_len - 1U;
        const uint8_t *sec_in_data = &req[1];
        uint16_t sec_out_len = 0U;

        if (server->secured_data_cb != NULL) {
            if (!server->secured_data_cb(sec_in_data, sec_in_len, &resp_buf[1], max_resp_len - 1U,
                                         &sec_out_len, server->secured_data_ctx)) {
                return make_negative_response(sid, SYN_UDS_NRC_CONDITIONS_NOT_CORRECT, resp_buf,
                                              resp_len);
            }
        } else {
            if (sec_in_len > (max_resp_len - 1U)) {
                return make_negative_response(sid, SYN_UDS_NRC_RESPONSE_TOO_LONG, resp_buf,
                                              resp_len);
            }
            memcpy(&resp_buf[1], sec_in_data, sec_in_len);
            sec_out_len = sec_in_len;
        }

        resp_buf[0] = sid + 0x40U;
        *resp_len = 1U + sec_out_len;
        success = true;
        break;
    }

    case SYN_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION: {
        if (req_len < 4U) {
            return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                          resp_len);
        }
        resp_buf[0] = sid + 0x40U;
        *resp_len = 1U;
        success = true;
        break;
    }

    case SYN_UDS_SID_READ_DTC_INFORMATION: {
        if (req_len < 2U) {
            return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                          resp_len);
        }
        uint8_t sub = req[1] & 0x7FU;
        resp_buf[0] = sid + 0x40U;
        resp_buf[1] = sub;
        resp_buf[2] = 0x00U;
        *resp_len = 3U;
        success = true;
        break;
    }

    case SYN_UDS_SID_CONTROL_DTC_SETTING: {
        if (req_len < 2U) {
            return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                          resp_len);
        }
        uint8_t sub = req[1] & 0x7FU;
        if (sub != 0x01U && sub != 0x02U) {
            return make_negative_response(sid, SYN_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, resp_buf,
                                          resp_len);
        }
        resp_buf[0] = sid + 0x40U;
        resp_buf[1] = sub;
        *resp_len = 2U;
        success = true;
        break;
    }

    case SYN_UDS_SID_RESPONSE_ON_EVENT: {
        if (req_len < 2U) {
            return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                          resp_len);
        }
        uint8_t sub = req[1] & 0x7FU;
        resp_buf[0] = sid + 0x40U;
        resp_buf[1] = sub;
        *resp_len = 2U;
        success = true;
        break;
    }

    case SYN_UDS_SID_REQUEST_DOWNLOAD:
    case SYN_UDS_SID_REQUEST_UPLOAD: {
        if (req_len < 3U) {
            return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                          resp_len);
        }
        if (server->security_state != SYN_UDS_SECURITY_UNLOCKED) {
            return make_negative_response(sid, SYN_UDS_NRC_SECURITY_ACCESS_DENIED, resp_buf,
                                          resp_len);
        }
        resp_buf[0] = sid + 0x40U;
        resp_buf[1] = 0x20U;
        syn_poke_u16(0x0400U, resp_buf, 2);
        *resp_len = 4U;
        success = true;
        break;
    }

    case SYN_UDS_SID_TRANSFER_DATA: {
        if (req_len < 2U) {
            return make_negative_response(sid, SYN_UDS_NRC_INCORRECT_MESSAGE_LENGTH, resp_buf,
                                          resp_len);
        }
        uint8_t block_seq = req[1];
        resp_buf[0] = sid + 0x40U;
        resp_buf[1] = block_seq;
        *resp_len = 2U;
        success = true;
        break;
    }

    case SYN_UDS_SID_REQUEST_TRANSFER_EXIT: {
        resp_buf[0] = sid + 0x40U;
        *resp_len = 1U;
        success = true;
        break;
    }

    default: {
        return make_negative_response(sid, SYN_UDS_NRC_SERVICE_NOT_SUPPORTED, resp_buf, resp_len);
    }
    }

    if (success && (server->session != SYN_UDS_SESSION_DEFAULT)) {
        server->s3_timer_ms = SYN_UDS_S3_TIMEOUT_MS;
    }

    return true;
}
