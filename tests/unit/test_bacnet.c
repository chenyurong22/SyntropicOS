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

    size_t encoded_len = syn_bacnet_mstp_encode_frame(
        SYN_BACNET_MSTP_FRAME_DATA_NOT_EXPECTING_REPLY, 10, 5, payload, 6, raw_buf);

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

    TEST_ASSERT_EQUAL_INT(
        SYN_OK, syn_bacnet_add_object(&node, SYN_BACNET_OBJ_ANALOG_INPUT, 1, 23.5f, "Temperature"));
    TEST_ASSERT_EQUAL_INT(2, node.object_count);
}

static void test_bacnet_node_who_is_process(void)
{
    SYN_BACnet_Node node;
    syn_bacnet_node_init(&node, 5, 555);

    /* Construct Who-Is request frame */
    SYN_BACnet_MSTP_Frame req = {.frame_type = SYN_BACNET_MSTP_FRAME_DATA_NOT_EXPECTING_REPLY,
                                 .destination_mac = SYN_BACNET_BROADCAST_MAC,
                                 .source_mac = 1,
                                 .data_len = 2,
                                 .payload = {0x10, SYN_BACNET_SERVICE_UNCONFIRMED_WHO_IS}};

    SYN_BACnet_MSTP_Frame tx_frame;
    bool has_tx = false;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_bacnet_node_process(&node, &req, &tx_frame, &has_tx));
    TEST_ASSERT_TRUE(has_tx);
    TEST_ASSERT_EQUAL_UINT8(SYN_BACNET_BROADCAST_MAC, tx_frame.destination_mac);
    TEST_ASSERT_EQUAL_UINT8(5, tx_frame.source_mac);
    TEST_ASSERT_EQUAL_UINT8(SYN_BACNET_SERVICE_UNCONFIRMED_I_AM, tx_frame.payload[1]);
}

static void test_bacnet_poll_for_master(void)
{
    SYN_BACnet_Node node;
    syn_bacnet_node_init(&node, 10, 100);

    SYN_BACnet_MSTP_Frame req = {.frame_type = SYN_BACNET_MSTP_FRAME_POLL_FOR_MASTER,
                                 .destination_mac = 10,
                                 .source_mac = 2,
                                 .data_len = 0};

    SYN_BACnet_MSTP_Frame tx_frame;
    bool has_tx = false;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_bacnet_node_process(&node, &req, &tx_frame, &has_tx));
    TEST_ASSERT_TRUE(has_tx);
    TEST_ASSERT_EQUAL_UINT8(SYN_BACNET_MSTP_FRAME_REPLY_TO_POLL_FOR_MASTER, tx_frame.frame_type);
    TEST_ASSERT_EQUAL_UINT8(2, tx_frame.destination_mac);
    TEST_ASSERT_EQUAL_UINT8(10, tx_frame.source_mac);
}

static void test_bacnet_read_property(void)
{
    SYN_BACnet_Node node;
    syn_bacnet_node_init(&node, 7, 700);
    syn_bacnet_add_object(&node, SYN_BACNET_OBJ_ANALOG_INPUT, 1, 42.0f, "TempSensor");

    /* Object 0 present_value read */
    node.objects[0].present_value = 42.0f;

    SYN_BACnet_MSTP_Frame req = {.frame_type = SYN_BACNET_MSTP_FRAME_DATA_EXPECTING_REPLY,
                                 .destination_mac = 7,
                                 .source_mac = 3,
                                 .data_len = 2,
                                 .payload = {0x00, SYN_BACNET_SERVICE_CONFIRMED_READ_PROPERTY}};

    SYN_BACnet_MSTP_Frame tx_frame;
    bool has_tx = false;

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_bacnet_node_process(&node, &req, &tx_frame, &has_tx));
    TEST_ASSERT_TRUE(has_tx);
    TEST_ASSERT_EQUAL_UINT8(3, tx_frame.destination_mac);
    TEST_ASSERT_EQUAL_UINT8(SYN_BACNET_SERVICE_CONFIRMED_READ_PROPERTY, tx_frame.payload[2]);

    float val = 0.0f;
    memcpy(&val, &tx_frame.payload[4], sizeof(float));
    TEST_ASSERT_EQUAL_FLOAT(42.0f, val);
}

static void test_bacnet_edge_cases_and_nulls(void)
{
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_bacnet_node_init(NULL, 1, 1));
    TEST_ASSERT_EQUAL_INT(SYN_ERROR, syn_bacnet_add_object(NULL, 0, 0, 0.0f, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_bacnet_node_process(NULL, NULL, NULL, NULL));

    SYN_BACnet_Node node;
    syn_bacnet_node_init(&node, 5, 50);

    /* Mismatched destination MAC -> ignored */
    SYN_BACnet_MSTP_Frame req_mismatch = {.destination_mac = 99};
    SYN_BACnet_MSTP_Frame tx_frame;
    bool has_tx = false;
    TEST_ASSERT_EQUAL_INT(SYN_OK,
                          syn_bacnet_node_process(&node, &req_mismatch, &tx_frame, &has_tx));
    TEST_ASSERT_FALSE(has_tx);

    /* Token frame -> ignored */
    SYN_BACnet_MSTP_Frame req_token = {.frame_type = SYN_BACNET_MSTP_FRAME_TOKEN,
                                       .destination_mac = 5};
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_bacnet_node_process(&node, &req_token, &tx_frame, &has_tx));
    TEST_ASSERT_FALSE(has_tx);
}

static void test_bacnet_add_object_max_capacity(void)
{
    SYN_BACnet_Node node;
    syn_bacnet_node_init(&node, 1, 10);
    for (int i = 0; i < SYN_BACNET_MAX_OBJECTS - 1; i++) {
        TEST_ASSERT_EQUAL_INT(
            SYN_OK, syn_bacnet_add_object(&node, SYN_BACNET_OBJ_ANALOG_INPUT, i, 1.0f, "AI"));
    }
    TEST_ASSERT_EQUAL_INT(
        SYN_ERROR, syn_bacnet_add_object(&node, SYN_BACNET_OBJ_ANALOG_INPUT, 99, 1.0f, "AI"));
}

void run_bacnet_tests(void)
{
    RUN_TEST(test_bacnet_crc8);
    RUN_TEST(test_bacnet_crc16);
    RUN_TEST(test_bacnet_mstp_frame_roundtrip);
    RUN_TEST(test_bacnet_node_init_and_objects);
    RUN_TEST(test_bacnet_node_who_is_process);
    RUN_TEST(test_bacnet_poll_for_master);
    RUN_TEST(test_bacnet_read_property);
    RUN_TEST(test_bacnet_edge_cases_and_nulls);
    RUN_TEST(test_bacnet_add_object_max_capacity);
}
