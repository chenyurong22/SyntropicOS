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

#define MOCK_GPIOA_BASE 0x40020000UL
#define MOCK_GPIOH_BASE (MOCK_GPIOA_BASE + (7UL * 0x0400UL))
#define MOCK_GPIOA ((void *)MOCK_GPIOA_BASE)
#define MOCK_GPIOH ((void *)MOCK_GPIOH_BASE)
#define GPIOA MOCK_GPIOA

#include "port/stm32_hal/port_stm32_hal.h"

#define MOCK_GPIO_PIN_0 0x0001U
#define MOCK_GPIO_PIN_1 0x0002U
#define MOCK_GPIO_PIN_2 0x0004U
#define MOCK_GPIO_PIN_3 0x0008U

static void test_stm32_hal_pin_macro(void)
{
    /* Comprehensive exhaust test: 9 ports (GPIOA..GPIOI), 16 pins (0..15) */
    for (uint8_t port_idx = 0; port_idx < 9; port_idx++) {
        uintptr_t port_addr = MOCK_GPIOA_BASE + ((uintptr_t)port_idx * 0x0400UL);
        void *port_ptr = (void *)port_addr;

        for (uint8_t pin_num = 0; pin_num < 16; pin_num++) {
            uint32_t hal_pin_mask = (1U << pin_num);
            uint16_t expected_handle = ((uint16_t)port_idx << 4) | pin_num;

            uint16_t actual_handle = SYN_PORT_STM32_PIN(port_ptr, hal_pin_mask);
            TEST_ASSERT_EQUAL_UINT16(expected_handle, actual_handle);
        }
    }
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
    RUN_TEST(test_stm32_hal_pin_macro);
}
