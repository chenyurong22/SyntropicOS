#!/usr/bin/env bash
set -e

# SyntropicOS GCC -fanalyzer Interprocedural Static Analysis Runner
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"

echo "=== SyntropicOS GCC Interprocedural Static Analyzer (-fanalyzer) ==="

FILES=$(find src/syntropic -name "*.c" ! -path "*/port_stubs/*")
BUILD_DIR="build/analyzer"
mkdir -p "${BUILD_DIR}"

ERR_COUNT=0
for f in ${FILES}; do
    obj="${BUILD_DIR}/$(basename "${f}" .c).o"
    gcc -std=c99 -pedantic -Wall -Wextra -Werror \
        -I. -Isrc -Isrc/syntropic \
        -DSYN_USE_TICKLESS=1 -DSYN_USE_DMA=1 -DSYN_USE_I2C_ASYNC=1 \
        -fanalyzer -c "${f}" -o "${obj}" 2>&1 || ERR_COUNT=$((ERR_COUNT+1))
done

if [ ${ERR_COUNT} -eq 0 ]; then
    echo "=== GCC -fanalyzer Interprocedural Analysis Completed Cleanly (0 Bugs) ==="
else
    echo "Warning: GCC -fanalyzer detected ${ERR_COUNT} warnings/errors."
    exit 1
fi
