/**
 * @file test_lintp.c
 * @brief Unity tests for ISO 17987-2 LIN Transport Protocol (syn_lintp).
 */

#include "mocks/mock_port.h"
#include "syntropic/proto/syn_lintp.h"
#include "syntropic/syntropic.h"
#include "unity/unity.h"

#if !defined(SYN_USE_LINTP) || SYN_USE_LINTP

static uint8_t g_rx_buf[256];

static void test_lintp_init_and_idle(void)
{
    SYN_LINTP_Link link;
    syn_lintp_init(&link, 0x01, g_rx_buf, sizeof(g_rx_buf), NULL, 0);
    TEST_ASSERT_TRUE(syn_lintp_is_tx_idle(&link));

    uint8_t dummy_out[8];
    TEST_ASSERT_FALSE(syn_lintp_get_tx_frame(&link, dummy_out));

    uint8_t recv[32];
    TEST_ASSERT_EQUAL_INT(0, syn_lintp_receive(&link, recv, sizeof(recv)));
}

static void test_lintp_single_frame_tx_rx(void)
{
    SYN_LINTP_Link tx_link, rx_link;
    syn_lintp_init(&tx_link, 0x01, g_rx_buf, sizeof(g_rx_buf), NULL, 0);
    syn_lintp_init(&rx_link, 0x02, g_rx_buf, sizeof(g_rx_buf), NULL, 0);

    /* Send 4-byte payload (SF) to NAD 0x02 */
    uint8_t payload[4] = {0x22, 0xF1, 0x90, 0xAA};
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_lintp_send(&tx_link, 0x02, payload, sizeof(payload)));
    TEST_ASSERT_FALSE(syn_lintp_is_tx_idle(&tx_link));

    uint8_t frame[8];
    TEST_ASSERT_TRUE(syn_lintp_get_tx_frame(&tx_link, frame));
    TEST_ASSERT_TRUE(syn_lintp_is_tx_idle(&tx_link));
    TEST_ASSERT_EQUAL_HEX8(0x02, frame[0]);
    TEST_ASSERT_EQUAL_HEX8(0x04, frame[1]); /* SF PCI + len 4 */
    TEST_ASSERT_EQUAL_MEMORY(payload, &frame[2], 4);

    /* Receive SF on rx_link */
    syn_lintp_process_rx_frame(&rx_link, frame);

    uint8_t rx_payload[32];
    ssize_t rx_bytes = syn_lintp_receive(&rx_link, rx_payload, sizeof(rx_payload));
    TEST_ASSERT_EQUAL_INT(4, rx_bytes);
    TEST_ASSERT_EQUAL_MEMORY(payload, rx_payload, 4);
}

