#!/usr/bin/env bash
set -e

# SyntropicOS Binary Memory Footprint & Section Budget Analyzer
# Analyzes Flash (.text + .rodata) and RAM (.data + .bss) allocation per subsystem.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"

BUILD_DIR="build/size_analysis"
mkdir -p "${BUILD_DIR}"

if command -v arm-none-eabi-gcc >/dev/null 2>&1; then
    CC_TOOL="arm-none-eabi-gcc"
    SIZE_TOOL="arm-none-eabi-size"
    CFLAGS="-std=c99 -mcpu=cortex-m4 -mthumb -Os -Iinclude -Isrc -Isrc/syntropic"
    TARGET_DESC="ARM Cortex-M4 (-mcpu=cortex-m4 -mthumb -Os)"
else
    CC_TOOL="gcc"
    SIZE_TOOL="size"
    CFLAGS="-std=c99 -Os -Iinclude -Isrc -Isrc/syntropic"
    TARGET_DESC="Host GCC (-Os)"
fi

echo "=== SyntropicOS Microcontroller Binary Footprint & Subsystem Memory Audit ==="
echo "Target Architecture: ${TARGET_DESC}"
echo ""

FILES=$(find src/syntropic -name "*.c" ! -path "*/port_stubs/*")

# Compile size-optimized objects
for f in ${FILES}; do
    subsys=$(echo "${f}" | awk -F'/' '{print $(NF-1)}')
    mkdir -p "${BUILD_DIR}/${subsys}"
    obj="${BUILD_DIR}/${subsys}/$(basename "${f}" .c).o"
    "${CC_TOOL}" ${CFLAGS} -c "${f}" -o "${obj}" 2>/dev/null || true
done

printf "| %-18s | %-12s | %-12s | %-12s | %-14s |\n" "Subsystem" "Flash (Text)" "RAM (Data)" "RAM (BSS)" "Total (Bytes)"
printf "|--------------------|--------------|--------------|--------------|----------------|\n"

TOTAL_TEXT=0
TOTAL_DATA=0
TOTAL_BSS=0

# Iterate through subsystems in alphabetical order
for subsys_dir in $(ls -d "${BUILD_DIR}"/* | sort); do
    if [ -d "${subsys_dir}" ]; then
        subsys=$(basename "${subsys_dir}")
        subsys_text=0
        subsys_data=0
        subsys_bss=0

        for obj in "${subsys_dir}"/*.o; do
            if [ -f "${obj}" ]; then
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

        printf "| %-18s | %-12d | %-12d | %-12d | %-14d |\n" "${subsys}" "${subsys_text}" "${subsys_data}" "${subsys_bss}" "${subsys_total}"
    fi
done

GRAND_TOTAL=$((TOTAL_TEXT + TOTAL_DATA + TOTAL_BSS))
printf "|--------------------|--------------|--------------|--------------|----------------|\n"
printf "| %-18s | %-12d | %-12d | %-12d | %-14d |\n" "TOTAL" "${TOTAL_TEXT}" "${TOTAL_DATA}" "${TOTAL_BSS}" "${GRAND_TOTAL}"

# Clean up
rm -rf "${BUILD_DIR}"

