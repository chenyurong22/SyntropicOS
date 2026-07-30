# WebAssembly (WASM) Virtual Machine Module

The `syn_wasm` module provides a 100% zero-heap, non-blocking 32-bit WebAssembly MVP cooperative interpreter designed for embedded microcontrollers. It supports static linear memory arrays, zero-copy module loading from Flash memory, host function registration, and instruction-sliced execution with protothread (`syn_pt`) scheduling.

---

## Features

- **Zero-Heap Execution**: All stack frames, local variables, and global states are allocated inside static structures.
- **Embedded C Compiler Compatibility**: Designed for standard C code compiled via `clang --target=wasm32-unknown-unknown`.
- **Customizable RAM Footprint**: Linear memory size and stack depth are user-configurable down to microcontrollers with <4KB RAM.
- **Host Function Integration**: Register C native callback functions to expose OS APIs (`syn_log`, `syn_gpio`, timers) to WASM programs.
- **Protothread Cooperative Slicing**: Execute WASM instructions in time-sliced increments (`syn_wasm_step`) without blocking system responsiveness.
- **Fixed-Point Math Option**: Optional `SYN_WASM_USE_FIXED=1` mode maps `f32`/`f64` operations to `syn_qmath.h` Q16.16 fixed-point arithmetic on FPU-less microcontrollers.

---

## Recommended C-to-WASM Compilation Command

To compile standard C applications into WASM binaries compatible with `syn_wasm`, use LLVM Clang:

```bash
clang --target=wasm32-unknown-unknown -O2 -nostdlib \
  -mbulk-memory -msign-ext -mmultivalue -mnontrapping-fptoint \
  -Wl,--no-entry -Wl,--export-all -Wl,--allow-undefined \
  -Wl,-z,stack-size=1024 app.c -o app.wasm
```

### Compiler Flags Summary

| Flag | Purpose |
|---|---|
| `--target=wasm32-unknown-unknown` | Targets 32-bit WebAssembly architecture |
| `-O2 -nostdlib` | Optimization level 2 with zero C standard library bloat |
| `-mbulk-memory` | Enables WASM `0xFC` bulk memory instructions (`memory.copy`, `memory.fill`) |
| `-msign-ext` | Enables sign extension instructions |
| `-mnontrapping-fptoint` | Enables non-trapping saturating float-to-int conversions |
| `-Wl,--no-entry` | Omits standard `_start` entry point requirement |
| `-Wl,--export-all` | Exports all declared C functions for lookup via `syn_wasm_find_export` |
| `-Wl,--allow-undefined` | Permits `extern` host function declarations |
| `-Wl,-z,stack-size=1024` | Limits internal WASM stack memory to 1KB for embedded RAM compatibility |

---

## Configuration Options

Configure VM parameters in `syn_config.h` or via compiler build flags:

```c
#define SYN_WASM_MAX_STACK       64    /* Evaluation stack depth */
#define SYN_WASM_MAX_LOCALS     256    /* Total local variables across all frames */
#define SYN_WASM_MAX_CALL_DEPTH  32    /* Maximum function call depth */
#define SYN_WASM_MAX_FUNCTIONS   32    /* Max functions per WASM module */
#define SYN_WASM_MAX_HOST_FUNCS  16    /* Max registered host function callbacks */
#define SYN_WASM_MAX_GLOBALS    16    /* Max WASM global variables */
```

---

## Code Example

```c
#include <syntropic/vm/syn_wasm.h>
#include <stdio.h>

static uint8_t wasm_ram[4096];
static SYN_WASM_Module mod;
static SYN_WASM_Context ctx;

/* Host function callback signature */
static uint64_t host_log_cb(SYN_WASM_Context *c, const uint64_t *args, uint8_t argc) {
    (void)c; (void)argc;
    printf("[WASM LOG] Code=%u, Val=%u\n", (uint32_t)args[0], (uint32_t)args[1]);
    return args[0] + args[1];
}

void run_wasm_app(const uint8_t *wasm_bytes, size_t wasm_size) {
    /* 1. Parse module header and sections from Flash */
    if (!syn_wasm_module_load(&mod, wasm_bytes, (uint32_t)wasm_size)) {
        printf("Error: Invalid WASM module\n");
        return;
    }

    /* 2. Initialize execution context with 4KB RAM buffer */
    if (!syn_wasm_init(&ctx, &mod, wasm_ram, sizeof(wasm_ram))) {
        printf("Error: WASM context init failed\n");
        return;
    }

    /* 3. Register native host callback at import index 0 */
    syn_wasm_register_host(&ctx, 0, host_log_cb);

    /* 4. Find exported function "run_tests" */
    int32_t fn_idx = syn_wasm_find_export(&mod, "run_tests");
    if (fn_idx < 0) {
        printf("Error: Exported function 'run_tests' not found\n");
        return;
    }

    /* 5. Set up call frame and execute with instruction slicing */
    syn_wasm_call(&ctx, (uint16_t)fn_idx);

    SYN_WASM_Status st;
    do {
        /* Slice execution: run up to 100 WASM opcodes per step */
        st = syn_wasm_step(&ctx, 100);
    } while (st == SYN_WASM_YIELDED);

    if (st == SYN_WASM_HALTED) {
        uint32_t result = (uint32_t)syn_wasm_result(&ctx);
        printf("WASM execution completed. Result: %u\n", result);
    } else {
        printf("WASM execution trapped! Trap code: %d\n", st);
    }
}
```
