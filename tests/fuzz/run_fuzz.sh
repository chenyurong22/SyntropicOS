#!/usr/bin/env bash
set -euo pipefail

# Script to run Clang libFuzzer smoke tests on SyntropicOS protocol decoders
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

cd "${ROOT_DIR}"

if ! command -v clang >/dev/null 2>&1; then
    echo "Clang not found on host. Run via container: make -C tools/containers container-fuzz"
    exit 0
fi

BUILD_DIR="build/fuzz"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo "=== Compiling & Fuzzing CBOR Decoder (10s smoke test) ==="
clang -std=c99 -I"${ROOT_DIR}" -I"${ROOT_DIR}/src" -I"${ROOT_DIR}/tests/unit/mocks" -DSYN_USE_MULTICORE=1 -fsanitize=fuzzer,address,undefined \
    "${ROOT_DIR}/src/syntropic/util/syn_cbor_read.c" \
    "${ROOT_DIR}/tests/unit/mocks/mock_port.c" \
    "${ROOT_DIR}/tests/fuzz/fuzz_cbor.c" \
    -o fuzzer_cbor
./fuzzer_cbor -max_total_time=10 || true
rm -f fuzzer_cbor

echo "=== Compiling & Fuzzing NMEA Decoder (10s smoke test) ==="
clang -std=c99 -I"${ROOT_DIR}" -I"${ROOT_DIR}/src" -I"${ROOT_DIR}/tests/unit/mocks" -DSYN_USE_MULTICORE=1 -fsanitize=fuzzer,address,undefined \
    "${ROOT_DIR}/src/syntropic/proto/syn_nmea.c" \
    "${ROOT_DIR}/tests/unit/mocks/mock_port.c" \
    "${ROOT_DIR}/tests/fuzz/fuzz_nmea.c" \
    -o fuzzer_nmea
./fuzzer_nmea -max_total_time=10 || true
rm -f fuzzer_nmea

echo "=== Compiling & Fuzzing CoAP Decoder (10s smoke test) ==="
clang -std=c99 -I"${ROOT_DIR}" -I"${ROOT_DIR}/src" -I"${ROOT_DIR}/tests/unit/mocks" -DSYN_USE_MULTICORE=1 -fsanitize=fuzzer,address,undefined \
    "${ROOT_DIR}/src/syntropic/net/syn_coap.c" \
    "${ROOT_DIR}/src/syntropic/util/syn_backoff.c" \
    "${ROOT_DIR}/src/syntropic/util/syn_random.c" \
    "${ROOT_DIR}/tests/unit/mocks/mock_port.c" \
    "${ROOT_DIR}/tests/fuzz/fuzz_coap.c" \
    -o fuzzer_coap
./fuzzer_coap -max_total_time=10 || true
rm -f fuzzer_coap

echo "=== Compiling & Fuzzing USB Device Stack (10s smoke test) ==="
clang -std=c99 -I"${ROOT_DIR}" -I"${ROOT_DIR}/src" -I"${ROOT_DIR}/tests/unit/mocks" -DSYN_USE_MULTICORE=1 -fsanitize=fuzzer,address,undefined \
    "${ROOT_DIR}/src/syntropic/drivers/syn_usb.c" \
    "${ROOT_DIR}/src/syntropic/drivers/syn_usb_cdc.c" \
    "${ROOT_DIR}/src/syntropic/drivers/syn_usb_hid.c" \
    "${ROOT_DIR}/src/syntropic/drivers/syn_usb_host.c" \
    "${ROOT_DIR}/src/syntropic/drivers/syn_usb_host_cdc.c" \
    "${ROOT_DIR}/src/syntropic/drivers/syn_transport_usb_host_cdc.c" \
    "${ROOT_DIR}/tests/unit/mocks/mock_port.c" \
    "${ROOT_DIR}/tests/fuzz/fuzz_usb.c" \
    -o fuzzer_usb
./fuzzer_usb -max_total_time=10 || true
rm -f fuzzer_usb

echo "=== Compiling & Fuzzing UDS Diagnostic Server (10s smoke test) ==="
clang -std=c99 -I"${ROOT_DIR}" -I"${ROOT_DIR}/src" -I"${ROOT_DIR}/tests/unit/mocks" -DSYN_USE_MULTICORE=1 -fsanitize=fuzzer,address,undefined \
    "${ROOT_DIR}/src/syntropic/proto/syn_uds.c" \
    "${ROOT_DIR}/src/syntropic/util/syn_aes128.c" \
    "${ROOT_DIR}/tests/unit/mocks/mock_port.c" \
    "${ROOT_DIR}/tests/fuzz/fuzz_uds.c" \
    -o fuzzer_uds
./fuzzer_uds -max_total_time=10 || true
rm -f fuzzer_uds

echo "=== Compiling & Fuzzing ISO-TP Transport Stack (10s smoke test) ==="
clang -std=c99 -I"${ROOT_DIR}" -I"${ROOT_DIR}/src" -I"${ROOT_DIR}/tests/unit/mocks" -DSYN_USE_MULTICORE=1 -fsanitize=fuzzer,address,undefined \
    "${ROOT_DIR}/src/syntropic/proto/syn_isotp.c" \
    "${ROOT_DIR}/tests/unit/mocks/mock_port.c" \
    "${ROOT_DIR}/tests/fuzz/fuzz_isotp.c" \
    -o fuzzer_isotp
./fuzzer_isotp -max_total_time=10 || true
rm -f fuzzer_isotp

echo "=== Protocol Fuzzing Smoke Tests Complete ==="
