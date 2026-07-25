---
description: Automated Quality & Testing Checks Workflow
---

# SyntropicOS Comprehensive Checks Workflow

Follow this sequence to verify software formatting, MISRA C safety compliance, memory safety, static analysis, coverage, and documentation.

## Step 0: Code Formatting, Linting & MISRA C:2023 Compliance
Run code polish, AST linting, and MISRA C:2023 compliance checkers:
- **Code Formatting**: `make format` (or `make container-format`)
- **AST Structural Linter**: `make lint` (or `make container-lint`)
- **MISRA C:2023 Compliance Scan**: `make misra` (or `make container-misra`)

## Step 1: Unit Testing & Dynamic Sanitizers
Run host and containerized test suites:
- **Unit Test Suite**: `make test` (or `make container-test`)
- **Containerized Sanitizer Audit**: `make san` (or `make container-san`)
- **QEMU Bare-Metal Emulation**: `make qemu` (or `make container-qemu`)
- **Renode STM32F4 Board Emulation**: `make renode` (or `make container-renode`)
- **Protocol libFuzzer Targets**: `make fuzz` (or `make container-fuzz`)

## Step 2: 3rd-Party Production Container Integration Suite
Execute integration tests against 8 genuine production container daemons (Mosquitto MQTT, Chrony SNTP, Nginx HTTP, Node.js WS, CoreDNS, SocketCAN, WireGuard, Modbus TCP):
- **Integration Test Battery**: `make integration` (or `make container-integration`)

## Step 3: Static Analysis
Run static analysis checks:
- **Cppcheck & Clang scan-build**: `make static` (or `make container-static`)

## Step 4: Code Coverage Analysis
Measure line and branch coverage:
- **LCOV HTML Report**: `make cov` (or `make container-cov`)

## Step 5: Doxygen API Documentation Coverage
Verify API documentation completeness:
- **Doxygen Check**: `make dox` (or `make container-dox`)

## Step 6: Markdown Documentation
Inspect project documentation (`README.md`, `docs/`, `mkdocs.yml`) for structural consistency.