static void test_lintp_multi_frame_tx_rx(void)
{
    SYN_LINTP_Link tx_link, rx_link;
    syn_lintp_init(&tx_link, 0x01, g_rx_buf, sizeof(g_rx_buf), NULL, 0);
    syn_lintp_init(&rx_link, 0x02, g_rx_buf, sizeof(g_rx_buf), NULL, 0);

    /* 18-byte multi-frame payload */
    uint8_t payload[18];
    for (int i = 0; i < 18; i++) {
        payload[i] = (uint8_t)(0x10 + i);
    }

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_lintp_send(&tx_link, 0x02, payload, sizeof(payload)));
    TEST_ASSERT_FALSE(syn_lintp_is_tx_idle(&tx_link));

    /* 1. FF (First Frame): 5 payload bytes */
    uint8_t frame_ff[8];
    TEST_ASSERT_TRUE(syn_lintp_get_tx_frame(&tx_link, frame_ff));
    TEST_ASSERT_EQUAL_HEX8(0x02, frame_ff[0]);
    TEST_ASSERT_EQUAL_HEX8(0x10, frame_ff[1]); /* FF PCI hi */
    TEST_ASSERT_EQUAL_HEX8(18, frame_ff[2]);   /* FF len lo */
    TEST_ASSERT_EQUAL_MEMORY(&payload[0], &frame_ff[3], 5);

    syn_lintp_process_rx_frame(&rx_link, frame_ff);

    /* 2. CF #1 (Consecutive Frame SN=1): 6 payload bytes */
    uint8_t frame_cf1[8];
    TEST_ASSERT_TRUE(syn_lintp_get_tx_frame(&tx_link, frame_cf1));
    TEST_ASSERT_EQUAL_HEX8(0x02, frame_cf1[0]);
    TEST_ASSERT_EQUAL_HEX8(0x21, frame_cf1[1]); /* CF SN=1 */
    TEST_ASSERT_EQUAL_MEMORY(&payload[5], &frame_cf1[2], 6);

    syn_lintp_process_rx_frame(&rx_link, frame_cf1);

    /* 3. CF #2 (Consecutive Frame SN=2): 6 payload bytes */
    uint8_t frame_cf2[8];
    TEST_ASSERT_TRUE(syn_lintp_get_tx_frame(&tx_link, frame_cf2));
    TEST_ASSERT_EQUAL_HEX8(0x02, frame_cf2[0]);
    TEST_ASSERT_EQUAL_HEX8(0x22, frame_cf2[1]); /* CF SN=2 */
    TEST_ASSERT_EQUAL_MEMORY(&payload[11], &frame_cf2[2], 6);

    syn_lintp_process_rx_frame(&rx_link, frame_cf2);

    /* 4. CF #3 (Consecutive Frame SN=3): 1 payload byte + padding */
    uint8_t frame_cf3[8];
    TEST_ASSERT_TRUE(syn_lintp_get_tx_frame(&tx_link, frame_cf3));
    TEST_ASSERT_EQUAL_HEX8(0x02, frame_cf3[0]);
    TEST_ASSERT_EQUAL_HEX8(0x23, frame_cf3[1]); /* CF SN=3 */
    TEST_ASSERT_EQUAL_HEX8(payload[17], frame_cf3[2]);

    syn_lintp_process_rx_frame(&rx_link, frame_cf3);

    TEST_ASSERT_FALSE(syn_lintp_get_tx_frame(&tx_link, frame_cf3));
    TEST_ASSERT_TRUE(syn_lintp_is_tx_idle(&tx_link));

    /* Read final assembled payload on rx_link */
    uint8_t rx_payload[32];
    ssize_t rx_bytes = syn_lintp_receive(&rx_link, rx_payload, sizeof(rx_payload));
    TEST_ASSERT_EQUAL_INT(18, rx_bytes);
    TEST_ASSERT_EQUAL_MEMORY(payload, rx_payload, 18);
}

static void test_lintp_sn_error_and_timeout(void)
{
    SYN_LINTP_Link rx_link;
    syn_lintp_init(&rx_link, 0x02, g_rx_buf, sizeof(g_rx_buf), NULL, 0);

    /* Process valid FF for 12 bytes */
    uint8_t ff_frame[8] = {0x02, 0x10, 0x0C, 0x01, 0x02, 0x03, 0x04, 0x05};
    syn_lintp_process_rx_frame(&rx_link, ff_frame);

    /* Process bad CF (SN=3 instead of expected SN=1) */
    uint8_t bad_cf[8] = {0x02, 0x23, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B};
    syn_lintp_process_rx_frame(&rx_link, bad_cf);

    /* Reception should abort */
    uint8_t recv[32];
    TEST_ASSERT_EQUAL_INT(0, syn_lintp_receive(&rx_link, recv, sizeof(recv)));

    /* Test N_Cr Timeout */
    syn_lintp_process_rx_frame(&rx_link, ff_frame);
    syn_lintp_step(&rx_link, 1500); /* 1500ms > 1000ms N_Cr */

    uint8_t valid_cf1[8] = {0x02, 0x21, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B};
    syn_lintp_process_rx_frame(&rx_link, valid_cf1); /* Should be ignored since timed out */
    TEST_ASSERT_EQUAL_INT(0, syn_lintp_receive(&rx_link, recv, sizeof(recv)));
}

