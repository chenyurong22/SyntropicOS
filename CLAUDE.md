# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Agent Directives

- **Zero Fluff Policy**: No preambles ("Sure, I can help"), postambles, or conversational filler. Short, punchy sentences. Maximum information density.
- **Anti-Hype Policy**: No buzzwords (*cutting-edge, leverage, seamless, robust, empower, next-gen*). Plain technical facts only in docs and comments.
- **Anti-Symptom Patching**: Never comment out `TEST_ASSERT` lines, return dummy status codes, suppress MISRA/lint warnings with pragmas, or loosen test thresholds to pass checks. Fix root cause in source.
- **Workflow Integrity**: When a slash command (`/checks`, `/coverage`) is triggered, execute every step and every containerized target (`make container-*`) in full. Never substitute host commands for container targets.
- **Test Reporting**: If any test fails or static analysis flags an issue, report `FAILURE:` followed by exact error output. Never hide or downplay failures.
- **Code changes**: Output minimal, targeted diffs only. No unmodified boilerplate. Comments only for complex algorithms, edge cases, and non-obvious logic — never for self-explanatory code.

## Commands

All real build targets live in `tools/containers/Makefile`; the top-level `Makefile` forwards to it.

### Unit Tests (host, no container)
```sh
make test                  # compile + run all ~1200+ Unity unit tests
make -f tests/Makefile.unity test   # direct invocation
```

### Containerized Targets (CI-equivalent)
```sh
make container-format      # clang-format check (add FIX=1 to auto-fix)
make container-static      # cppcheck + clang-tidy + MISRA
make container-test        # unit test suite inside container
make container-qemu        # bare-metal ARM Cortex-M4 boot test in QEMU
make container-cov         # LCOV HTML coverage report → coverage_html/index.html
make container-dox         # Doxygen API documentation
make container-san         # AddressSanitizer + UndefinedBehaviorSanitizer
make container-fuzz        # LLVM libFuzzer targets
make container-integration # Docker Compose E2E tests against real services
```

### Slash-Command Workflows
- `/checks` — container-format → container-static → container-test → container-qemu
- `/coverage` — container-cov → inspect HTML report → container-dox → docs audit

### Formatting & Linting (host)
```sh
make format                # clang-format (FIX=1 to auto-fix in-place)
make lint                  # cppcheck via tests/Makefile.check
make static                # clang-tidy
```

### Install Git Hooks
```sh
make install-hooks         # pre-commit hook runs container-format FIX=1 automatically
```

## Architecture

### Design Constraints

SyntropicOS is a **C99, zero-heap, zero-RTOS** embedded framework. Every design decision follows from three invariants:
- **No dynamic allocation** — all structures are statically allocated by the application.
- **No blocking calls** — all drivers and protocol stacks are non-blocking state machines.
- **No stack-switching** — cooperative multitasking uses stackless protothreads (`syn_pt`), costing 2 bytes RAM per "thread".

### Kernel Layer (`src/syntropic/pt/`, `src/syntropic/sched/`)

`syn_pt` implements Duff's-device stackless coroutines via `PT_BEGIN` / `PT_WAIT_UNTIL` / `PT_END` macros. A protothread saves only its resume line number (2 bytes). The cooperative scheduler (`syn_sched`) drives all tasks from a single superloop; task descriptors are ~16–28 bytes each. Supporting primitives: timer wheel, workqueue, event flags, active objects (AO), sequencer, HW watchdog.

### Port / HAL Layer (`src/syntropic/port/`, `src/syntropic/port_stubs/`)

Platform-specific functions are declared as weak stubs in `port_stubs/`. Applications override them with concrete HAL implementations for their target (stm32_hal, rp2040, esp32, arduino, posix). Unit tests use `tests/unit/mocks/mock_port.c` instead. This is the only file that differs between host builds and target builds.

### Protocol Stacks (`src/syntropic/proto/`)

70+ non-blocking protocol drivers, all implemented as state machines callable from a protothread: Modbus RTU/Master/TCP, CANopen (CiA 301/401/402/303), UDS (ISO 14229-1), ISO-TP, J1939, EtherCAT, BACnet MS/TP, LIN, DALI, NMEA, MAVLink, DMX512, CRSF, SBUS, SMBus/PMBus, M-Bus, DLT645, AT parser, COBS, XCP, CCP, LSS, DeviceNet, N2K, IR.

### Networking (`src/syntropic/net/`)

Full TCP/IP stack: Ethernet → IP → TCP/UDP/ICMP/IGMP/DHCP/DNS/AutoIP → CoAP, SNTP, MQTT, HTTP client/server, WebSocket, WireGuard. All layers non-blocking; driven by the scheduler.

### Module Conventions

- **New module**: add `.c` to `sources.mk` (`SYN_SRCS`), add a `test_<module>.c` in `tests/unit/`, add a Doxygen-documented header in `src/syntropic/<subsystem>/`, add a doc page in `docs/modules/`, add an example sketch in `examples/`.
- **Public API surface**: everything exposed via `src/syntropic/syntropic.h` (the single consumer include). Platform/Arduino consumers use `src/SyntropicOS.h` (umbrella wrapper).
- **Feature flags**: `src/syntropic/syn_config_template.h` — copy to application as `syn_config.h`.

### Test Architecture

Unit tests (`tests/unit/test_*.c`) compile with `gcc -std=c99 -Wall -Wextra -Werror -pedantic` on the host using the Unity framework and `mock_port.c`. Each test file covers one module. Fuzz targets (`tests/fuzz/fuzz_*.c`) use LLVM libFuzzer against parser/codec boundaries. Integration tests (`tests/integration/`) spin up real service containers (Mosquitto, Modbus, SNTP, EtherCAT, WireGuard, DNS, HTTP, WebSocket, CAN) via Docker Compose and run against them from a host test binary.
