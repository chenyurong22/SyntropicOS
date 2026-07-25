#!/usr/bin/env bash
set -e

# SyntropicOS Binary Memory Footprint & Section Budget Analyzer
# Analyzes Flash (.text + .rodata) and RAM (.data + .bss) allocation per subsystem.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"

BUILD_DIR="build/size_analysis"
mkdir -p "${BUILD_DIR}"

echo "=== SyntropicOS Binary Footprint & Subsystem Memory Audit ==="

if command -v arm-none-eabi-gcc >/dev/null 2>&1; then
    CC_TOOL="arm-none-eabi-gcc"
    SIZE_TOOL="arm-none-eabi-size"
else
    CC_TOOL="gcc"
    SIZE_TOOL="size"
fi

# Find source files
FILES=$(find src/syntropic -name "*.c" ! -path "*/port_stubs/*")

# Compile objects
for f in ${FILES}; do
    subsys=$(echo "${f}" | awk -F'/' '{print $(NF-1)}')
    mkdir -p "${BUILD_DIR}/${subsys}"
    obj="${BUILD_DIR}/${subsys}/$(basename "${f}" .c).o"
    "${CC_TOOL}" -std=c99 -Iinclude -Isrc -Isrc/syntropic -c "${f}" -o "${obj}" 2>/dev/null || true
done

echo ""
printf "%-20s %-12s %-12s %-12s %-12s\n" "Subsystem" "Text (Code)" "Data (RAM)" "BSS (RAM)" "Total (Bytes)"
echo "-----------------------------------------------------------------------"

TOTAL_TEXT=0
TOTAL_DATA=0
TOTAL_BSS=0

# Iterate through subsystems
for subsys_dir in "${BUILD_DIR}"/*; do
    if [ -d "${subsys_dir}" ]; then
        subsys=$(basename "${subsys_dir}")
        subsys_text=0
        subsys_data=0
        subsys_bss=0

        for obj in "${subsys_dir}"/*.o; do
            if [ -f "${obj}" ]; then
                # Parse size output: text data bss dec hex filename
                read text data bss _ < <("${SIZE_TOOL}" "${obj}" | tail -n 1)
                subsys_text=$((subsys_text + text))
                subsys_data=$((subsys_data + data))
                subsys_bss=$((subsys_bss + bss))
            fi
        done

        subsys_total=$((subsys_text + subsys_data + subsys_bss))
        TOTAL_TEXT=$((TOTAL_TEXT + subsys_text))
        TOTAL_DATA=$((TOTAL_DATA + subsys_data))
        TOTAL_BSS=$((TOTAL_BSS + subsys_bss))

        printf "%-20s %-12d %-12d %-12d %-12d\n" "${subsys}" "${subsys_text}" "${subsys_data}" "${subsys_bss}" "${subsys_total}"
    fi
done

GRAND_TOTAL=$((TOTAL_TEXT + TOTAL_DATA + TOTAL_BSS))
echo "-----------------------------------------------------------------------"
printf "%-20s %-12d %-12d %-12d %-12d\n" "TOTAL" "${TOTAL_TEXT}" "${TOTAL_DATA}" "${TOTAL_BSS}" "${GRAND_TOTAL}"
echo "======================================================================="

# Cleanup
rm -rf "${BUILD_DIR}"
