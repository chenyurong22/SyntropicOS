/**
 * @file test_traps.c
 * @brief WASM test source for triggering VM traps safely.
 */

volatile int g_zero = 0;
volatile int *g_invalid_ptr = (int *)0x00FFFFFF;

int trigger_div_zero(void)
{
    return 42 / g_zero;
}

int trigger_oob_read(void)
{
    return *g_invalid_ptr;
}
