/**
 * @file test_control_flow.c
 * @brief WASM test source for switch (br_table), nested loops, and conditionals.
 */

static int t = 0;

static int fail(void)
{
    return t;
}

static int eval_switch(int val)
{
    switch (val) {
    case 0:
        return 100;
    case 1:
        return 200;
    case 2:
        return 300;
    case 3:
        return 400;
    default:
        return -1;
    }
}

int run_tests(void)
{
    /* 1. Switch statement (br_table) */
    t++;
    if (eval_switch(0) != 100)
        return fail();
    t++;
    if (eval_switch(2) != 300)
        return fail();
    t++;
    if (eval_switch(99) != -1)
        return fail();

    /* 2. Nested loop with break/continue */
    t++;
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            if (j == 3)
                break;
            if (i == 2)
                continue;
            sum += i + j;
        }
    }
    if (sum != 36)
        return fail();

    /* 3. Short-circuit logic */
    t++;
    int side_effect = 0;
    if (0 && (++side_effect))
        return fail();
    if (side_effect != 0)
        return fail();

    return 0;
}
