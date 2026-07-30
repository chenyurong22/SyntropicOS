/**
 * @file syn_wasm.c
 * @brief Implementation of 32-bit WebAssembly (Wasm MVP) cooperative interpreter.
 */

#include "syn_wasm.h"

#include <string.h>

/* Wasm Magic Header & Version */
#define WASM_MAGIC 0x6D736100U /* "\0asm" */
#define WASM_VERSION 0x00000001U

/* Value Types */
#define WASM_TYPE_I32 0x7FU
#define WASM_TYPE_I64 0x7EU
#define WASM_TYPE_F32 0x7DU
#define WASM_TYPE_F64 0x7CU

/* Section IDs */
#define WASM_SEC_TYPE 1
#define WASM_SEC_IMPORT 2
#define WASM_SEC_FUNCTION 3
#define WASM_SEC_GLOBAL 6
#define WASM_SEC_EXPORT 7
#define WASM_SEC_START 8
#define WASM_SEC_CODE 10

/* Opcodes */
#define OP_UNREACHABLE 0x00
#define OP_NOP 0x01
#define OP_BLOCK 0x02
#define OP_LOOP 0x03
#define OP_IF 0x04
#define OP_ELSE 0x05
#define OP_END 0x0B
#define OP_BR 0x0C
#define OP_BR_IF 0x0D
#define OP_RETURN 0x0F
#define OP_CALL 0x10

#define OP_DROP 0x1A
#define OP_SELECT 0x1B

#define OP_LOCAL_GET 0x20
#define OP_LOCAL_SET 0x21
#define OP_LOCAL_TEE 0x22
#define OP_GLOBAL_GET 0x23
#define OP_GLOBAL_SET 0x24

#define OP_I32_LOAD 0x28
#define OP_I32_LOAD8_S 0x2C
#define OP_I32_LOAD8_U 0x2D
#define OP_I32_LOAD16_S 0x2E
#define OP_I32_LOAD16_U 0x2F
#define OP_I32_STORE 0x36
#define OP_I32_STORE8 0x3A
#define OP_I32_STORE16 0x3B

#define OP_I32_CONST 0x41
#define OP_I32_EQZ 0x45
#define OP_I32_EQ 0x46
#define OP_I32_NE 0x47
#define OP_I32_LT_S 0x48
#define OP_I32_LT_U 0x49
#define OP_I32_GT_S 0x4A
#define OP_I32_GT_U 0x4B
#define OP_I32_LE_S 0x4C
#define OP_I32_LE_U 0x4D
#define OP_I32_GE_S 0x4E
#define OP_I32_GE_U 0x4F

#define OP_I32_CLZ 0x67
#define OP_I32_CTZ 0x68
#define OP_I32_POPCNT 0x69
#define OP_I32_ADD 0x6A
#define OP_I32_SUB 0x6B
#define OP_I32_MUL 0x6C
#define OP_I32_DIV_S 0x6D
#define OP_I32_DIV_U 0x6E
#define OP_I32_REM_S 0x6F
#define OP_I32_REM_U 0x70
#define OP_I32_AND 0x71
#define OP_I32_OR 0x72
#define OP_I32_XOR 0x73
#define OP_I32_SHL 0x74
#define OP_I32_SHR_S 0x75
#define OP_I32_SHR_U 0x76
#define OP_I32_ROTL 0x77
#define OP_I32_ROTR 0x78

#define OP_F32_CONST 0x43
#define OP_F32_EQ 0x5B
#define OP_F32_NE 0x5C
#define OP_F32_LT 0x5D
#define OP_F32_GT 0x5E
#define OP_F32_LE 0x5F
#define OP_F32_GE 0x60
#define OP_F32_ADD 0x92
#define OP_F32_SUB 0x93
#define OP_F32_MUL 0x94
#define OP_F32_DIV 0x95

/* ── LEB128 Decoding Helpers ───────────────────────────────────────────── */

static uint32_t read_u32_leb128(const uint8_t *bytes, uint32_t max_size, uint32_t *offset)
{
    uint32_t result = 0;
    uint32_t shift = 0;
    uint32_t cur = *offset;

    while (cur < max_size) {
        uint8_t byte = bytes[cur++];
        result |= (uint32_t)(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) {
            break;
        }
        shift += 7;
        if (shift >= 35) {
            break;
        }
    }

    *offset = cur;
    return result;
}

