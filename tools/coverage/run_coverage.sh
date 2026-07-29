#!/usr/bin/env bash
set -euo pipefail

# Script to run test coverage analysis and generate HTML reports
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

cd "${ROOT_DIR}"

echo "=== Preparing Coverage Environment ==="
rm -rf build/cov
mkdir -p build/cov
rm -f *.gcov

echo "=== Running Test Suite with Coverage Instrumentation ==="
JOBS=${JOBS:-$(nproc 2>/dev/null || echo 4)}
make -j"${JOBS}" -f tests/Makefile.unity test-cov BUILD_DIR=build/cov

echo "=== Generating HTML Coverage Report ==="
mkdir -p coverage_html
if command -v lcov >/dev/null 2>&1; then
    lcov --capture --ignore-errors inconsistent,path --directory build/cov --output-file build/cov/coverage.info --quiet
    lcov --extract build/cov/coverage.info "*/src/*" --ignore-errors inconsistent,path --output-file build/cov/coverage_src.info --quiet
    genhtml build/cov/coverage_src.info --ignore-errors inconsistent,path --output-directory build/cov/html --title "SyntropicOS Coverage" --quiet
    cp -r build/cov/html/* coverage_html/ 2>/dev/null || true
    echo "=== Coverage HTML report generated in coverage_html/index.html ==="
elif command -v gcovr >/dev/null 2>&1; then
    mkdir -p build/cov/html
    gcovr -r . --object-directory build/cov --html --html-details -o build/cov/html/index.html -e "tests/.*" -e "tools/.*"
    cp -r build/cov/html/* coverage_html/ 2>/dev/null || true
    echo "=== Coverage HTML report generated via gcovr in coverage_html/index.html ==="
else
    echo "Notice: Neither lcov nor gcovr found. Run via container: make -C tools/containers container-cov"
fi
rm -f *.gcov
