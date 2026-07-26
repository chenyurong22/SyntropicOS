/**
 * @file test_dipswitch.c
 * @brief Unity tests for syn_dipswitch module.
 */

#include "mocks/mock_port.h"
#include "syntropic/input/syn_dipswitch.h"
#include "unity/unity.h"

static void test_dipswitch_reading(void)
{
    mock_port_reset();
    SYN_DipSwitch ds;

    SYN_GPIO_Pin pins[] = {0, 1, 2, 3};

    /* Active HIGH dip switches */
    SYN_Status st = syn_dipswitch_init(&ds, pins, 4, false);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL_UINT32(0, syn_dipswitch_get_value(&ds));

    /* Turn ON switch 0 and 2 (bits 0 and 2 -> 0x05) */
    mock_gpio_states[0] = 1;
    mock_gpio_states[2] = 1;

    syn_dipswitch_read(&ds);
    TEST_ASSERT_TRUE(syn_dipswitch_has_changed(&ds));
    TEST_ASSERT_EQUAL_UINT32(0x05, syn_dipswitch_get_value(&ds));

    /* Read again without changes */
    syn_dipswitch_read(&ds);
    TEST_ASSERT_FALSE(syn_dipswitch_has_changed(&ds));
}

void run_dipswitch_tests(void)
{
    RUN_TEST(test_dipswitch_reading);
}