static int32_t read_i32_leb128(const uint8_t *bytes, uint32_t max_size, uint32_t *offset)
{
    int32_t result = 0;
    uint32_t shift = 0;
    uint32_t cur = *offset;
    uint8_t byte = 0;

    while (cur < max_size) {
        byte = bytes[cur++];
        result |= (int32_t)(byte & 0x7F) << shift;
        shift += 7;
        if ((byte & 0x80) == 0) {
            break;
        }
        if (shift >= 35) {
            break;
        }
    }

    if ((shift < 32) && (byte & 0x40)) {
        result |= (~0U << shift);
    }

    *offset = cur;
    return result;
}

/* ── Stack Helpers ──────────────────────────────────────────────────────── */

static bool push_stack(SYN_WASM_Context *ctx, uint32_t val)
{
    if (ctx->sp >= SYN_WASM_MAX_STACK) {
        ctx->status = SYN_WASM_TRAP_STACK_OVERFLOW;
        return false;
    }
    ctx->stack[ctx->sp++] = val;
    return true;
}

static bool pop_stack(SYN_WASM_Context *ctx, uint32_t *val)
{
    if (ctx->sp == 0) {
        ctx->status = SYN_WASM_TRAP_STACK_UNDERFLOW;
        return false;
    }
    *val = ctx->stack[--ctx->sp];
    return true;
}

/* ── Module Parser ──────────────────────────────────────────────────────── */

