/**
 * @file test_conversions.c
 * @brief WASM test source for type casting, sign extension, and float truncations.
 */

static int t = 0;

static int fail(void)
{
    return t;
}

int run_tests(void)
{
    /* 1. Integer sign extension (extend8_s, extend16_s) */
    t++;
    signed char sc = -50;
    int sc_ext = (int)sc;
    if (sc_ext != -50)
        return fail();

    t++;
    short ss = -12345;
    int ss_ext = (int)ss;
    if (ss_ext != -12345)
        return fail();

    /* 2. Float to int truncation (trunc_sat) */
    t++;
    float f = 42.9f;
    int f_int = (int)f;
    if (f_int != 42)
        return fail();

    t++;
    double d = -99.8;
    int d_int = (int)d;
    if (d_int != -99)
        return fail();

    /* 3. Int to float conversion */
    t++;
    int val = 123456;
    float val_f = (float)val;
    if (val_f != 123456.0f)
        return fail();

    return 0;
}
