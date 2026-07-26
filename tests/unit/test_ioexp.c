/**
 * @file test_ioexp.c
 * @brief Unity tests for syn_ioexp module.
 */

#include "mocks/mock_port.h"
#include "syntropic/drivers/syn_ioexp.h"
#include "unity/unity.h"

static void test_ioexp_mcp23017_and_pcf8574(void)
{
    mock_port_reset();
    SYN_IOExp io;

    /* MCP23017 Test */
    SYN_Status st = syn_ioexp_init(&io, 0, 1, 0x20, SYN_IOEXP_MCP23017);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    syn_ioexp_set_pin_mode(&io, 0, SYN_GPIO_OUTPUT);
    syn_ioexp_set_pin_mode(&io, 1, SYN_GPIO_INPUT_PULLUP);
    syn_ioexp_write_pin(&io, 0, SYN_GPIO_HIGH);

    TEST_ASSERT_EQUAL(SYN_GPIO_HIGH, syn_ioexp_read_pin(&io, 0));

    /* MCP23008 Test */
    st = syn_ioexp_init(&io, 0, 1, 0x20, SYN_IOEXP_MCP23008);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    syn_ioexp_write_port(&io, 0x55);

    /* PCF8574 Test */
    st = syn_ioexp_init(&io, 0, 1, 0x27, SYN_IOEXP_PCF8574);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    syn_ioexp_write_port(&io, 0xAA);

    /* TCA9555 Test */
    st = syn_ioexp_init(&io, 0, 1, 0x20, SYN_IOEXP_TCA9555);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    syn_ioexp_write_port(&io, 0x1234);

    /* NULL guards */
    syn_ioexp_set_pin_mode(NULL, 0, SYN_GPIO_INPUT);
    syn_ioexp_write_pin(NULL, 0, SYN_GPIO_HIGH);
    TEST_ASSERT_EQUAL(SYN_GPIO_LOW, syn_ioexp_read_pin(NULL, 0));
    syn_ioexp_write_port(NULL, 0);
    TEST_ASSERT_EQUAL_UINT16(0, syn_ioexp_read_port(NULL));
}

void run_ioexp_tests(void)
{
    RUN_TEST(test_ioexp_mcp23017_and_pcf8574);
}
