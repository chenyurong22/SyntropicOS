/**
 * @file test_wasm.c
 * @brief Unit tests for 32-bit WebAssembly (Wasm MVP) cooperative interpreter.
 */

#include "syntropic/vm/syn_wasm.h"
#include "unity/unity.h"

#include <string.h>

static uint8_t g_linear_mem[256];
static SYN_WASM_Module g_wasm_mod;
static SYN_WASM_Context g_wasm_ctx;

/* Sample valid minimal Wasm module with an export 'add' (i32.add) */
/* Binary generated from WebAssembly text:
 * (module
 *   (func (export "add") (param i32 i32) (result i32)
 *     local.get 0
 *     local.get 1
 *     i32.add)
 * )
 */
static const uint8_t g_wasm_add_binary[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,       /* magic & version */
    0x01, 0x07, 0x01, 0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f, /* Type: (i32, i32) -> i32 */
    0x03, 0x02, 0x01, 0x00,                               /* Func 0: type 0 */
    0x07, 0x07, 0x01, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00, /* Export "add" -> func 0 */
    0x0a, 0x09, 0x01, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01,
    0x6a, 0x0b /* Code: local.get 0, local.get 1, i32.add, end */
};

/* Sample Wasm module with loop counting down from N */
/*
 * (module
 *   (func (export "loop_count") (param i32) (result i32)
 *     (local i32)
 *     local.get 0
 *     local.set 1
 *     block
 *       loop
 *         local.get 1
 *         i32.eqz
 *         br_if 1
 *         local.get 1
 *         i32.const 1
 *         i32.sub
 *         local.set 1
 *         br 0
 *       end
 *     end
 *     local.get 1)
 * )
 */
static const uint8_t g_wasm_loop_binary[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x06, 0x01, 0x60, 0x01, 0x7f,
    0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07, 0x0e, 0x01, 0x0a, 0x6c, 0x6f, 0x6f, 0x70,
    0x5f, 0x63, 0x6f, 0x75, 0x6e, 0x74, 0x00, 0x00, 0x0a, 0x20, 0x01, 0x1e, 0x01, 0x01,
    0x7f, 0x20, 0x00, 0x21, 0x01, 0x02, 0x40, 0x03, 0x40, 0x20, 0x01, 0x45, 0x0d, 0x01,
    0x20, 0x01, 0x41, 0x01, 0x6b, 0x21, 0x01, 0x0c, 0x00, 0x0b, 0x0b, 0x20, 0x01, 0x0b};

/* Host function callback counter */
static uint32_t g_host_call_count = 0;
static uint32_t mock_host_func(SYN_WASM_Context *ctx, const uint32_t *args, uint8_t argc)
{
    (void)ctx;
    (void)args;
    (void)argc;
    g_host_call_count++;
    return 42;
}

static void test_wasm_load_null_and_invalid(void)
{
    TEST_ASSERT_FALSE(syn_wasm_module_load(NULL, g_wasm_add_binary, sizeof(g_wasm_add_binary)));
    TEST_ASSERT_FALSE(syn_wasm_module_load(&g_wasm_mod, NULL, sizeof(g_wasm_add_binary)));
    TEST_ASSERT_FALSE(syn_wasm_module_load(&g_wasm_mod, g_wasm_add_binary, 4));

    uint8_t bad_hdr[8] = {0x00, 0x61, 0x73, 0x6d, 0x02, 0x00, 0x00, 0x00};
    TEST_ASSERT_FALSE(syn_wasm_module_load(&g_wasm_mod, bad_hdr, sizeof(bad_hdr)));
}

static void test_wasm_load_and_export_lookup(void)
{
    TEST_ASSERT_TRUE(
        syn_wasm_module_load(&g_wasm_mod, g_wasm_add_binary, sizeof(g_wasm_add_binary)));
    TEST_ASSERT_EQUAL_UINT16(1, g_wasm_mod.export_count);

    int32_t add_idx = syn_wasm_find_export(&g_wasm_mod, "add");
    TEST_ASSERT_EQUAL_INT32(0, add_idx);

    int32_t missing_idx = syn_wasm_find_export(&g_wasm_mod, "nonexistent");
    TEST_ASSERT_EQUAL_INT32(-1, missing_idx);
}

static void test_wasm_init_and_execution(void)
{
    TEST_ASSERT_TRUE(
        syn_wasm_module_load(&g_wasm_mod, g_wasm_add_binary, sizeof(g_wasm_add_binary)));
    TEST_ASSERT_TRUE(syn_wasm_init(&g_wasm_ctx, &g_wasm_mod, g_linear_mem, sizeof(g_linear_mem)));

    TEST_ASSERT_TRUE(syn_wasm_call(&g_wasm_ctx, 0));

    /* Populate arguments into local variables 0 and 1 */
    g_wasm_ctx.locals[0] = 15;
    g_wasm_ctx.locals[1] = 27;

    SYN_WASM_Status st = syn_wasm_step(&g_wasm_ctx, 100);
    TEST_ASSERT_EQUAL(SYN_WASM_HALTED, st);
    TEST_ASSERT_EQUAL_UINT32(42, syn_wasm_result(&g_wasm_ctx));
}

static void test_wasm_loop_and_yielding(void)
{
    TEST_ASSERT_TRUE(
        syn_wasm_module_load(&g_wasm_mod, g_wasm_loop_binary, sizeof(g_wasm_loop_binary)));
    TEST_ASSERT_TRUE(syn_wasm_init(&g_wasm_ctx, &g_wasm_mod, g_linear_mem, sizeof(g_linear_mem)));

    TEST_ASSERT_TRUE(syn_wasm_call(&g_wasm_ctx, 0));
    g_wasm_ctx.locals[0] = 10; /* Loop 10 iterations */

    /* Step with small instruction budget (5 opcodes) -> Expect YIELDED */
    SYN_WASM_Status st = syn_wasm_step(&g_wasm_ctx, 5);
    TEST_ASSERT_EQUAL(SYN_WASM_YIELDED, st);

    /* Finish remaining execution */
    st = syn_wasm_step(&g_wasm_ctx, 1000);
    TEST_ASSERT_EQUAL(SYN_WASM_HALTED, st);
    TEST_ASSERT_EQUAL_UINT32(0, syn_wasm_result(&g_wasm_ctx));
}

static void test_wasm_host_function_registration(void)
{
    g_host_call_count = 0;
    TEST_ASSERT_TRUE(syn_wasm_init(&g_wasm_ctx, &g_wasm_mod, g_linear_mem, sizeof(g_linear_mem)));
    TEST_ASSERT_TRUE(syn_wasm_register_host(&g_wasm_ctx, 0, mock_host_func));

    /* Manually invoke host func registration slot 0 */
    uint32_t ret = g_wasm_ctx.host_funcs[0](&g_wasm_ctx, NULL, 0);
    TEST_ASSERT_EQUAL_UINT32(42, ret);
    TEST_ASSERT_EQUAL_UINT32(1, g_host_call_count);
}

void run_wasm_tests(void)
{
    RUN_TEST(test_wasm_load_null_and_invalid);
    RUN_TEST(test_wasm_load_and_export_lookup);
    RUN_TEST(test_wasm_init_and_execution);
    RUN_TEST(test_wasm_loop_and_yielding);
    RUN_TEST(test_wasm_host_function_registration);
}
