/**
 * @file syn_wasm.h
 * @brief [EXPERIMENTAL] Zero-heap, 32-bit WebAssembly (Wasm MVP) cooperative interpreter.
 *
 * @warning EXPERIMENTAL MODULE - API and bytecode execution semantics subject to change.
 *
 * Implements a 100% zero-malloc, non-blocking 32-bit WebAssembly MVP interpreter
 * designed for embedded microcontrollers. Supports static linear memory arrays,
 * zero-copy parsing from flash, host function registration, and instruction-sliced
 * cooperative execution with protothread (`syn_pt`) integration.
 *
 * Recommended C-to-WASM compilation command for user applications:
 *   clang --target=wasm32-unknown-unknown -O2 -nostdlib \
 *     -mbulk-memory -msign-ext -mmultivalue -mnontrapping-fptoint \
 *     -Wl,--no-entry -Wl,--export-all -Wl,--allow-undefined \
 *     -Wl,-z,stack-size=1024 app.c -o app.wasm
 */

#ifndef SYN_WASM_H
#define SYN_WASM_H

#include "../common/syn_defs.h"
#include "../util/syn_qmath.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Configuration Defaults ──────────────────────────────────────────────── */

#ifndef SYN_WASM_MAX_STACK
#define SYN_WASM_MAX_STACK 64
#endif

#ifndef SYN_WASM_MAX_LOCALS
#define SYN_WASM_MAX_LOCALS 256
#endif

#ifndef SYN_WASM_MAX_CALL_DEPTH
#define SYN_WASM_MAX_CALL_DEPTH 32
#endif

#ifndef SYN_WASM_MAX_FUNCTIONS
#define SYN_WASM_MAX_FUNCTIONS 32
#endif

#ifndef SYN_WASM_MAX_HOST_FUNCS
#define SYN_WASM_MAX_HOST_FUNCS 16
#endif

#ifndef SYN_WASM_MAX_GLOBALS
#define SYN_WASM_MAX_GLOBALS 16
#endif

#ifndef SYN_WASM_MAX_LABELS
#define SYN_WASM_MAX_LABELS 16
#endif

/* ── Status Codes & Traps ───────────────────────────────────────────────── */

typedef enum {
    SYN_WASM_OK = 0,
    SYN_WASM_YIELDED,
    SYN_WASM_HALTED,
    SYN_WASM_TRAP_STACK_OVERFLOW,
    SYN_WASM_TRAP_STACK_UNDERFLOW,
    SYN_WASM_TRAP_OUT_OF_BOUNDS,
    SYN_WASM_TRAP_BAD_OPCODE,
    SYN_WASM_TRAP_DIV_ZERO,
    SYN_WASM_TRAP_UNREACHABLE,
    SYN_WASM_TRAP_CALL_STACK_OVERFLOW,
    SYN_WASM_TRAP_TYPE_MISMATCH,
    SYN_WASM_TRAP_INVALID_MODULE,
    SYN_WASM_TRAP_UNREGISTERED_HOST
} SYN_WASM_Status;

/* Forward declarations */
typedef struct SYN_WASM_Context_s SYN_WASM_Context;

/**
 * @brief Host function signature callable from Wasm bytecode.
 * @param ctx Pointer to Wasm execution context.
 * @param args Array of 32-bit arguments passed from Wasm stack.
 * @param argc Number of arguments.
 * @return 32-bit return value (pushed onto Wasm stack).
 */
typedef uint64_t (*SYN_WASM_HostFunc)(SYN_WASM_Context *ctx, const uint64_t *args, uint8_t argc);

/* ── Module Metadata (Zero-Heap Flash References) ────────────────────────── */

typedef struct {
    uint32_t type_idx;
    uint32_t code_offset;
    uint32_t code_size;
    uint8_t param_count;
    uint8_t result_count;
} SYN_WASM_FuncDef;

typedef struct {
    const uint8_t *bytes;
    uint32_t size;

    SYN_WASM_FuncDef funcs[SYN_WASM_MAX_FUNCTIONS];
    uint16_t func_count;
    uint16_t import_func_count;

    struct {
        uint32_t name_offset;
        uint16_t name_len;
        uint16_t func_idx;
    } exports[SYN_WASM_MAX_FUNCTIONS];
    uint16_t export_count;

    uint16_t table_elements[64];
    uint16_t table_element_count;

    uint32_t start_func_idx;
    bool has_start_func;
} SYN_WASM_Module;

