/**
 * @file test_arithmetic.c
 * @brief WASM test source for integer and floating point math.
 */

static int t = 0;

static int fail(void)
{
    return t;
}

int run_tests(void)
{
    /* 1. i32 basics */
    t++;
    if ((3 + 4) != 7)
        return fail();
    t++;
    if ((10 - 3) != 7)
        return fail();
    t++;
    if ((3 * 4) != 12)
        return fail();
    t++;
    if ((12 / 4) != 3)
        return fail();
    t++;
    if ((17 % 5) != 2)
        return fail();

    /* 2. i64 basics */
    t++;
    {
        long long a = 10000000000LL, b = 20000000000LL;
        if ((a + b) != 30000000000LL)
            return fail();
    }
    t++;
    {
        long long a = 30000000000LL, b = 10000000000LL;
        if ((a - b) != 20000000000LL)
            return fail();
    }
    t++;
    {
        long long a = 1000000LL, b = 2000000LL;
        if ((a * b) != 2000000000000LL)
            return fail();
    }

    /* 3. float (f32) math */
    t++;
    if ((3.0f + 4.0f) != 7.0f)
        return fail();
    t++;
    if ((10.0f - 3.0f) != 7.0f)
        return fail();
    t++;
    if ((3.0f * 4.0f) != 12.0f)
        return fail();
    t++;
    if ((12.0f / 4.0f) != 3.0f)
        return fail();

    /* 4. double (f64) math */
    t++;
    if ((3.0 + 4.0) != 7.0)
        return fail();
    t++;
    if ((10.0 - 3.0) != 7.0)
        return fail();
    t++;
    if ((3.0 * 4.0) != 12.0)
        return fail();
    t++;
    if ((12.0 / 4.0) != 3.0)
        return fail();

    return 0;
}
