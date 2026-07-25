#!/usr/bin/env bash
set -euo pipefail

# Script to run Doxygen documentation coverage analysis
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

cd "${ROOT_DIR}"

echo "=== Running Doxygen Documentation Coverage Analysis ==="
if command -v doxygen >/dev/null 2>&1; then
    rm -f doxygen_warnings.txt
    doxygen Doxyfile
    
    if [ -f doxygen_warnings.txt ]; then
        WARN_COUNT=$(wc -l < doxygen_warnings.txt || echo 0)
        echo "=== Doxygen Analysis Finished ==="
        echo "Documentation Warning Count: ${WARN_COUNT}"
        if [ "${WARN_COUNT}" -gt 0 ]; then
            echo "--- Sample Doxygen Warnings (first 10) ---"
            head -n 10 doxygen_warnings.txt
            echo "------------------------------------------"
        else
            echo "=== 100% Doxygen Documentation Coverage Clean ==="
        fi
    else
        echo "=== Doxygen Execution Clean ==="
    fi
else
    echo "Notice: doxygen not found on host. Run via container: make -C tools/containers container-dox"
fi
