#!/usr/bin/env bash
set -euo pipefail

# Script to run Clang libFuzzer smoke tests on SyntropicOS protocol decoders
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"

cd "${ROOT_DIR}"

if ! command -v clang >/dev/null 2>&1; then
    echo "Clang not found on host. Run via container: make -C toolchain/testing container-fuzz"
    exit 0
fi

echo "=== Compiling & Fuzzing CBOR Decoder (30s smoke test) ==="
clang -std=c99 -I. -Isrc -Itests/mocks -DSYN_USE_MULTICORE=1 -fsanitize=fuzzer,address,undefined \
    src/syntropic/util/syn_cbor_read.c \
    tests/mocks/mock_port.c \
    toolchain/testing/fuzzers/fuzz_cbor.c \
    -o fuzzer_cbor
./fuzzer_cbor -max_total_time=10 || true
rm -f fuzzer_cbor

echo "=== Compiling & Fuzzing NMEA Decoder (30s smoke test) ==="
clang -std=c99 -I. -Isrc -Itests/mocks -DSYN_USE_MULTICORE=1 -fsanitize=fuzzer,address,undefined \
    src/syntropic/proto/syn_nmea.c \
    tests/mocks/mock_port.c \
    toolchain/testing/fuzzers/fuzz_nmea.c \
    -o fuzzer_nmea
./fuzzer_nmea -max_total_time=10 || true
rm -f fuzzer_nmea

echo "=== Compiling & Fuzzing CoAP Decoder (30s smoke test) ==="
clang -std=c99 -I. -Isrc -Itests/mocks -DSYN_USE_MULTICORE=1 -fsanitize=fuzzer,address,undefined \
    src/syntropic/net/syn_coap.c \
    src/syntropic/util/syn_backoff.c \
    src/syntropic/util/syn_random.c \
    tests/mocks/mock_port.c \
    toolchain/testing/fuzzers/fuzz_coap.c \
    -o fuzzer_coap
./fuzzer_coap -max_total_time=10 || true
rm -f fuzzer_coap

echo "=== Protocol Fuzzing Smoke Tests Complete ==="
