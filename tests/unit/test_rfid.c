/**
 * @file test_rfid.c
 * @brief Unity tests for syn_rfid module.
 */

#include "mocks/mock_port.h"
#include "syntropic/drivers/syn_rfid.h"
#include "unity/unity.h"

static void test_rfid_operations(void)
{
    mock_port_reset();
    SYN_RFID rfid;

    SYN_Status st = syn_rfid_init(&rfid, 0, 1, SYN_RFID_MFRC522);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    uint8_t uid[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    syn_rfid_feed_card(&rfid, uid, 4);

    TEST_ASSERT_TRUE(syn_rfid_is_card_present(&rfid));

    uint8_t len = 0;
    const uint8_t *p_uid = syn_rfid_get_uid(&rfid, &len);
    TEST_ASSERT_NOT_NULL(p_uid);
    TEST_ASSERT_EQUAL_UINT8(4, len);
    TEST_ASSERT_EQUAL_UINT8(0xDE, p_uid[0]);

    syn_rfid_clear_card(&rfid);
    TEST_ASSERT_FALSE(syn_rfid_is_card_present(&rfid));

    /* NULL guards */
    syn_rfid_feed_card(NULL, NULL, 0);
    syn_rfid_clear_card(NULL);
    TEST_ASSERT_FALSE(syn_rfid_is_card_present(NULL));
    TEST_ASSERT_NULL(syn_rfid_get_uid(NULL, NULL));
}

void run_rfid_tests(void)
{
    RUN_TEST(test_rfid_operations);
}