bool syn_wasm_module_load(SYN_WASM_Module *mod, const uint8_t *bytes, uint32_t size)
{
    if (!mod || !bytes || size < 8) {
        return false;
    }

    memset(mod, 0, sizeof(*mod));
    mod->bytes = bytes;
    mod->size = size;

    /* Verify Magic Header & Version */
    uint32_t magic = ((uint32_t)bytes[0]) | ((uint32_t)bytes[1] << 8) | ((uint32_t)bytes[2] << 16) |
                     ((uint32_t)bytes[3] << 24);
    uint32_t version = ((uint32_t)bytes[4]) | ((uint32_t)bytes[5] << 8) |
                       ((uint32_t)bytes[6] << 16) | ((uint32_t)bytes[7] << 24);

    if (magic != WASM_MAGIC || version != WASM_VERSION) {
        return false;
    }

    uint32_t offset = 8;
    uint32_t type_func_indices[SYN_WASM_MAX_FUNCTIONS];
    uint16_t func_decl_count = 0;

    while (offset < size) {
        uint8_t section_id = bytes[offset++];
        uint32_t section_len = read_u32_leb128(bytes, size, &offset);
        uint32_t section_end = offset + section_len;

        if (section_end > size) {
            return false;
        }

        if (section_id == WASM_SEC_TYPE) {
            uint32_t num_types = read_u32_leb128(bytes, section_end, &offset);
            for (uint32_t i = 0; i < num_types && offset < section_end; i++) {
                uint8_t form = bytes[offset++];
                if (form != 0x60) { /* Func form */
                    return false;
                }
                uint32_t num_params = read_u32_leb128(bytes, section_end, &offset);
                for (uint32_t p = 0; p < num_params && offset < section_end; p++) {
                    uint8_t pt = bytes[offset++];
#if (!defined(SYN_WASM_USE_FIXED) || !SYN_WASM_USE_FIXED) && \
    (!defined(SYN_WASM_USE_FLOAT) || !SYN_WASM_USE_FLOAT)
                    if (pt == WASM_TYPE_F32 || pt == WASM_TYPE_F64) {
                        return false; /* Reject float types when tier disabled */
                    }
#endif
                    (void)pt;
                }
                uint32_t num_results = read_u32_leb128(bytes, section_end, &offset);
                for (uint32_t r = 0; r < num_results && offset < section_end; r++) {
                    uint8_t rt = bytes[offset++];
#if (!defined(SYN_WASM_USE_FIXED) || !SYN_WASM_USE_FIXED) && \
    (!defined(SYN_WASM_USE_FLOAT) || !SYN_WASM_USE_FLOAT)
                    if (rt == WASM_TYPE_F32 || rt == WASM_TYPE_F64) {
                        return false; /* Reject float types when tier disabled */
                    }
#endif
                    (void)rt;
                }
            }
        } else if (section_id == WASM_SEC_IMPORT) {
            uint32_t num_imports = read_u32_leb128(bytes, section_end, &offset);
            for (uint32_t i = 0; i < num_imports && offset < section_end; i++) {
                uint32_t mod_len = read_u32_leb128(bytes, section_end, &offset);
                offset += mod_len;
                uint32_t field_len = read_u32_leb128(bytes, section_end, &offset);
                offset += field_len;
                uint8_t kind = bytes[offset++];
                if (kind == 0) { /* Function import */
                    uint32_t type_idx = read_u32_leb128(bytes, section_end, &offset);
                    if (mod->func_count < SYN_WASM_MAX_FUNCTIONS) {
                        mod->funcs[mod->func_count].type_idx = type_idx;
                        mod->funcs[mod->func_count].code_offset = 0;
                        mod->funcs[mod->func_count].code_size = 0;
                        mod->func_count++;
                        mod->import_func_count++;
                    }
                } else if (kind == 1) { /* Table */
                    offset++;
                    offset++;
                } else if (kind == 2) { /* Memory */
                    uint8_t flags = bytes[offset++];
                    read_u32_leb128(bytes, section_end, &offset);
                    if (flags & 1)
                        read_u32_leb128(bytes, section_end, &offset);
                } else if (kind == 3) { /* Global */
                    offset++;
                    offset++;
                }
            }
        } else if (section_id == WASM_SEC_FUNCTION) {
            uint32_t num_funcs = read_u32_leb128(bytes, section_end, &offset);
            for (uint32_t i = 0; i < num_funcs && offset < section_end; i++) {
                uint32_t type_idx = read_u32_leb128(bytes, section_end, &offset);
                if (func_decl_count < SYN_WASM_MAX_FUNCTIONS) {
                    type_func_indices[func_decl_count++] = type_idx;
                }
            }
        } else if (section_id == WASM_SEC_EXPORT) {
            uint32_t num_exports = read_u32_leb128(bytes, section_end, &offset);
            for (uint32_t i = 0; i < num_exports && offset < section_end; i++) {
                uint32_t name_len = read_u32_leb128(bytes, section_end, &offset);
                uint32_t name_offset = offset;
                offset += name_len;
                uint8_t kind = bytes[offset++];
                uint32_t idx = read_u32_leb128(bytes, section_end, &offset);
                if (kind == 0 && mod->export_count < SYN_WASM_MAX_FUNCTIONS) {
                    mod->exports[mod->export_count].name_offset = name_offset;
                    mod->exports[mod->export_count].name_len = (uint16_t)name_len;
                    mod->exports[mod->export_count].func_idx = (uint16_t)idx;
                    mod->export_count++;
                }
            }
        } else if (section_id == WASM_SEC_START) {
            mod->start_func_idx = read_u32_leb128(bytes, section_end, &offset);
            mod->has_start_func = true;
        } else if (section_id == WASM_SEC_CODE) {
            uint32_t num_bodies = read_u32_leb128(bytes, section_end, &offset);
            for (uint32_t i = 0; i < num_bodies && offset < section_end; i++) {
                uint32_t body_size = read_u32_leb128(bytes, section_end, &offset);
                uint32_t body_start = offset;
                if (i < func_decl_count && mod->func_count < SYN_WASM_MAX_FUNCTIONS) {
                    mod->funcs[mod->func_count].type_idx = type_func_indices[i];
                    mod->funcs[mod->func_count].code_offset = body_start;
                    mod->funcs[mod->func_count].code_size = body_size;
                    mod->func_count++;
                }
                offset = body_start + body_size;
            }
        }

        offset = section_end;
    }

    return true;
}

/* ── Context & Execution ─────────────────────────────────────────────────── */

