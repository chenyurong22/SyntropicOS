---
description: Automated Quality & Testing Checks Workflow
---

# SyntropicOS Comprehensive Checks Workflow

Follow this sequence for maximum feedback speed, cache reuse, and fail-fast verification. Always execute the containerized target (`make container-*`) for each step.

## Step 0: Code Formatting, Static Analysis, Footprint & MISRA Safety (Fail-Fast Static Pipeline)
Run code polish, AST linting, MISRA C:2023 compliance, stack usage, memory size audit, and static analysis *first* to catch errors before compiling binaries or running emulators:
- **Code Formatting**: `make container-format`
- **AST Structural Linter**: `make container-lint`
- **MISRA C:2023 Compliance Scan**: `make container-misra` *(Uses `build/cppcheck` AST cache)*
- **Clang `scan-build` Static Analyzer**: `make container-static` *(Uses `build/cppcheck` AST cache)*
- **Stack Usage Frame Audit**: `make container-stack`
- **Binary Footprint Subsystem Audit**: `make container-size`
- **Cyclomatic Complexity Audit**: `make container-complexity`

## Step 1: Unit Testing & Dynamic Sanitizer Audit
Run fast unit tests and sanitizer builds to verify runtime memory safety:
- **Unit Test Suite**: `make container-test`
- **Containerized Sanitizer Audit**: `make container-san`

## Step 2: Bare-Metal & Hardware Board Emulation
Verify microcontroller execution across virtual hardware targets:
- **QEMU Bare-Metal ARM Cortex-M4**: `make container-qemu`
- **Renode STM32F4 Board Emulation**: `make container-renode`
- **Protocol libFuzzer Targets**: `make container-fuzz`

## Step 3: 3rd-Party Production Container Integration Suite
Execute end-to-end integration tests against 8 production container daemons:
- **Integration Test Battery**: `make container-integration`

## Step 4: Doxygen API Documentation Coverage
Verify API documentation completeness:
- **Doxygen Check**: `make container-dox`

## Step 5: Markdown Documentation
Inspect project documentation (`README.md`, `docs/`, `mkdocs.yml`) for structural consistency.