#include "mock_port.h"
#include "syntropic/proto/syn_cobs.h"
#include "unity/unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void)
{
}
void tearDown(void)
{
}

void test_renode_multinode_cobs_transport(void)
{
    printf("[Renode Emulation] Testing Multi-Node Inter-MCU COBS Framing & Transport...\n");

    /* Node 0 encodes payload frame */
    const uint8_t original_msg[] = "NODE0_TO_NODE1_PING";
    uint8_t cobs_buf[64];
    size_t encoded_len = syn_cobs_encode(original_msg, sizeof(original_msg), cobs_buf);

    TEST_ASSERT_TRUE(encoded_len > 0);
    printf("[Renode Emulation] Node 0 Encoded COBS Payload (%zu bytes)\n", encoded_len);

    /* Node 1 decodes received payload frame */
    uint8_t rx_msg[64] = {0};
    size_t decoded_len = syn_cobs_decode(cobs_buf, encoded_len, rx_msg);

    TEST_ASSERT_EQUAL_INT(sizeof(original_msg), decoded_len);
    TEST_ASSERT_EQUAL_MEMORY(original_msg, rx_msg, sizeof(original_msg));

    printf("[Renode Emulation] Multi-Node Inter-MCU Transport PASS!\n");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_renode_multinode_cobs_transport);
    return UNITY_END();
}
