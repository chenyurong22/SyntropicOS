/**
 * @file test_embedded.c
 * @brief WASM test source verifying small stack allocation (-Wl,-z,stack-size=1024) in 4KB linear
 * memory.
 */

static int t = 0;

static int fail(void)
{
    return t;
}

int run_tests(void)
{
    int arr[64]; /* Stack allocation */
    for (int i = 0; i < 64; i++) {
        arr[i] = i * 2;
    }

    t++;
    int sum = 0;
    for (int i = 0; i < 64; i++) {
        sum += arr[i];
    }

    if (sum != 4032)
        return fail();

    return 0;
}
