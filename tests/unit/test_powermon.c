/**
 * @file test_powermon.c
 * @brief Unity tests for syn_powermon module.
 */

#include "mocks/mock_port.h"
#include "syntropic/sensor/syn_powermon.h"
#include "unity/unity.h"

static void test_powermon_operations(void)
{
    mock_port_reset();
    SYN_PowerMon pm;

    /* INA219 Test */
    SYN_Status st = syn_powermon_init(&pm, 0, 1, 0x40, 0.1f, SYN_POWERMON_INA219);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    syn_powermon_feed_raw(&pm, 3000 << 3, 10.0f); /* 12V bus, 10mV shunt = 100mA current */
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 12.0f, syn_powermon_get_bus_voltage(&pm));
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, syn_powermon_get_current_ma(&pm));
    TEST_ASSERT_FLOAT_WITHIN(10.0f, 1200.0f, syn_powermon_get_power_mw(&pm));

    /* INA226 Test */
    st = syn_powermon_init(&pm, 0, 1, 0x40, 0.1f, SYN_POWERMON_INA226);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    syn_powermon_feed_raw(&pm, 9600, 10.0f); /* 12V bus (9600 * 1.25mV) */
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 12.0f, syn_powermon_get_bus_voltage(&pm));

    /* Zero shunt resistor edge case */
    pm.shunt_resistor_ohms = 0.0f;
    syn_powermon_feed_raw(&pm, 1000, 5.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, syn_powermon_get_current_ma(&pm));

    /* NULL guards */
    syn_powermon_feed_raw(NULL, 0, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, syn_powermon_get_bus_voltage(NULL));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, syn_powermon_get_current_ma(NULL));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, syn_powermon_get_power_mw(NULL));
}

void run_powermon_tests(void)
{
    RUN_TEST(test_powermon_operations);
}
