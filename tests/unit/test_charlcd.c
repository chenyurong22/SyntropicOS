/**
 * @file test_charlcd.c
 * @brief Unity tests for syn_charlcd module.
 */

#include "mocks/mock_port.h"
#include "syntropic/display/syn_charlcd.h"
#include "unity/unity.h"

static void test_charlcd_i2c_mode(void)
{
    mock_port_reset();
    SYN_CharLCD lcd;

    SYN_Status st = syn_charlcd_init_i2c(&lcd, 0, 1, 0x27, 16, 2);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    syn_charlcd_set_cursor(&lcd, 2, 1);
    syn_charlcd_print(&lcd, "I2C Test");
    syn_charlcd_set_backlight(&lcd, true);
    syn_charlcd_set_backlight(&lcd, false);

    uint8_t glyph[8] = {0x04, 0x0E, 0x1F, 0x04, 0x04, 0x04, 0x04, 0x00};
    syn_charlcd_create_char(&lcd, 0, glyph);
    syn_charlcd_clear(&lcd);

    /* NULL safety guards */
    syn_charlcd_clear(NULL);
    syn_charlcd_set_cursor(NULL, 0, 0);
    syn_charlcd_print(NULL, NULL);
    syn_charlcd_set_backlight(NULL, true);
    syn_charlcd_create_char(NULL, 0, NULL);
}

static void test_charlcd_gpio_mode(void)
{
    mock_port_reset();
    SYN_CharLCD lcd;

    SYN_Status st = syn_charlcd_init_gpio(&lcd, 0, 1, 2, 3, 4, 5, 20, 4);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    syn_charlcd_set_cursor(&lcd, 5, 2);
    syn_charlcd_print(&lcd, "GPIO Mode");
    syn_charlcd_clear(&lcd);
}

void run_charlcd_tests(void)
{
    RUN_TEST(test_charlcd_i2c_mode);
    RUN_TEST(test_charlcd_gpio_mode);
}
