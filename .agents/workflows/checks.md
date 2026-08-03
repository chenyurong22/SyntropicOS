---
description: Automated Fast Quality & Core Testing Checks Workflow
---

# SyntropicOS Core Checks Workflow

Fast, fail-fast static verification and core test execution. Always execute the containerized target (`make container-*`) for each step.

**Completion Verification**: Every `make container-*` target MUST terminate on its own. Do NOT declare a step passed based on partial output from a still-running process. Wait for the command to exit, verify exit code 0, and confirm the final summary line exists (e.g., `Tests 0 Failures` / `PASS` / `No bugs found`). A hung container is a test failure.

**Reporting Requirement**: Always explicitly report the exact test pass/fail summary metrics (e.g., `X Tests 0 Failures 0 Ignored / OK`) in the workflow output for both Step 1 (Unit Testing) and Step 2 (Bare-Metal ARM QEMU Emulation).

## Step 0: Fast Static Analysis & Code Formatting
- **Code Formatting**: `make container-format`
- **Static Analysis & MISRA**: `make container-static`

## Step 1: Unit Testing
- **Unit Test Suite**: `make container-test` (MUST report exact `X Tests 0 Failures 0 Ignored` metric summary)

## Step 2: Bare-Metal ARM Emulation
- **QEMU Bare-Metal ARM Cortex-M4**: `make container-qemu` (MUST report exact `X Tests 0 Failures 0 Ignored` metric summary from QEMU output)

## Step 3: Examples Cross-Compilation Check
- **Examples Cross-Compilation**: `make container-examples`