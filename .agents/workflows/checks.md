---
description: Automated Quality & Testing Checks Workflow
---

# SyntropicOS Comprehensive Checks Workflow

Follow this sequence for maximum feedback speed, cache reuse, and fail-fast verification.

## Step 0: Code Formatting, Static Analysis & MISRA Safety (Fail-Fast Static Pipeline)
Run code polish, AST linting, MISRA C:2023 compliance, and static analysis *first* to catch errors before compiling binaries or running emulators:
- **Code Formatting**: `make format` (or `make container-format`)
- **AST Structural Linter**: `make lint` (or `make container-lint`)
- **MISRA C:2023 Compliance Scan**: `make misra` (or `make container-misra`) *(Uses `build/cppcheck` AST cache)*
- **Clang `scan-build` Static Analyzer**: `make static` (or `make container-static`) *(Uses `build/cppcheck` AST cache)*

## Step 1: Unit Testing & Dynamic Sanitizer Audit
Run fast unit tests and sanitizer builds to verify runtime memory safety:
- **Unit Test Suite**: `make test` (or `make container-test`)
- **Containerized Sanitizer Audit**: `make san` (or `make container-san`)

## Step 2: Bare-Metal & Hardware Board Emulation
Verify microcontroller execution across virtual hardware targets:
- **QEMU Bare-Metal ARM Cortex-M4**: `make qemu` (or `make container-qemu`)
- **Renode STM32F4 Board Emulation**: `make renode` (or `make container-renode`)
- **Protocol libFuzzer Targets**: `make fuzz` (or `make container-fuzz`)

## Step 3: 3rd-Party Production Container Integration Suite
Execute end-to-end integration tests against 8 production container daemons (Mosquitto MQTT, Chrony SNTP, Nginx HTTP, Node.js WS, CoreDNS, SocketCAN, WireGuard, Modbus TCP):
- **Integration Test Battery**: `make integration` (or `make container-integration`)

## Step 4: Doxygen API Documentation Coverage
Verify API documentation completeness:
- **Doxygen Check**: `make dox` (or `make container-dox`)

## Step 5: Markdown Documentation
Inspect project documentation (`README.md`, `docs/`, `mkdocs.yml`) for structural consistency.