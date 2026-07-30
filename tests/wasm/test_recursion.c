/**
 * @file test_recursion.c
 * @brief WASM test source for recursive function call stack frames.
 */

static int t = 0;

static int fail(void)
{
    return t;
}

int count_down(int n)
{
    if (n <= 0)
        return 0;
    return 1 + count_down(n - 1);
}

int run_tests(void)
{
    /* 1. Recursion stack frame check */
    t++;
    if (count_down(5) != 5)
        return fail();

    return 0;
}
