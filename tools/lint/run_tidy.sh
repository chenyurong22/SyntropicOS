#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"

echo "=== SyntropicOS AST Code Quality Linter (clang-tidy) ==="

if ! command -v clang-tidy >/dev/null 2>&1; then
    echo "Notice: clang-tidy not found on host. Run via container: make container-lint"
    exit 0
fi

JOBS=$(nproc 2>/dev/null || echo 4)
CFLAGS="-std=c99 -D_DEFAULT_SOURCE -I. -Isrc -Itests/unit -Itests/unit/mocks -DSYN_LOG_COLOR=1 -DSYN_USE_COREDUMP=1 -DSYN_USE_TICKLESS=1"

FILES=$(find src/syntropic -name "*.c" ! -path "*/port_stubs/*")

echo "Running clang-tidy across ${FILES} with ${JOBS} parallel workers..."
printf "%s\n" ${FILES} | xargs -n 1 -P "${JOBS}" -I {} clang-tidy {} -- ${CFLAGS} || true

echo "=== Running Protothread Switch Linter ==="
python3 tools/lint/check_pt_switch.py

echo "=== Clang-Tidy Complete ==="
