/**
 * @file test_memory.c
 * @brief WASM test source for memory ops, data section, and bulk memory.
 */

static int t = 0;

static int fail(void)
{
    return t;
}

static const char g_msg[] = "SYN TROPIC";
static char g_buffer[64];

int run_tests(void)
{
    /* 1. Data section initialization check */
    t++;
    if (g_msg[0] != 'S' || g_msg[4] != 'T')
        return fail();

    /* 2. Bulk memory memcpy (0xFC memory.copy) */
    t++;
    __builtin_memcpy(g_buffer, g_msg, 10);
    t++;
    if (g_buffer[0] != 'S' || g_buffer[9] != 'C')
        return fail();

    /* 3. Bulk memory memset (0xFC memory.fill) */
    t++;
    __builtin_memset(g_buffer, 'X', 10);
    t++;
    if (g_buffer[0] != 'X' || g_buffer[9] != 'X')
        return fail();

    return 0;
}
