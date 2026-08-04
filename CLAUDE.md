# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SyntropicOS is a zero-heap, C99 bare-metal application framework and cooperative OS for embedded MCUs (STM32, RP2040, ESP32, AVR, RISC-V). All 70+ drivers and protocol stacks are non-blocking state machines; there is no `malloc()` anywhere — everything is statically allocated. Compiles under `-std=c99 -pedantic -Wall -Wextra -Werror`.

## Common Commands

Run from the repo root (Git Bash on Windows). Every native target has a `container-*` variant that runs the same thing inside Docker/Podman (`make container-test`, `make container-san`, ...) — use those when a tool (gcc, clang-format, qemu) isn't on the host.

```bash
make test           # Unity unit test suite (host gcc, single binary, 1200+ tests)
make format         # clang-format check;  make format FIX=1  to auto-fix
make lint           # clang-tidy
make misra          # MISRA C:2023 compliance scan
make san            # test suite under ASan + UBSan
make static         # Cppcheck + Clang scan-build
make dox            # Doxygen build — ZERO warnings tolerated
make qemu           # bare-metal Cortex-M4 boot test (QEMU lm3s6965evb)
make fuzz           # libFuzzer protocol targets (COBS, Modbus, MQTT, HTTP...)
make cov            # LCOV coverage report
make integration    # E2E tests vs real daemons (Mosquitto, Nginx, CoreDNS...) via docker-compose
make examples       # compile-check the examples/
make install-hooks  # enable .githooks pre-commit (runs container-format FIX=1)
```

The test suite can also be invoked directly: `make -f tests/Makefile.unity test`. There is no single-test filter — `test_runner.c` builds one binary that calls every `run_*_tests()` function.

## Testing Architecture

- Each module gets `tests/unit/test_<module>.c` exposing a `void run_<module>_tests(void)` that calls `RUN_TEST(...)` for each Unity test function.
- **Registering a new test file requires two edits**: declare + call `run_<module>_tests()` in `tests/unit/test_runner.c`, and (if it's a new source file) add the `.c` to `SYN_SRCS` in `tests/Makefile.unity`.
- Hardware is mocked by `tests/unit/mocks/mock_port.c`; the global `setUp()` calls `mock_port_reset()` before every test.
- The same test suite is cross-compiled for ARM and run under QEMU by `make qemu` (uses the `ARM_*` rules in `tests/Makefile.unity`).

## Source Architecture

Layering (top to bottom): application → modules → cooperative kernel → port layer → hardware.

- `src/syntropic/` — all portable code, organized by category (`drivers/`, `proto/`, `net/`, `sched/`, `util/`, `dsp/`, `motor/`, `control/`, `storage/`, `display/`, `ui/`, `crypto/`, ...). Files are named `syn_<module>.c/.h`. Umbrella header: `src/syntropic/syntropic.h`.
- `src/syntropic/pt/` + `src/syntropic/sched/` — the kernel: stackless protothreads (`syn_pt`, 2 bytes of continuation state) and the cooperative scheduler (`syn_sched`). Tasks are `SYN_PT_Status fn(SYN_PT *pt, SYN_Task *task)` functions using `PT_BEGIN`/`PT_END` macros; they must never block — use `PT_TASK_DELAY_MS` etc.
- `src/syntropic/port/` — the hardware abstraction as `syn_port_*.h` headers declaring free functions (`syn_port_get_tick_ms`, `syn_port_gpio_write`, ...). Portable code calls only these.
- `src/port/` — concrete port implementations per platform: `stm32f4` (bare-register), `stm32_hal`, `esp32`, `rp2040`, `arduino`, `posix`, `ch32v307`.
- `src/syntropic/port_stubs/syn_port_stubs.c` — weak stubs for every port function that assert at runtime, so a missing port implementation fails loudly instead of at link time.
- Configuration is compile-time: users copy `src/syntropic/syn_config_template.h` to `syn_config.h` and toggle `SYN_USE_*` feature flags and buffer sizes; headers fall back to defaults when absent. Optional modules are guarded by these flags (the test build enables them via `-D` in `tests/Makefile.unity`).

## Adding a New Module — Checklist

1. `src/syntropic/<category>/syn_<name>.c/.h` with full Doxygen comments on the public API (`make dox` enforces zero warnings).
2. Add the `.c` to `SYN_SRCS` in `tests/Makefile.unity` and write `tests/unit/test_<name>.c`; register it in `test_runner.c`.
3. `CMakeLists.txt` and `sources.mk` hold the consumer-facing source lists — add the file there if it should ship to CMake/Makefile users.
4. Format with clang-format (`.clang-format`, LLVM-based, 4-space indent) — `make format FIX=1`.
