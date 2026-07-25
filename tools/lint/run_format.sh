#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"

MODE="check"
if [[ $# -gt 0 && "$1" == "--fix" ]]; then
    MODE="fix"
fi

echo "=== SyntropicOS Code Formatting Check (clang-format) ==="

if ! command -v clang-format >/dev/null 2>&1; then
    echo "Notice: clang-format not found on host. Run via container: make container-format"
    exit 0
fi

FILES=$(find src port tests -name "*.c" -o -name "*.h" ! -path "*/unity/*")

if [[ "${MODE}" == "fix" ]]; then
    echo "Applying formatting fixes..."
    clang-format -i ${FILES}
    echo "=== Formatting Complete ==="
else
    echo "Checking code formatting..."
    UNFORMATTED=$(clang-format --dry-run --Werror ${FILES} 2>&1 || true)
    if [[ -n "${UNFORMATTED}" ]]; then
        echo "${UNFORMATTED}"
        echo ""
        echo "Notice: Formatting issues found. Run 'make format FIX=1' to auto-fix."
    else
        echo "=== Code Formatting PASS ==="
    fi
fi
