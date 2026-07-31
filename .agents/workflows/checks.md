---
description: Automated Fast Quality & Core Testing Checks Workflow
---

# SyntropicOS Core Checks Workflow

Fast, fail-fast static verification and core test execution. Always execute the containerized target (`make container-*`) for each step.

## Step 0: Fast Static Analysis & Code Formatting
- **Code Formatting**: `make container-format`
- **Static Analysis & MISRA**: `make container-static`

## Step 1: Unit Testing
- **Unit Test Suite**: `make container-test`

## Step 2: Bare-Metal ARM Emulation
- **QEMU Bare-Metal ARM Cortex-M4**: `make container-qemu`

## Step 3: Examples Cross-Compilation Check
- **Examples Cross-Compilation**: `make container-examples`