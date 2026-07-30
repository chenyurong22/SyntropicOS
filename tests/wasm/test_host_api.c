/**
 * @file test_host_api.c
 * @brief WASM test source for host imports and global variable state.
 */

extern int host_log(int code, int val);

static int g_counter = 100;

int get_counter(void)
{
    return g_counter;
}

void inc_counter(int delta)
{
    g_counter += delta;
}

int run_tests(void)
{
    g_counter = 100;
    inc_counter(50);
    if (get_counter() != 150)
        return 1;

    int res = host_log(42, g_counter);
    if (res != 192)
        return 2; /* Expected host return: 42 + 150 = 192 */

    return 0;
}
