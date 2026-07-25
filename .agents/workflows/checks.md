---
description: Automated Quality & Testing Checks Workflow
---

# SyntropicOS Comprehensive Checks Workflow

Follow this sequence to verify software correctness, memory safety, static analysis, coverage, and documentation.

## Step 1: Unit Testing & Dynamic Sanitizers
Run host and containerized test suites:
- **Unit Test Suite**: `make -f tests/Makefile.unity test`
- **Containerized Sanitizer Audit**: `make -C tools/containers container-san`
- **QEMU Bare-Metal Emulation**: `make -C tools/containers container-qemu`
- **Protocol libFuzzer Targets**: `make -C tools/containers container-fuzz`

## Step 2: Static Analysis
Run static analysis checks:
- **Containerized Cppcheck & scan-build**: `make -C tools/containers container-static`

## Step 3: Code Coverage Analysis
Measure line and branch coverage:
- **Containerized LCOV HTML Report**: `make -C tools/containers container-cov`

## Step 4: Doxygen API Documentation Coverage
Verify API documentation completeness:
- **Containerized Doxygen Check**: `make -C tools/containers container-dox`

## Step 5: Markdown Documentation
Inspect project documentation (`README.md`, `docs/`, `mkdocs.yml`) for structural consistency.