static void test_lintp_edge_cases_and_nulls(void)
{
    /* Null check validations */
    syn_lintp_init(NULL, 0x01, g_rx_buf, sizeof(g_rx_buf), NULL, 0);
    syn_lintp_set_timeouts(NULL, 500, 500);
    syn_lintp_set_padding(NULL, 0xAA);
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_lintp_send(NULL, 0x02, g_rx_buf, 4));
    uint8_t payload8[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_lintp_send(NULL, 0x02, NULL, 4));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_lintp_send(NULL, 0x02, payload8, 0));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_lintp_send(NULL, 0x02, payload8, 5000));
    TEST_ASSERT_FALSE(syn_lintp_get_tx_frame(NULL, payload8));
    syn_lintp_process_rx_frame(NULL, payload8);
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_lintp_receive(NULL, payload8, 4));
    syn_lintp_step(NULL, 100);
    TEST_ASSERT_TRUE(syn_lintp_is_tx_idle(NULL));

    /* Configuration setters */
    SYN_LINTP_Link link;
    syn_lintp_init(&link, 0x02, g_rx_buf, 10, NULL, 0);
    syn_lintp_set_timeouts(&link, 0, 0); /* Should fallback to default 1000 */
    TEST_ASSERT_EQUAL_UINT32(1000, link.timer_n_as_ms);
    TEST_ASSERT_EQUAL_UINT32(1000, link.timer_n_cr_ms);
    syn_lintp_set_timeouts(&link, 500, 600);
    TEST_ASSERT_EQUAL_UINT32(500, link.timer_n_as_ms);
    TEST_ASSERT_EQUAL_UINT32(600, link.timer_n_cr_ms);

    syn_lintp_set_padding(&link, 0xAA);
    TEST_ASSERT_EQUAL_HEX8(0xAA, link.padding_byte);

    /* TX Busy Check */
    uint8_t big_payload[20];
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_lintp_send(&link, 0x02, big_payload, sizeof(big_payload)));
    TEST_ASSERT_EQUAL_INT(SYN_BUSY, syn_lintp_send(&link, 0x02, payload8, 4));

    /* TX Timeout (N_As) test */
    syn_lintp_step(&link, 600);
    TEST_ASSERT_TRUE(syn_lintp_is_tx_idle(&link));

    /* RX Null and invalid frame processing */
    syn_lintp_process_rx_frame(&link, NULL);

    /* Ignore NAD not for this node */
    uint8_t wrong_nad_frame[8] = {0x05, 0x04, 0x22, 0x01, 0x02, 0x03, 0x04, 0x05};
    syn_lintp_process_rx_frame(&link, wrong_nad_frame);

    /* Invalid SF len 0 */
    uint8_t invalid_sf0[8] = {0x02, 0x00, 0x22, 0x01, 0x02, 0x03, 0x04, 0x05};
    syn_lintp_process_rx_frame(&link, invalid_sf0);

    /* Invalid SF len > 6 */
    uint8_t invalid_sf7[8] = {0x02, 0x07, 0x22, 0x01, 0x02, 0x03, 0x04, 0x05};
    syn_lintp_process_rx_frame(&link, invalid_sf7);

    /* Invalid FF len <= 6 */
    uint8_t invalid_ff5[8] = {0x02, 0x10, 0x05, 0x22, 0x01, 0x02, 0x03, 0x04};
    syn_lintp_process_rx_frame(&link, invalid_ff5);

    /* Invalid FF len > rx_buf_size (10) */
    uint8_t invalid_ff20[8] = {0x02, 0x10, 0x14, 0x22, 0x01, 0x02, 0x03, 0x04};
    syn_lintp_process_rx_frame(&link, invalid_ff20);

    /* Unknown PCI type (0x30) */
    uint8_t unknown_pci[8] = {0x02, 0x30, 0x22, 0x01, 0x02, 0x03, 0x04, 0x05};
    syn_lintp_process_rx_frame(&link, unknown_pci);

    /* Receive null checks */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_lintp_receive(&link, NULL, 10));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_lintp_receive(&link, g_rx_buf, 0));
}

void run_lintp_tests(void)
{
    RUN_TEST(test_lintp_init_and_idle);
    RUN_TEST(test_lintp_single_frame_tx_rx);
    RUN_TEST(test_lintp_multi_frame_tx_rx);
    RUN_TEST(test_lintp_sn_error_and_timeout);
    RUN_TEST(test_lintp_edge_cases_and_nulls);
}

#endif /* SYN_USE_LINTP */
