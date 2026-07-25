#!/usr/bin/env bash
set -e

# SyntropicOS Stack Usage Analyzer (-fstack-usage)
# Analyzes static stack frame allocations across source files.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"

STACK_THRESHOLD_BYTES=${STACK_THRESHOLD_BYTES:-256}
BUILD_DIR="build/stack_analysis"
mkdir -p "${BUILD_DIR}"

echo "=== SyntropicOS Stack Frame Usage Analysis ==="
echo "Max Stack Frame Threshold: ${STACK_THRESHOLD_BYTES} bytes"

# Find all C source files
FILES=$(find src/syntropic -name "*.c" ! -path "*/port_stubs/*")

# Compile with -fstack-usage flag to generate .su files
gcc -std=c99 -Iinclude -Isrc -fstack-usage -c ${FILES} -o /dev/null 2>/dev/null || true

# Collect all .su files
SU_FILES=$(find . -maxdepth 2 -name "*.su" -o -name "*.su" 2>/dev/null || true)

if [ -z "${SU_FILES}" ]; then
    echo "Notice: No .su files generated. Compiling in ${BUILD_DIR}..."
    for f in ${FILES}; do
        obj="${BUILD_DIR}/$(basename "${f}" .c).o"
        gcc -std=c99 -Iinclude -Isrc -Isrc/syntropic -fstack-usage -c "${f}" -o "${obj}" 2>/dev/null || true
    done
    SU_FILES=$(find "${BUILD_DIR}" -name "*.su" 2>/dev/null || true)
fi

echo ""
echo "--- Top 15 Stack Frame Allocations ---"
printf "%-40s %-30s %-12s %s\n" "File" "Function" "Stack Size" "Type"
echo "--------------------------------------------------------------------------------------------------"

OUT_FILE="${BUILD_DIR}/su_results.txt"
rm -f "${OUT_FILE}"

for su in ${SU_FILES}; do
    while IFS= read -r line; do
        if [ -n "${line}" ]; then
            location=$(echo "${line}" | awk '{print $1}')
            func=$(echo "${location}" | awk -F: '{print $NF}')
            file=$(echo "${location}" | awk -F: '{print $1}')
            size=$(echo "${line}" | awk '{print $2}')
            type=$(echo "${line}" | awk '{print $3}')

            if [ -n "${size}" ] && [ "${size}" -eq "${size}" ] 2>/dev/null; then
                printf "%-8d %-35s %-30s %s\n" "${size}" "${file}" "${func}()" "${type}" >> "${OUT_FILE}"
            fi
        fi
    done < "${su}"
done

if [ -f "${OUT_FILE}" ]; then
    sort -nr -k1 "${OUT_FILE}" | head -n 15 | while read -r sz f fn t; do
        if [ "${sz}" -gt "${STACK_THRESHOLD_BYTES}" ]; then
            echo "WARNING: ${f} ${fn} uses ${sz} bytes (exceeds ${STACK_THRESHOLD_BYTES}B threshold)"
        else
            echo "OK: ${f} ${fn} uses ${sz} bytes"
        fi
    done
    EXCEEDED_COUNT=$(awk -v limit="${STACK_THRESHOLD_BYTES}" '$1 > limit { c++ } END { print c+0 }' "${OUT_FILE}")
fi

# Clean up .su files
find . -maxdepth 2 -name "*.su" -delete 2>/dev/null || true
rm -rf "${BUILD_DIR}"

echo ""
if [ "${EXCEEDED_COUNT}" -gt 0 ]; then
    echo "=== Stack Usage Scan Complete: ${EXCEEDED_COUNT} functions exceeded ${STACK_THRESHOLD_BYTES}B threshold ==="
else
    echo "=== Stack Usage Scan PASS: All functions within ${STACK_THRESHOLD_BYTES}B frame budget ==="
fi
