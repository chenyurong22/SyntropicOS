---
description: Code & Documentation Coverage Workflow
---

# SyntropicOS Code & Documentation Coverage Workflow

Measure line/branch code coverage, verify API documentation completeness, and ensure new/modified modules and features have full documentation coverage.

## Step 0: Run Instrumented Code Coverage Analysis
Execute the containerized LCOV coverage build:
- **Containerized Coverage Report**: `make container-cov`

## Step 1: Inspect Coverage Summary & Uncovered Branches
Review the terminal coverage summary and `build/cov/coverage_src.info` output to identify untested functions, unreached branches, or edge cases.

## Step 2: Doxygen API Documentation Coverage
Verify API documentation completeness and check for missing symbol docs:
- **Doxygen Coverage Check**: `make container-dox`

## Step 3: Module & Feature Documentation Audit
Audit markdown documentation (`docs/`, `docs/modules/`, `README.md`, `mkdocs.yml`) for feature coverage:
- **New Module Coverage**: Verify every new or modified source file (`src/syntropic/*`, `src/port/*`) has matching documentation in `docs/modules/`.
- **API Reference Sync**: Ensure all newly exposed public APIs are documented with signatures, parameters, return values, and usage notes.
- **Example Coverage**: Verify new features or drivers have corresponding example sketches in `examples/`.
