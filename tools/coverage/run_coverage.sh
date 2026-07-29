#!/usr/bin/env bash
set -euo pipefail

# Script to run test coverage analysis and generate HTML reports
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

cd "${ROOT_DIR}"

echo "=== Cleaning previous build artifacts & coverage data ==="
make -f tests/Makefile.unity clean || true
rm -rf build/coverage coverage.info coverage_html coverage_src.info || true
mkdir -p build/coverage

echo "=== Running Test Suite with Coverage Instrumentation ==="
JOBS=$(nproc 2>/dev/null || true)
if [ -z "${JOBS}" ]; then
    JOBS=4
fi
make -j"${JOBS}" -f tests/Makefile.unity test-cov

echo "=== Generating HTML Coverage Report via LCOV ==="
if command -v lcov >/dev/null 2>&1; then
    lcov --capture --directory . --output-file build/coverage/coverage.info --quiet
    lcov --extract build/coverage/coverage.info "*/src/*" --output-file build/coverage/coverage_src.info --quiet
    genhtml build/coverage/coverage_src.info --output-directory build/coverage/html --title "SyntropicOS Coverage" --quiet
    echo "=== Coverage HTML report generated in build/coverage/html/index.html ==="
elif command -v gcovr >/dev/null 2>&1; then
    mkdir -p build/coverage/html
    gcovr -r . --html --html-details -o build/coverage/html/index.html -e "tests/.*" -e "tools/.*"
    echo "=== Coverage HTML report generated via gcovr in build/coverage/html/index.html ==="
else
    echo "Notice: Neither lcov nor gcovr found. Run via container: make -C tools/containers container-cov"
fi
