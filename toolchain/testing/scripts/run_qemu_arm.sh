#!/usr/bin/env bash
set -euo pipefail

# Script to cross-compile SyntropicOS for ARM Cortex-M4 and execute in QEMU
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
QEMU_DIR="${ROOT_DIR}/toolchain/testing/qemu_arm"

cd "${ROOT_DIR}"

echo "=== Cross-compiling SyntropicOS for ARM Cortex-M4 (bare-metal) ==="

# Gather source C files, excluding port_stubs, syn_wg.c, syn_hpclock.c, syn_timesync.c
SRC_FILES=$(find src/syntropic -name "*.c" ! -path "*/port_stubs/*" ! -name "syn_wg.c" ! -name "syn_hpclock.c" ! -name "syn_timesync.c")

# Gather test files, excluding host-only Linux socket tests and standalone main binaries
TEST_FILES=$(find tests -name "test_*.c" ! -name "test_wg_integration.c" ! -name "test_geo.c" ! -name "test_hpclock.c" ! -name "test_timesync.c")

arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -std=c99 -pedantic -Wall -Wextra \
    -O2 -fno-unwind-tables -fno-asynchronous-unwind-tables -I. -Isrc -Itests -Itests/mocks \
    -DSYN_LOG_COLOR=0 -DSYN_USE_COREDUMP=1 -DSYN_COREDUMP_FLASH_ADDR=0 -DSYN_USE_TICKLESS=1 -DSYN_USE_DMA=1 -DSYN_USE_I2C_ASYNC=1 -DSYN_USE_SPI_ASYNC=1 -DSYN_FW_USE_HMAC=1 -DSYN_USE_MULTICORE=1 -DUNITY_INCLUDE_DOUBLE -DSYN_USE_METRICS=1 -DSYN_USE_ROUTER=1 -DSYN_USE_LIN=1 -DSYN_USE_IR=1 -DSYN_USE_SMBUS=1 -DSYN_USE_PMBUS=1 \
    -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-format -Wno-stringop-truncation -Wno-type-limits \
    -T "${QEMU_DIR}/qemu_arm.ld" \
    -specs=rdimon.specs \
    "${QEMU_DIR}/startup_cortexm4.c" \
    ${SRC_FILES} \
    tests/unity/unity.c \
    tests/mocks/mock_port.c \
    ${TEST_FILES} \
    -o test_cortexm4.elf -lm

echo "=== Executing in QEMU ARM Cortex-M4 (mps2-an385) ==="
if command -v qemu-system-arm >/dev/null 2>&1; then
    timeout 10s qemu-system-arm -machine mps2-an385 -nographic -semihosting -kernel test_cortexm4.elf || true
    echo "=== QEMU ARM Execution Complete ==="
else
    echo "Notice: qemu-system-arm not found on host. Run via container: make -C toolchain/testing container-qemu"
fi
