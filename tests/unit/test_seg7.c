/**
 * @file test_seg7.c
 * @brief Unity tests for syn_seg7 module.
 */

#include "mocks/mock_port.h"
#include "syntropic/display/syn_seg7.h"
#include "unity/unity.h"

static void test_seg7_int_and_scan(void)
{
    mock_port_reset();
    SYN_Seg7 seg;

    SYN_GPIO_Pin segment_pins[] = {0, 1, 2, 3, 4, 5, 6, 7};
    SYN_GPIO_Pin digit_pins[]   = {8, 9, 10, 11};

    SYN_Status st = syn_seg7_init(&seg, segment_pins, digit_pins, 4, SYN_SEG7_COMMON_CATHODE);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    /* Print integer 1234 */
    syn_seg7_print_int(&seg, 1234);
    TEST_ASSERT_EQUAL_HEX8(0x06, seg.digit_buffers[0]); /* '1' */
    TEST_ASSERT_EQUAL_HEX8(0x5B, seg.digit_buffers[1]); /* '2' */
    TEST_ASSERT_EQUAL_HEX8(0x4F, seg.digit_buffers[2]); /* '3' */
    TEST_ASSERT_EQUAL_HEX8(0x66, seg.digit_buffers[3]); /* '4' */

    /* Scan next digit */
    syn_seg7_scan(&seg);
    TEST_ASSERT_EQUAL_UINT8(1, seg.active_digit);

    /* Test integer overflow (5 digits on 4-digit display -> dashes ----) */
    syn_seg7_print_int(&seg, 12345);
    TEST_ASSERT_EQUAL_HEX8(0x40, seg.digit_buffers[0]);
    TEST_ASSERT_EQUAL_HEX8(0x40, seg.digit_buffers[3]);
}

static void test_seg7_float(void)
{
    mock_port_reset();
    SYN_Seg7 seg;

    SYN_GPIO_Pin segment_pins[] = {0, 1, 2, 3, 4, 5, 6, 7};
    SYN_GPIO_Pin digit_pins[]   = {8, 9, 10, 11};

    syn_seg7_init(&seg, segment_pins, digit_pins, 4, SYN_SEG7_COMMON_CATHODE);

    /* Print float 12.3 */
    syn_seg7_print_float(&seg, 12.3f, 1);
    TEST_ASSERT_EQUAL_HEX8(0x06, seg.digit_buffers[0]); /* '1' */
    TEST_ASSERT_EQUAL_HEX8(0x5B | 0x80, seg.digit_buffers[1]); /* '2.' */
    TEST_ASSERT_EQUAL_HEX8(0x4F, seg.digit_buffers[2]); /* '3' */
}

static void test_seg7_hex_string_and_anode(void)
{
    mock_port_reset();
    SYN_Seg7 seg;

    SYN_GPIO_Pin segment_pins[] = {0, 1, 2, 3, 4, 5, 6, 7};
    SYN_GPIO_Pin digit_pins[]   = {8, 9, 10, 11};

    /* Common Anode wiring test */
    syn_seg7_init(&seg, segment_pins, digit_pins, 4, SYN_SEG7_COMMON_ANODE);

    /* Print hex 0xABCDEF */
    syn_seg7_print_hex(&seg, 0xABCDEF);
    TEST_ASSERT_EQUAL_HEX8(0x77, seg.digit_buffers[0]); /* 'A' */
    TEST_ASSERT_EQUAL_HEX8(0x7C, seg.digit_buffers[1]); /* 'b' */
    TEST_ASSERT_EQUAL_HEX8(0x39, seg.digit_buffers[2]); /* 'C' */
    TEST_ASSERT_EQUAL_HEX8(0x5E, seg.digit_buffers[3]); /* 'd' */

    /* Scan in Common Anode mode */
    syn_seg7_scan(&seg);
    TEST_ASSERT_EQUAL_UINT8(1, seg.active_digit);

    /* Test all font glyphs */
    syn_seg7_print_str(&seg, "0567");
    TEST_ASSERT_EQUAL_HEX8(0x3F, seg.digit_buffers[0]); /* '0' */
    TEST_ASSERT_EQUAL_HEX8(0x6D, seg.digit_buffers[1]); /* '5' */
    TEST_ASSERT_EQUAL_HEX8(0x7D, seg.digit_buffers[2]); /* '6' */
    TEST_ASSERT_EQUAL_HEX8(0x07, seg.digit_buffers[3]); /* '7' */

    syn_seg7_print_str(&seg, "89FU");
    TEST_ASSERT_EQUAL_HEX8(0x7F, seg.digit_buffers[0]); /* '8' */
    TEST_ASSERT_EQUAL_HEX8(0x6F, seg.digit_buffers[1]); /* '9' */
    TEST_ASSERT_EQUAL_HEX8(0x71, seg.digit_buffers[2]); /* 'F' */
    TEST_ASSERT_EQUAL_HEX8(0x3E, seg.digit_buffers[3]); /* 'U' */

    syn_seg7_print_str(&seg, "ehlp");
    TEST_ASSERT_EQUAL_HEX8(0x79, seg.digit_buffers[0]); /* 'e' */
    TEST_ASSERT_EQUAL_HEX8(0x76, seg.digit_buffers[1]); /* 'h' */
    TEST_ASSERT_EQUAL_HEX8(0x38, seg.digit_buffers[2]); /* 'l' */
    TEST_ASSERT_EQUAL_HEX8(0x73, seg.digit_buffers[3]); /* 'p' */

    syn_seg7_print_str(&seg, "A.B?");
    TEST_ASSERT_EQUAL_HEX8(0x77 | 0x80, seg.digit_buffers[0]); /* 'A.' */
    TEST_ASSERT_EQUAL_HEX8(0x7C, seg.digit_buffers[1]); /* 'B' */
    TEST_ASSERT_EQUAL_HEX8(0x00, seg.digit_buffers[2]); /* '?' */

    syn_seg7_print_str(&seg, ".-_ ");
    TEST_ASSERT_EQUAL_HEX8(0x40, seg.digit_buffers[0]); /* '-' */
    TEST_ASSERT_EQUAL_HEX8(0x08, seg.digit_buffers[1]); /* '_' */
    TEST_ASSERT_EQUAL_HEX8(0x00, seg.digit_buffers[2]); /* ' ' */

    /* Raw digit setter (valid and out-of-bounds) */
    syn_seg7_set_digit_raw(&seg, 0, 0xFF);
    TEST_ASSERT_EQUAL_HEX8(0xFF, seg.digit_buffers[0]);
    syn_seg7_set_digit_raw(&seg, 10, 0xFF); /* OOB: ignored */

    /* NULL safety guards */
    syn_seg7_scan(NULL);
    syn_seg7_clear(NULL);
    syn_seg7_set_digit_raw(NULL, 0, 0);
    syn_seg7_print_int(NULL, 0);
    syn_seg7_print_float(NULL, 0.0f, 0);
    syn_seg7_print_hex(NULL, 0);
    syn_seg7_print_str(NULL, NULL);
}

void run_seg7_tests(void)
{
    RUN_TEST(test_seg7_int_and_scan);
    RUN_TEST(test_seg7_float);
    RUN_TEST(test_seg7_hex_string_and_anode);
}
