/**
 * @file test_keypad.c
 * @brief Unity tests for syn_keypad module.
 */

#include "mocks/mock_port.h"
#include "syntropic/input/syn_keypad.h"
#include "unity/unity.h"

static void dummy_keypad_cb(SYN_Keypad *kp, char key, bool pressed, void *ctx)
{
    (void)kp;
    (void)key;
    (void)pressed;
    uint32_t *count = (uint32_t *)ctx;
    if (count)
        (*count)++;
}

static void test_keypad_scan(void)
{
    mock_port_reset();
    SYN_Keypad kp;

    SYN_GPIO_Pin rows[] = {0, 1};
    SYN_GPIO_Pin cols[] = {2, 3};
    const char keymap[] = "1234";

    SYN_Status st = syn_keypad_init(&kp, rows, 2, cols, 2, keymap);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    uint32_t events = 0;
    syn_keypad_set_callback(&kp, dummy_keypad_cb, &events);

    /* Initial scan - no key pressed */
    syn_keypad_scan(&kp);
    char k = 0;
    TEST_ASSERT_FALSE(syn_keypad_get_key(&kp, &k));

    /* Simulate row 0 col 0 pressed (col pin 2) */
    mock_gpio_states[2] = 1;
    syn_keypad_scan(&kp);

    TEST_ASSERT_TRUE(syn_keypad_get_key(&kp, &k));
    TEST_ASSERT_EQUAL_INT('1', k);
    TEST_ASSERT_EQUAL_UINT32(1, events);

    /* Release key */
    mock_gpio_states[2] = 0;
    syn_keypad_scan(&kp);
    TEST_ASSERT_FALSE(syn_keypad_get_key(&kp, &k));
    TEST_ASSERT_EQUAL_UINT32(2, events);
}

void run_keypad_tests(void)
{
    RUN_TEST(test_keypad_scan);
}
