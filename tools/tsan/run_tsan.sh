#!/usr/bin/env bash
set -e

# SyntropicOS ThreadSanitizer (TSan) Data Race Analysis Runner
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"

echo "=== SyntropicOS ThreadSanitizer (TSan) Multithread & Data Race Analysis ==="

BUILD_DIR="build/tests_tsan"
mkdir -p "${BUILD_DIR}"
make -f tests/Makefile.unity wasm-fixtures BUILD_DIR=build/tests

# Find all C files
SRC_FILES=$(find src/syntropic -name "*.c" ! -path "*/port_stubs/*" ! -name "syn_wg.c" ! -name "syn_hpclock.c" ! -name "syn_timesync.c" ! -name "syn_lfs.c")
PORT_FILES="src/port/posix/port_posix_socket.c"

# Compile with ThreadSanitizer flags
gcc -std=c99 -g -fsanitize=thread -pthread -O1 \
    -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE \
    -I. -Isrc -Itests/unit -Itests/unit/mocks -Ibuild/tests \
    -DSYN_LOG_COLOR=1 -DSYN_USE_TICKLESS=1 -DSYN_USE_DMA=1 -DSYN_USE_MULTICORE=1 \
    ${SRC_FILES} ${PORT_FILES} \
    tests/unit/unity/unity.c \
    tests/unit/mocks/mock_port.c \
    tests/unit/test_runner.c \
    -o "${BUILD_DIR}/test_tsan" -lm

# Run under ThreadSanitizer
TSAN_OPTIONS="halt_on_error=1" "${BUILD_DIR}/test_tsan"

echo "=== ThreadSanitizer (TSan) Audit Completed Cleanly (0 Data Races) ==="