/* ── Runtime Call Frame & Label Stacks ───────────────────────────────────── */

typedef struct {
    uint16_t func_idx;
    uint32_t return_pc;
    uint32_t frame_sp;
    uint16_t local_base;
} SYN_WASM_CallFrame;

typedef struct {
    uint8_t opcode; /* 0x02 block, 0x03 loop, 0x04 if */
    uint32_t target_pc;
    uint32_t stack_sp;
} SYN_WASM_Label;

/* ── Runtime Execution Context ────────────────────────────────────────────── */

struct SYN_WASM_Context_s {
    const SYN_WASM_Module *module;

    uint32_t pc;
    uint32_t sp;
    uint64_t stack[SYN_WASM_MAX_STACK];

    uint64_t locals[SYN_WASM_MAX_LOCALS];
    uint16_t local_count;

    SYN_WASM_CallFrame call_stack[SYN_WASM_MAX_CALL_DEPTH];
    uint8_t call_depth;

    SYN_WASM_Label label_stack[SYN_WASM_MAX_LABELS];
    uint8_t label_depth;

    uint64_t globals[SYN_WASM_MAX_GLOBALS];
    uint16_t global_count;

    uint8_t *linear_mem;
    uint32_t linear_mem_size;

    SYN_WASM_HostFunc host_funcs[SYN_WASM_MAX_HOST_FUNCS];
    uint16_t host_func_count;

    void *user_ctx;
    SYN_WASM_Status status;
};

/* ── Public API ─────────────────────────────────────────────────────────── */

/**
 * @brief Parse Wasm binary module from flash buffer (zero memory allocation).
 * @param mod Pointer to module descriptor to populate.
 * @param bytes Pointer to flash buffer containing .wasm binary.
 * @param size Length of wasm binary buffer.
 * @return true if valid Wasm MVP binary module parsed successfully.
 */
bool syn_wasm_module_load(SYN_WASM_Module *mod, const uint8_t *bytes, uint32_t size);

/**
 * @brief Initialize execution context for loaded module.
 * @param ctx Pointer to context structure.
 * @param mod Pointer to loaded module descriptor.
 * @param linear_mem Pointer to statically-allocated linear memory byte array.
 * @param mem_size Size of linear memory buffer in bytes.
 * @return true if initialized successfully.
 */
bool syn_wasm_init(SYN_WASM_Context *ctx, const SYN_WASM_Module *mod, uint8_t *linear_mem,
                   uint32_t mem_size);

/**
 * @brief Register C host function for imported Wasm functions.
 * @param ctx Pointer to context.
 * @param import_index Index of import function slot (0 .. import_func_count - 1).
 * @param func Function pointer to host C handler.
 * @return true if registered successfully.
 */
bool syn_wasm_register_host(SYN_WASM_Context *ctx, uint16_t import_index, SYN_WASM_HostFunc func);

/**
 * @brief Find exported function index by name.
 * @param mod Pointer to loaded module.
 * @param name Exported function name string.
 * @return Function index if found, or -1 if not found.
 */
int32_t syn_wasm_find_export(const SYN_WASM_Module *mod, const char *name);

/**
 * @brief Prepare context to call a function index.
 * @param ctx Pointer to context.
 * @param func_index Function index to invoke.
 * @return true if call frame initialized successfully.
 */
bool syn_wasm_call(SYN_WASM_Context *ctx, uint16_t func_index);

/**
 * @brief Execute instruction slice non-blockingly.
 * @param ctx Pointer to context.
 * @param max_instructions Maximum opcodes to process before yielding.
 * @return SYN_WASM_OK (completed), SYN_WASM_YIELDED (time slice expired), SYN_WASM_HALTED
 * (finished), or TRAP.
 */
SYN_WASM_Status syn_wasm_step(SYN_WASM_Context *ctx, uint16_t max_instructions);

/**
 * @brief Read top of evaluation stack result after HALTED.
 * @param ctx Pointer to context.
 * @return 32-bit return value.
 */
uint64_t syn_wasm_result(const SYN_WASM_Context *ctx);

#ifdef __cplusplus
}
#endif

#endif /* SYN_WASM_H */
