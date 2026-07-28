#include "syntropic/proto/syn_bacnet.h"
#include "unity/unity.h"
#include <string.h>

static void test_bacnet_crc8(void)
{
    uint8_t header[] = {0x00, 0x01, 0x02, 0x00, 0x00};
    uint8_t crc = syn_bacnet_crc8(header, 5);
    TEST_ASSERT_NOT_EQUAL(0, crc);
}

static void test_bacnet_crc16(void)
{
    uint8_t payload[] = {0x10, 0x08};
    uint16_t crc = syn_bacnet_crc16(payload, 2);
    TEST_ASSERT_NOT_EQUAL(0, crc);
}

static void test_bacnet_mstp_frame_roundtrip(void)
{
    uint8_t raw_buf[64];
    uint8_t payload[] = {0x10, 0x08, 0xC4, 0x00, 0x01, 0x02};

    size_t encoded_len = syn_bacnet_mstp_encode_frame(SYN_BACNET_MSTP_FRAME_DATA_NOT_EXPECTING_REPLY,
                                                      10, 5, payload, 6, raw_buf);

    TEST_ASSERT_EQUAL_INT(8 + 6 + 2, encoded_len);

    SYN_BACnet_MSTP_Frame rx_frame;
    bool valid = syn_bacnet_mstp_decode_frame(raw_buf, encoded_len, &rx_frame);

    TEST_ASSERT_TRUE(valid);
    TEST_ASSERT_EQUAL_UINT8(SYN_BACNET_MSTP_FRAME_DATA_NOT_EXPECTING_REPLY, rx_frame.frame_type);
    TEST_ASSERT_EQUAL_UINT8(10, rx_frame.destination_mac);
    TEST_ASSERT_EQUAL_UINT8(5, rx_frame.source_mac);
    TEST_ASSERT_EQUAL_UINT16(6, rx_frame.data_len);
    TEST_ASSERT_EQUAL_MEMORY(payload, rx_frame.payload, 6);
}

static void test_bacnet_node_init_and_objects(void)
{
    SYN_BACnet_Node node;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_bacnet_node_init(&node, 12, 123456));

    TEST_ASSERT_EQUAL_UINT8(12, node.mac_address);
    TEST_ASSERT_EQUAL_UINT32(123456, node.device_id);
    TEST_ASSERT_EQUAL_INT(1, node.object_count); /* Device object automatically added */

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_bacnet_add_object(&node, SYN_BACNET_OBJ_ANALOG_INPUT, 1, 23.5f, "Temperature"));
    TEST_ASSERT_EQUAL_INT(2, node.object_count);
}

static void test_bacnet_node_who_is_process(void)
{
    SYN_BACnet_Node node;
    syn_bacnet_node_init(&node, 5, 555);

    /* Construct Who-Is request frame */
    SYN_BACnet_MSTP_Frame req = {
        .frame_type = SYN_BACNET_MSTP_FRAME_DATA_NOT_EXPECTING_REPLY,
        .destination_mac = SYN_BACNET_BROADCAST_MAC,
        .source_mac = 1,
        .data_len = 2,
        .payload = {0x10, SYN_BACNET_SERVICE_UNCONFIRMED_WHO_IS}
    };

    SYN_BACnet_MSTP_Frame tx_frame;
    bool has_tx = false;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_bacnet_node_process(&node, &req, &tx_frame, &has_tx));
    TEST_ASSERT_TRUE(has_tx);
    TEST_ASSERT_EQUAL_UINT8(SYN_BACNET_BROADCAST_MAC, tx_frame.destination_mac);
    TEST_ASSERT_EQUAL_UINT8(5, tx_frame.source_mac);
    TEST_ASSERT_EQUAL_UINT8(SYN_BACNET_SERVICE_UNCONFIRMED_I_AM, tx_frame.payload[1]);
}

void run_bacnet_tests(void)
{
    RUN_TEST(test_bacnet_crc8);
    RUN_TEST(test_bacnet_crc16);
    RUN_TEST(test_bacnet_mstp_frame_roundtrip);
    RUN_TEST(test_bacnet_node_init_and_objects);
    RUN_TEST(test_bacnet_node_who_is_process);
}
