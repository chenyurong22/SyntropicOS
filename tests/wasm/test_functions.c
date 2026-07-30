/**
 * @file test_functions.c
 * @brief WASM test source for function calls and function pointer indirect calls.
 */

static int t = 0;

static int fail(void)
{
    return t;
}

static int add(int a, int b)
{
    return a + b;
}
static int sub(int a, int b)
{
    return a - b;
}

typedef int (*math_fn)(int, int);
static const math_fn g_ops[2] = {add, sub};

int run_tests(void)
{
    /* 1. Direct call */
    t++;
    if (add(15, 27) != 42)
        return fail();

    /* 2. Indirect call via function pointer table (call_indirect) */
    t++;
    if (g_ops[0](10, 20) != 30)
        return fail();
    t++;
    if (g_ops[1](50, 10) != 40)
        return fail();

    return 0;
}
