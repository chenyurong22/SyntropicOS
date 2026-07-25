#!/usr/bin/env bash
set -euo pipefail

# Script to run full SyntropicOS test suite under AddressSanitizer and UndefinedBehaviorSanitizer
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"

cd "${ROOT_DIR}"

echo "=== Running SyntropicOS Test Suite under UBSan ==="
make -f tests/Makefile.unity clean
CFLAGS="-std=c99 -pedantic -Wall -Wextra -Werror -I. -Isrc -Itests -Itests/mocks -DSYN_LOG_COLOR=1 -DSYN_USE_COREDUMP=1 -DSYN_COREDUMP_FLASH_ADDR=0 -DSYN_USE_TICKLESS=1 -DSYN_USE_DMA=1 -DSYN_USE_I2C_ASYNC=1 -DSYN_USE_SPI_ASYNC=1 -DSYN_FW_USE_HMAC=1 -DSYN_USE_MULTICORE=1 -DUNITY_INCLUDE_DOUBLE -DSYN_USE_METRICS=1 -DSYN_USE_ROUTER=1 -DSYN_USE_LIN=1 -DSYN_USE_IR=1 -DSYN_USE_SMBUS=1 -DSYN_USE_PMBUS=1 -fsanitize=undefined" make -f tests/Makefile.unity

echo "=== Sanitizer Analysis Clean ==="
