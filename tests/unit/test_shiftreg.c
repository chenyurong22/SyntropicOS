/**
 * @file test_shiftreg.c
 * @brief Unity tests for syn_shiftreg module.
 */

#include "mocks/mock_port.h"
#include "syntropic/drivers/syn_shiftreg.h"
#include "unity/unity.h"

static void test_shiftreg_out_operations(void)
{
    mock_port_reset();
    SYN_ShiftRegOut sr;

    SYN_Status st = syn_shiftreg_out_init(&sr, 1, 2, 3, 2); /* 2 chips = 16 bits */
    TEST_ASSERT_EQUAL(SYN_OK, st);

    syn_shiftreg_out_set_oe_pin(&sr, 4);

    /* Set bits 0 and 10 */
    syn_shiftreg_out_set_bit(&sr, 0, true);
    syn_shiftreg_out_set_bit(&sr, 10, true);
    TEST_ASSERT_EQUAL_HEX8(0x01, sr.buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0x04, sr.buffer[1]);

    /* Write raw byte to chip 1 */
    syn_shiftreg_out_write_byte(&sr, 1, 0xAA);
    TEST_ASSERT_EQUAL_HEX8(0xAA, sr.buffer[1]);

    /* Flush out MSB-first */
    syn_shiftreg_out_set_bit_order(&sr, true);
    syn_shiftreg_out_flush(&sr);

    /* Flush out LSB-first */
    syn_shiftreg_out_set_bit_order(&sr, false);
    syn_shiftreg_out_flush(&sr);

    /* Test OE control */
    syn_shiftreg_out_set_enable(&sr, true);
    syn_shiftreg_out_set_enable(&sr, false);

    /* Clear bit 0 */
    syn_shiftreg_out_set_bit(&sr, 0, false);
    TEST_ASSERT_EQUAL_HEX8(0x00, sr.buffer[0]);

    /* NULL guards */
    syn_shiftreg_out_set_oe_pin(NULL, 0);
    syn_shiftreg_out_set_bit_order(NULL, true);
    syn_shiftreg_out_set_bit(NULL, 0, true);
    syn_shiftreg_out_set_bit(&sr, 999, true); /* Out of bounds */
    syn_shiftreg_out_write_byte(NULL, 0, 0);
    syn_shiftreg_out_write_byte(&sr, 99, 0); /* Out of bounds */
    syn_shiftreg_out_flush(NULL);
    syn_shiftreg_out_set_enable(NULL, true);
}

static void test_shiftreg_in_operations(void)
{
    mock_port_reset();
    SYN_ShiftRegIn sr;

    SYN_Status st = syn_shiftreg_in_init(&sr, 1, 2, 3, 2); /* 2 chips = 16 bits */
    TEST_ASSERT_EQUAL(SYN_OK, st);

    /* Mock data pin reading HIGH */
    mock_gpio_states[1] = 1;

    /* Read MSB-first */
    syn_shiftreg_in_set_bit_order(&sr, true);
    syn_shiftreg_in_read(&sr);
    TEST_ASSERT_EQUAL_HEX8(0xFF, syn_shiftreg_in_get_byte(&sr, 0));
    TEST_ASSERT_TRUE(syn_shiftreg_in_get_bit(&sr, 0));

    /* Read LSB-first */
    syn_shiftreg_in_set_bit_order(&sr, false);
    syn_shiftreg_in_read(&sr);
    TEST_ASSERT_EQUAL_HEX8(0xFF, syn_shiftreg_in_get_byte(&sr, 0));

    /* Mock data pin reading LOW */
    mock_gpio_states[1] = 0;
    syn_shiftreg_in_read(&sr);
    TEST_ASSERT_EQUAL_HEX8(0x00, syn_shiftreg_in_get_byte(&sr, 0));
    TEST_ASSERT_FALSE(syn_shiftreg_in_get_bit(&sr, 0));

    /* NULL guards */
    syn_shiftreg_in_set_bit_order(NULL, true);
    syn_shiftreg_in_read(NULL);
    TEST_ASSERT_FALSE(syn_shiftreg_in_get_bit(NULL, 0));
    TEST_ASSERT_FALSE(syn_shiftreg_in_get_bit(&sr, 999)); /* Out of bounds */
    TEST_ASSERT_EQUAL_HEX8(0, syn_shiftreg_in_get_byte(NULL, 0));
    TEST_ASSERT_EQUAL_HEX8(0, syn_shiftreg_in_get_byte(&sr, 99)); /* Out of bounds */
}

void run_shiftreg_tests(void)
{
    RUN_TEST(test_shiftreg_out_operations);
    RUN_TEST(test_shiftreg_in_operations);
}