bool syn_wasm_init(SYN_WASM_Context *ctx, const SYN_WASM_Module *mod, uint8_t *linear_mem,
                   uint32_t mem_size)
{
    if (!ctx || !mod) {
        return false;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->module = mod;
    ctx->linear_mem = linear_mem;
    ctx->linear_mem_size = mem_size;
    ctx->status = SYN_WASM_OK;

    return true;
}

bool syn_wasm_register_host(SYN_WASM_Context *ctx, uint16_t import_index, SYN_WASM_HostFunc func)
{
    if (!ctx || import_index >= SYN_WASM_MAX_HOST_FUNCS) {
        return false;
    }

    ctx->host_funcs[import_index] = func;
    if (import_index >= ctx->host_func_count) {
        ctx->host_func_count = import_index + 1;
    }
    return true;
}

int32_t syn_wasm_find_export(const SYN_WASM_Module *mod, const char *name)
{
    if (!mod || !name) {
        return -1;
    }

    size_t len = strlen(name);
    for (uint16_t i = 0; i < mod->export_count; i++) {
        if (mod->exports[i].name_len == len &&
            memcmp(mod->bytes + mod->exports[i].name_offset, name, len) == 0) {
            return (int32_t)mod->exports[i].func_idx;
        }
    }

    return -1;
}

bool syn_wasm_call(SYN_WASM_Context *ctx, uint16_t func_index)
{
    if (!ctx || !ctx->module || func_index >= ctx->module->func_count) {
        return false;
    }

    const SYN_WASM_Module *mod = ctx->module;

    /* Handle imported function directly */
    if (func_index < mod->import_func_count) {
        if (func_index < ctx->host_func_count && ctx->host_funcs[func_index]) {
            uint32_t ret = ctx->host_funcs[func_index](ctx, NULL, 0);
            push_stack(ctx, ret);
            ctx->status = SYN_WASM_HALTED;
            return true;
        }
        ctx->status = SYN_WASM_TRAP_UNREGISTERED_HOST;
        return false;
    }

    /* Initialize Call Frame */
    ctx->call_depth = 0;
    ctx->label_depth = 0;
    ctx->sp = 0;

    SYN_WASM_CallFrame *frame = &ctx->call_stack[0];
    frame->func_idx = func_index;
    frame->return_pc = 0;
    frame->frame_sp = 0;
    frame->local_base = 0;
    ctx->call_depth = 1;

    /* Skip Local Declarations Header in Code Body */
    uint32_t cur = mod->funcs[func_index].code_offset;
    uint32_t end = cur + mod->funcs[func_index].code_size;
    uint32_t num_local_vecs = read_u32_leb128(mod->bytes, end, &cur);

    for (uint32_t i = 0; i < num_local_vecs && cur < end; i++) {
        (void)read_u32_leb128(mod->bytes, end, &cur);
        cur++; /* Skip type byte */
    }

    ctx->local_count = SYN_WASM_MAX_LOCALS;
    memset(ctx->locals, 0, sizeof(ctx->locals));
    ctx->pc = cur;
    ctx->status = SYN_WASM_OK;
    return true;
}

uint32_t syn_wasm_result(const SYN_WASM_Context *ctx)
{
    if (!ctx || ctx->sp == 0) {
        return 0;
    }
    return ctx->stack[ctx->sp - 1];
}

static void branch_to_label(SYN_WASM_Context *ctx, uint32_t label_idx)
{
    if (label_idx >= ctx->label_depth) {
        return;
    }
    uint8_t target_depth = (uint8_t)(ctx->label_depth - 1 - label_idx);
    SYN_WASM_Label *lbl = &ctx->label_stack[target_depth];

    if (lbl->opcode == OP_LOOP) {
        ctx->label_depth = (uint8_t)(target_depth + 1);
        ctx->pc = lbl->target_pc;
    } else {
        ctx->label_depth = target_depth;
        uint32_t depth = 1;
        ctx->pc = lbl->target_pc;
        while (ctx->pc < ctx->module->size && depth > 0) {
            uint8_t op = ctx->module->bytes[ctx->pc++];
            if (op == OP_BLOCK || op == OP_LOOP || op == OP_IF) {
                ctx->pc++;
                depth++;
            } else if (op == OP_END) {
                depth--;
            }
        }
    }
}

/* ── Interpreter Step Loop ───────────────────────────────────────────────── */

SYN_WASM_Status syn_wasm_step(SYN_WASM_Context *ctx, uint16_t max_instructions)
{
    if (!ctx || !ctx->module) {
        return SYN_WASM_TRAP_INVALID_MODULE;
    }

    if (ctx->status != SYN_WASM_OK) {
        return ctx->status;
    }

    const SYN_WASM_Module *mod = ctx->module;
    uint16_t executed = 0;

    while (executed < max_instructions && ctx->status == SYN_WASM_OK) {
        if (ctx->call_depth == 0) {
            ctx->status = SYN_WASM_HALTED;
            break;
        }

        uint8_t opcode = mod->bytes[ctx->pc++];
        executed++;

        switch (opcode) {
        case OP_UNREACHABLE:
            ctx->status = SYN_WASM_TRAP_UNREACHABLE;
            break;

        case OP_NOP:
            break;

        case OP_BLOCK:
        case OP_LOOP:
        case OP_IF: {
            ctx->pc++; /* Block return type byte */
            if (opcode == OP_IF) {
                uint32_t cond = 0;
                if (!pop_stack(ctx, &cond))
                    break;
                if (cond == 0) {
                    /* Skip to else or end */
                    uint32_t depth = 1;
                    while (ctx->pc < mod->size && depth > 0) {
                        uint8_t op = mod->bytes[ctx->pc++];
                        if (op == OP_BLOCK || op == OP_LOOP || op == OP_IF) {
                            ctx->pc++;
                            depth++;
                        } else if (op == OP_END) {
                            depth--;
                        } else if (op == OP_ELSE && depth == 1) {
                            break;
                        }
                    }
                    break;
                }
            }
            if (ctx->label_depth < SYN_WASM_MAX_LABELS) {
                SYN_WASM_Label *lbl = &ctx->label_stack[ctx->label_depth++];
                lbl->opcode = opcode;
                lbl->target_pc = ctx->pc;
                lbl->stack_sp = ctx->sp;
            }
            break;
        }

        case OP_ELSE: {
            /* Skip to end of block */
            uint32_t depth = 1;
            while (ctx->pc < mod->size && depth > 0) {
                uint8_t op = mod->bytes[ctx->pc++];
                if (op == OP_BLOCK || op == OP_LOOP || op == OP_IF) {
                    ctx->pc++;
                    depth++;
                } else if (op == OP_END) {
                    depth--;
                }
            }
            break;
        }

        case OP_END:
            if (ctx->label_depth > 0) {
                ctx->label_depth--;
            } else {
                /* Function Return */
                ctx->call_depth--;
                if (ctx->call_depth == 0) {
                    ctx->status = SYN_WASM_HALTED;
                } else {
                    ctx->pc = ctx->call_stack[ctx->call_depth].return_pc;
                }
            }
            break;

        case OP_BR: {
            uint32_t label_idx = read_u32_leb128(mod->bytes, mod->size, &ctx->pc);
            branch_to_label(ctx, label_idx);
            break;
        }

        case OP_BR_IF: {
            uint32_t label_idx = read_u32_leb128(mod->bytes, mod->size, &ctx->pc);
            uint32_t cond = 0;
            if (pop_stack(ctx, &cond) && cond != 0) {
                branch_to_label(ctx, label_idx);
            }
            break;
        }

        case OP_RETURN:
            ctx->call_depth--;
            if (ctx->call_depth == 0) {
                ctx->status = SYN_WASM_HALTED;
            } else {
                ctx->pc = ctx->call_stack[ctx->call_depth].return_pc;
            }
            break;

        case OP_CALL: {
            uint32_t target_idx = read_u32_leb128(mod->bytes, mod->size, &ctx->pc);
            if (target_idx < mod->import_func_count) {
                /* Call Host Function */
                if (target_idx < ctx->host_func_count && ctx->host_funcs[target_idx]) {
                    uint32_t ret = ctx->host_funcs[target_idx](ctx, NULL, 0);
                    push_stack(ctx, ret);
                } else {
                    ctx->status = SYN_WASM_TRAP_UNREGISTERED_HOST;
                }
            } else if (ctx->call_depth < SYN_WASM_MAX_CALL_DEPTH) {
                SYN_WASM_CallFrame *frame = &ctx->call_stack[ctx->call_depth++];
                frame->func_idx = (uint16_t)target_idx;
                frame->return_pc = ctx->pc;
                frame->frame_sp = ctx->sp;

                uint32_t cur = mod->funcs[target_idx].code_offset;
                uint32_t end = cur + mod->funcs[target_idx].code_size;
                uint32_t num_local_vecs = read_u32_leb128(mod->bytes, end, &cur);

                for (uint32_t i = 0; i < num_local_vecs && cur < end; i++) {
                    uint32_t count = read_u32_leb128(mod->bytes, end, &cur);
                    cur++;
                    (void)count;
                }
                ctx->pc = cur;
            } else {
                ctx->status = SYN_WASM_TRAP_CALL_STACK_OVERFLOW;
            }
            break;
        }

        case OP_DROP: {
            uint32_t dummy = 0;
            pop_stack(ctx, &dummy);
            break;
        }

        case OP_SELECT: {
            uint32_t cond = 0, val2 = 0, val1 = 0;
            if (pop_stack(ctx, &cond) && pop_stack(ctx, &val2) && pop_stack(ctx, &val1)) {
                push_stack(ctx, (cond != 0) ? val1 : val2);
            }
            break;
        }

        case OP_LOCAL_GET: {
            uint32_t idx = read_u32_leb128(mod->bytes, mod->size, &ctx->pc);
            if (idx < ctx->local_count) {
                push_stack(ctx, ctx->locals[idx]);
            }
            break;
        }

        case OP_LOCAL_SET: {
            uint32_t idx = read_u32_leb128(mod->bytes, mod->size, &ctx->pc);
            uint32_t val;
            if (pop_stack(ctx, &val) && idx < ctx->local_count) {
                ctx->locals[idx] = val;
            }
            break;
        }

        case OP_LOCAL_TEE: {
            uint32_t idx = read_u32_leb128(mod->bytes, mod->size, &ctx->pc);
            if (ctx->sp > 0 && idx < ctx->local_count) {
                ctx->locals[idx] = ctx->stack[ctx->sp - 1];
            }
            break;
        }

        case OP_GLOBAL_GET: {
            uint32_t idx = read_u32_leb128(mod->bytes, mod->size, &ctx->pc);
            if (idx < SYN_WASM_MAX_GLOBALS) {
                push_stack(ctx, ctx->globals[idx]);
            }
            break;
        }

        case OP_GLOBAL_SET: {
            uint32_t idx = read_u32_leb128(mod->bytes, mod->size, &ctx->pc);
            uint32_t val;
            if (pop_stack(ctx, &val) && idx < SYN_WASM_MAX_GLOBALS) {
                ctx->globals[idx] = val;
            }
            break;
        }

        case OP_I32_LOAD:
        case OP_I32_LOAD8_S:
        case OP_I32_LOAD8_U:
        case OP_I32_LOAD16_S:
        case OP_I32_LOAD16_U: {
            read_u32_leb128(mod->bytes, mod->size, &ctx->pc); /* alignment */
            uint32_t offset = read_u32_leb128(mod->bytes, mod->size, &ctx->pc);
            uint32_t base_addr = 0;
            if (!pop_stack(ctx, &base_addr))
                break;
            uint32_t addr = base_addr + offset;

            if (!ctx->linear_mem || addr + 4 > ctx->linear_mem_size) {
                ctx->status = SYN_WASM_TRAP_OUT_OF_BOUNDS;
                break;
            }

            uint32_t val = 0;
            if (opcode == OP_I32_LOAD) {
                val = ((uint32_t)ctx->linear_mem[addr]) |
                      ((uint32_t)ctx->linear_mem[addr + 1] << 8) |
                      ((uint32_t)ctx->linear_mem[addr + 2] << 16) |
                      ((uint32_t)ctx->linear_mem[addr + 3] << 24);
            } else if (opcode == OP_I32_LOAD8_U) {
                val = ctx->linear_mem[addr];
            } else if (opcode == OP_I32_LOAD8_S) {
                val = (int32_t)(int8_t)ctx->linear_mem[addr];
            } else if (opcode == OP_I32_LOAD16_U) {
                val =
                    ((uint32_t)ctx->linear_mem[addr]) | ((uint32_t)ctx->linear_mem[addr + 1] << 8);
            } else if (opcode == OP_I32_LOAD16_S) {
                val = (int32_t)(int16_t)(((uint32_t)ctx->linear_mem[addr]) |
                                         ((uint32_t)ctx->linear_mem[addr + 1] << 8));
            }
            push_stack(ctx, val);
            break;
        }

        case OP_I32_STORE:
        case OP_I32_STORE8:
        case OP_I32_STORE16: {
            read_u32_leb128(mod->bytes, mod->size, &ctx->pc); /* alignment */
            uint32_t offset = read_u32_leb128(mod->bytes, mod->size, &ctx->pc);
            uint32_t val = 0, base_addr = 0;
            if (!pop_stack(ctx, &val) || !pop_stack(ctx, &base_addr))
                break;
            uint32_t addr = base_addr + offset;

            if (!ctx->linear_mem || addr >= ctx->linear_mem_size) {
                ctx->status = SYN_WASM_TRAP_OUT_OF_BOUNDS;
                break;
            }

            if (opcode == OP_I32_STORE && addr + 4 <= ctx->linear_mem_size) {
                ctx->linear_mem[addr] = (uint8_t)(val & 0xFF);
                ctx->linear_mem[addr + 1] = (uint8_t)((val >> 8) & 0xFF);
                ctx->linear_mem[addr + 2] = (uint8_t)((val >> 16) & 0xFF);
                ctx->linear_mem[addr + 3] = (uint8_t)((val >> 24) & 0xFF);
            } else if (opcode == OP_I32_STORE8) {
                ctx->linear_mem[addr] = (uint8_t)(val & 0xFF);
            } else if (opcode == OP_I32_STORE16 && addr + 2 <= ctx->linear_mem_size) {
                ctx->linear_mem[addr] = (uint8_t)(val & 0xFF);
                ctx->linear_mem[addr + 1] = (uint8_t)((val >> 8) & 0xFF);
            }
            break;
        }

        case OP_I32_CONST: {
            int32_t val = read_i32_leb128(mod->bytes, mod->size, &ctx->pc);
            push_stack(ctx, (uint32_t)val);
            break;
        }

        case OP_I32_EQZ: {
            uint32_t a = 0;
            if (pop_stack(ctx, &a)) {
                push_stack(ctx, (a == 0) ? 1 : 0);
            }
            break;
        }

        case OP_I32_EQ:
        case OP_I32_NE:
        case OP_I32_LT_S:
        case OP_I32_LT_U:
        case OP_I32_GT_S:
        case OP_I32_GT_U:
        case OP_I32_LE_S:
        case OP_I32_LE_U:
        case OP_I32_GE_S:
        case OP_I32_GE_U: {
            uint32_t b = 0, a = 0;
            if (!pop_stack(ctx, &b) || !pop_stack(ctx, &a))
                break;
            bool res = false;
            switch (opcode) {
            case OP_I32_EQ:
                res = (a == b);
                break;
            case OP_I32_NE:
                res = (a != b);
                break;
            case OP_I32_LT_S:
                res = ((int32_t)a < (int32_t)b);
                break;
            case OP_I32_LT_U:
                res = (a < b);
                break;
            case OP_I32_GT_S:
                res = ((int32_t)a > (int32_t)b);
                break;
            case OP_I32_GT_U:
                res = (a > b);
                break;
            case OP_I32_LE_S:
                res = ((int32_t)a <= (int32_t)b);
                break;
            case OP_I32_LE_U:
                res = (a <= b);
                break;
            case OP_I32_GE_S:
                res = ((int32_t)a >= (int32_t)b);
                break;
            case OP_I32_GE_U:
                res = (a >= b);
                break;
            }
            push_stack(ctx, res ? 1 : 0);
            break;
        }

        case OP_I32_ADD:
        case OP_I32_SUB:
        case OP_I32_MUL:
        case OP_I32_DIV_S:
        case OP_I32_DIV_U:
        case OP_I32_REM_S:
        case OP_I32_REM_U:
        case OP_I32_AND:
        case OP_I32_OR:
        case OP_I32_XOR:
        case OP_I32_SHL:
        case OP_I32_SHR_S:
        case OP_I32_SHR_U: {
            uint32_t b = 0, a = 0;
            if (!pop_stack(ctx, &b) || !pop_stack(ctx, &a))
                break;
            uint32_t res = 0;
            switch (opcode) {
            case OP_I32_ADD:
                res = a + b;
                break;
            case OP_I32_SUB:
                res = a - b;
                break;
            case OP_I32_MUL:
                res = a * b;
                break;
            case OP_I32_DIV_S:
                if (b == 0) {
                    ctx->status = SYN_WASM_TRAP_DIV_ZERO;
                    break;
                }
                res = (uint32_t)((int32_t)a / (int32_t)b);
                break;
            case OP_I32_DIV_U:
                if (b == 0) {
                    ctx->status = SYN_WASM_TRAP_DIV_ZERO;
                    break;
                }
                res = a / b;
                break;
            case OP_I32_REM_S:
                if (b == 0) {
                    ctx->status = SYN_WASM_TRAP_DIV_ZERO;
                    break;
                }
                res = (uint32_t)((int32_t)a % (int32_t)b);
                break;
            case OP_I32_REM_U:
                if (b == 0) {
                    ctx->status = SYN_WASM_TRAP_DIV_ZERO;
                    break;
                }
                res = a % b;
                break;
            case OP_I32_AND:
                res = a & b;
                break;
            case OP_I32_OR:
                res = a | b;
                break;
            case OP_I32_XOR:
                res = a ^ b;
                break;
            case OP_I32_SHL:
                res = a << (b & 31);
                break;
            case OP_I32_SHR_S:
                res = (uint32_t)((int32_t)a >> (b & 31));
                break;
            case OP_I32_SHR_U:
                res = a >> (b & 31);
                break;
            }
            if (ctx->status == SYN_WASM_OK) {
                push_stack(ctx, res);
            }
            break;
        }

#if defined(SYN_WASM_USE_FIXED) && SYN_WASM_USE_FIXED
        case OP_F32_CONST: {
            uint32_t raw_bits = ((uint32_t)mod->bytes[ctx->pc]) |
                                ((uint32_t)mod->bytes[ctx->pc + 1] << 8) |
                                ((uint32_t)mod->bytes[ctx->pc + 2] << 16) |
                                ((uint32_t)mod->bytes[ctx->pc + 3] << 24);
            ctx->pc += 4;
            float fval;
            memcpy(&fval, &raw_bits, sizeof(float));
            syn_q16_t qval = syn_q16_from_float(fval);
            push_stack(ctx, (uint32_t)qval);
            break;
        }

        case OP_F32_ADD:
        case OP_F32_SUB:
        case OP_F32_MUL:
        case OP_F32_DIV: {
            uint32_t b = 0, a = 0;
            if (!pop_stack(ctx, &b) || !pop_stack(ctx, &a))
                break;
            syn_q16_t qa = (syn_q16_t)a, qb = (syn_q16_t)b, qres = 0;
            switch (opcode) {
            case OP_F32_ADD:
                qres = syn_q16_add(qa, qb);
                break;
            case OP_F32_SUB:
                qres = syn_q16_sub(qa, qb);
                break;
            case OP_F32_MUL:
                qres = syn_q16_mul(qa, qb);
                break;
            case OP_F32_DIV:
                if (qb == 0) {
                    ctx->status = SYN_WASM_TRAP_DIV_ZERO;
                    break;
                }
                qres = syn_q16_div(qa, qb);
                break;
            }
            if (ctx->status == SYN_WASM_OK) {
                push_stack(ctx, (uint32_t)qres);
            }
            break;
        }

        case OP_F32_EQ:
        case OP_F32_NE:
        case OP_F32_LT:
        case OP_F32_GT:
        case OP_F32_LE:
        case OP_F32_GE: {
            uint32_t b = 0, a = 0;
            if (!pop_stack(ctx, &b) || !pop_stack(ctx, &a))
                break;
            syn_q16_t qa = (syn_q16_t)a, qb = (syn_q16_t)b;
            bool res = false;
            switch (opcode) {
            case OP_F32_EQ:
                res = (qa == qb);
                break;
            case OP_F32_NE:
                res = (qa != qb);
                break;
            case OP_F32_LT:
                res = (qa < qb);
                break;
            case OP_F32_GT:
                res = (qa > qb);
                break;
            case OP_F32_LE:
                res = (qa <= qb);
                break;
            case OP_F32_GE:
                res = (qa >= qb);
                break;
            }
            push_stack(ctx, res ? 1 : 0);
            break;
        }
#endif

        default:
            ctx->status = SYN_WASM_TRAP_BAD_OPCODE;
            break;
        }
    }

    if (executed >= max_instructions && ctx->status == SYN_WASM_OK) {
        return SYN_WASM_YIELDED;
    }

    return ctx->status;
}
