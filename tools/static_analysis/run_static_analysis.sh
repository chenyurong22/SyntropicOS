#!/usr/bin/env bash
set -euo pipefail

# Script to run Cppcheck static analysis and Clang scan-build
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

cd "${ROOT_DIR}"

JOBS=$(nproc 2>/dev/null || echo 4)

mkdir -p build/cppcheck
echo "=== Running Cppcheck Static Analysis on src/ ==="
if command -v cppcheck >/dev/null 2>&1; then
    cppcheck --std=c99 -j "${JOBS}" \
        --cppcheck-build-dir=build/cppcheck \
        --enable=warning,style,performance,portability \
        --inline-suppr \
        --suppress=missingIncludeSystem \
        --suppress=unusedFunction \
        -I src -I . \
        src/ || true
    echo "=== Cppcheck Analysis Complete ==="
else
    echo "Notice: cppcheck not found on host. Run via container: make -C tools/containers container-static"
fi

echo "=== Running Clang Static Analyzer (scan-build) ==="
if command -v scan-build >/dev/null 2>&1; then
    scan-build -o /tmp/scan-build-out --status-bugs make -j "${JOBS}" -f tests/Makefile.unity test || {
        echo "=== SCAN-BUILD BUG DETAILS ==="
        find /tmp/scan-build-out -name "*.html" -exec grep -H -C 3 "BUGDESC" {} + || true
        find /tmp/scan-build-out -name "*.html" -exec grep -H -C 3 "td class=\"DESC\"" {} + || true
        exit 1
    }
    echo "=== Clang Static Analyzer Complete ==="
else
    echo "Notice: scan-build not found on host. Run via container: make -C tools/containers container-static"
fi
