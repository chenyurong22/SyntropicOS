#include "mocks/mock_port.h"
#include "syntropic/drivers/syn_gpio.h"
#include "unity/unity.h"

/** init_multiple: success path */
static void test_gpio_init_multiple(void)
{
    SYN_GPIO_Pin pins[] = {0, 1, 2};
    SYN_Status st = syn_gpio_init_multiple(pins, 3, SYN_GPIO_OUTPUT);
    TEST_ASSERT_EQUAL(SYN_OK, st);
}

/** init_multiple: count=0 */
static void test_gpio_init_multiple_zero(void)
{
    SYN_Status st = syn_gpio_init_multiple(NULL, 0, SYN_GPIO_OUTPUT);
    TEST_ASSERT_EQUAL(SYN_OK, st);
}

/** init_multiple: failure on second pin (pin >= 32 triggers error) */
static void test_gpio_init_multiple_fail(void)
{
    SYN_GPIO_Pin pins[] = {0, 99}; /* 99 >= 32 → SYN_INVALID_PARAM */
    SYN_Status st = syn_gpio_init_multiple(pins, 2, SYN_GPIO_OUTPUT);
    TEST_ASSERT_NOT_EQUAL(SYN_OK, st);
}

/** write_multiple: success path */
static void test_gpio_write_multiple(void)
{
    SYN_GPIO_Pin pins[] = {0, 1};
    SYN_Status st = syn_gpio_write_multiple(pins, 2, SYN_GPIO_HIGH);
    TEST_ASSERT_EQUAL(SYN_OK, st);
}

/** write_multiple: count=0 */
static void test_gpio_write_multiple_zero(void)
{
    SYN_Status st = syn_gpio_write_multiple(NULL, 0, SYN_GPIO_HIGH);
    TEST_ASSERT_EQUAL(SYN_OK, st);
}

/** write_multiple: failure on bad pin */
static void test_gpio_write_multiple_fail(void)
{
    SYN_GPIO_Pin pins[] = {0, 99};
    SYN_Status st = syn_gpio_write_multiple(pins, 2, SYN_GPIO_HIGH);
    TEST_ASSERT_NOT_EQUAL(SYN_OK, st);
}

/** single pin ops via port layer */
static void test_gpio_single_ops(void)
{
    mock_port_reset();

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gpio_init(5, SYN_GPIO_OUTPUT));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gpio_write(5, SYN_GPIO_HIGH));
    TEST_ASSERT_EQUAL_INT(SYN_GPIO_HIGH, syn_gpio_read(5));

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gpio_toggle(5));
    TEST_ASSERT_EQUAL_INT(SYN_GPIO_LOW, syn_gpio_read(5));

    /* Invalid pin check */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_gpio_init(99, SYN_GPIO_INPUT));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_gpio_write(99, SYN_GPIO_HIGH));
    TEST_ASSERT_EQUAL_INT(SYN_GPIO_LOW, syn_gpio_read(99));
}

/** null pins with count > 0 */
static void test_gpio_null_pins(void)
{
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_gpio_init_multiple(NULL, 5, SYN_GPIO_OUTPUT));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_gpio_write_multiple(NULL, 5, SYN_GPIO_HIGH));
}

void run_gpio_tests(void)
{
    RUN_TEST(test_gpio_init_multiple);
    RUN_TEST(test_gpio_init_multiple_zero);
    RUN_TEST(test_gpio_init_multiple_fail);
    RUN_TEST(test_gpio_write_multiple);
    RUN_TEST(test_gpio_write_multiple_zero);
    RUN_TEST(test_gpio_write_multiple_fail);
    RUN_TEST(test_gpio_single_ops);
    RUN_TEST(test_gpio_null_pins);
}
