#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"

echo "=== SyntropicOS AST Code Quality Linter (clang-tidy) ==="

if ! command -v clang-tidy >/dev/null 2>&1; then
    echo "Notice: clang-tidy not found on host. Run via container: make container-lint"
    exit 0
fi

CFLAGS="-std=c99 -D_DEFAULT_SOURCE -I. -Isrc -Itests/unit -Itests/unit/mocks -DSYN_LOG_COLOR=1 -DSYN_USE_COREDUMP=1 -DSYN_USE_TICKLESS=1"

FILES=$(find src/syntropic -name "*.c" ! -path "*/port_stubs/*")

clang-tidy ${FILES} -- ${CFLAGS} || true

echo "=== Clang-Tidy Complete ==="
