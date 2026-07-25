#!/usr/bin/env bash
set -euo pipefail

# Script to run test coverage analysis and generate HTML reports
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

cd "${ROOT_DIR}"

echo "=== Cleaning previous build artifacts & coverage data ==="
make -f tests/Makefile.unity clean || true
rm -rf coverage.info coverage_html coverage_src.info

echo "=== Running Test Suite with Coverage Instrumentation ==="
make -f tests/Makefile.unity test-cov

echo "=== Generating HTML Coverage Report via LCOV ==="
if command -v lcov >/dev/null 2>&1; then
    lcov --capture --directory . --output-file coverage.info --quiet
    lcov --extract coverage.info "*/src/*" --output-file coverage_src.info --quiet
    genhtml coverage_src.info --output-directory coverage_html --title "SyntropicOS Coverage" --quiet
    echo "=== Coverage HTML report generated in coverage_html/index.html ==="
elif command -v gcovr >/dev/null 2>&1; then
    mkdir -p coverage_html
    gcovr -r . --html --html-details -o coverage_html/index.html -e "tests/.*" -e "tools/.*"
    echo "=== Coverage HTML report generated via gcovr in coverage_html/index.html ==="
else
    echo "Notice: Neither lcov nor gcovr found. Run via container: make -C tools/containers container-cov"
fi
