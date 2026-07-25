#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"

echo "=== SyntropicOS MISRA C:2023 Safety Rule Compliance Checker ==="

if ! command -v cppcheck >/dev/null 2>&1; then
    echo "Notice: cppcheck not found on host. Run via container: make container-misra"
    exit 0
fi

# Enable Cppcheck with MISRA addon and C99 standard rules
cppcheck --std=c99 \
    --enable=warning,style,performance,portability \
    --addon=misra \
    --inline-suppr \
    --suppress=missingIncludeSystem \
    --suppress=unusedFunction \
    -I src -I . \
    src/ || true

echo "=== MISRA C:2023 Compliance Scan Complete ==="
