---
description: Code Coverage Measurement & LCOV Report Workflow
---

# SyntropicOS Code Coverage Workflow

Follow this sequence to measure line and branch coverage across the unit test suite and generate an HTML report:

## Step 0: Run Instrumented Coverage Analysis
Execute the containerized or host LCOV coverage build:
- **Containerized Coverage Report**: `make container-cov`
- **Host Coverage Report**: `make cov`

## Step 1: Inspect HTML Report Artifacts
Review the generated report in `coverage_html/index.html` to identify untested functions, branch branches, or edge cases.
