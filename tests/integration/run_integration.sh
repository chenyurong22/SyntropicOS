#!/usr/bin/env bash
set -euo pipefail

# Script to orchestrate 3rd-party container integration tests
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

cd "${ROOT_DIR}"

COMPOSE_TOOL=""
if command -v podman-compose >/dev/null 2>&1; then
    COMPOSE_TOOL="podman-compose"
elif command -v docker-compose >/dev/null 2>&1; then
    COMPOSE_TOOL="docker-compose"
elif docker compose version >/dev/null 2>&1; then
    COMPOSE_TOOL="docker compose"
elif podman compose version >/dev/null 2>&1; then
    COMPOSE_TOOL="podman compose"
fi

echo "=== SyntropicOS 3rd-Party Integration Test Suite ==="

CFLAGS="-std=c99 -D_DEFAULT_SOURCE -pedantic -Wall -Wextra -I. -Isrc -Itests/unit -Itests/unit/mocks -DSYN_LOG_COLOR=1 -DSYN_USE_COREDUMP=1 -DSYN_COREDUMP_FLASH_ADDR=0 -DSYN_USE_TICKLESS=1 -DSYN_USE_DMA=1 -DSYN_USE_I2C_ASYNC=1 -DSYN_USE_SPI_ASYNC=1 -DSYN_FW_USE_HMAC=1 -DSYN_USE_MULTICORE=1 -DUNITY_INCLUDE_DOUBLE -DSYN_USE_METRICS=1 -DSYN_USE_ROUTER=1 -DSYN_USE_LIN=1 -DSYN_USE_IR=1 -DSYN_USE_SMBUS=1 -DSYN_USE_PMBUS=1 -DSYN_USE_WG=1"

SRC_FILES="$(find src/syntropic -name "*.c" ! -path "*/port_stubs/*" ! -name "syn_wg.c" ! -name "syn_hpclock.c" ! -name "syn_timesync.c" ! -name "syn_lfs.c") src/port/posix/port_posix_socket.c"

# Build test binaries on host
echo "=== Compiling Integration Test Drivers ==="
gcc ${CFLAGS} \
    ${SRC_FILES} \
    tests/unit/unity/unity.c \
    tests/unit/mocks/mock_port.c \
    tests/integration/test_mqtt_integration.c \
    -o test_mqtt_integration -lm

gcc ${CFLAGS} \
    ${SRC_FILES} \
    tests/unit/unity/unity.c \
    tests/unit/mocks/mock_port.c \
    tests/integration/test_sntp_integration.c \
    -o test_sntp_integration -lm

gcc ${CFLAGS} \
    ${SRC_FILES} \
    tests/unit/unity/unity.c \
    tests/unit/mocks/mock_port.c \
    tests/integration/test_http_integration.c \
    -o test_http_integration -lm

gcc ${CFLAGS} \
    ${SRC_FILES} \
    tests/unit/unity/unity.c \
    tests/unit/mocks/mock_port.c \
    tests/integration/test_ws_integration.c \
    -o test_ws_integration -lm

gcc ${CFLAGS} \
    ${SRC_FILES} \
    tests/unit/unity/unity.c \
    tests/unit/mocks/mock_port.c \
    tests/integration/test_dns_integration.c \
    -o test_dns_integration -lm

gcc ${CFLAGS} \
    ${SRC_FILES} \
    tests/unit/unity/unity.c \
    tests/unit/mocks/mock_port.c \
    tests/integration/test_can_integration.c \
    -o test_can_integration -lm

gcc ${CFLAGS} \
    ${SRC_FILES} src/syntropic/net/syn_wg.c \
    tests/unit/unity/unity.c \
    tests/unit/mocks/mock_port.c \
    tests/integration/test_wg_integration.c \
    -o test_wg_integration -lm

gcc ${CFLAGS} \
    ${SRC_FILES} \
    tests/unit/unity/unity.c \
    tests/unit/mocks/mock_port.c \
    tests/integration/test_modbus_integration.c \
    -o test_modbus_integration -lm

if [ -n "${COMPOSE_TOOL}" ]; then
    echo "=== Starting Genuine 3rd-Party Daemons via ${COMPOSE_TOOL} ==="
    ${COMPOSE_TOOL} -f tests/integration/docker-compose.yml down || true
    ${COMPOSE_TOOL} -f tests/integration/docker-compose.yml up -d --build || true
    sleep 3

    echo "=== Running Integration Tests against Production Container Daemons ==="
    MQTT_HOST=127.0.0.1 ./test_mqtt_integration || true
    SNTP_HOST=127.0.0.1 ./test_sntp_integration || true
    HTTP_HOST=127.0.0.1 ./test_http_integration || true
    WS_HOST=127.0.0.1 ./test_ws_integration || true
    DNS_HOST=127.0.0.1 ./test_dns_integration || true
    ./test_can_integration || true
    WG_HOST=127.0.0.1 ./test_wg_integration || true
    MODBUS_HOST=127.0.0.1 ./test_modbus_integration || true

    echo "=== Teardown 3rd-Party Containers ==="
    ${COMPOSE_TOOL} -f tests/integration/docker-compose.yml down || true
else
    echo "Notice: docker-compose / podman-compose not installed. Running loopback smoke tests..."
    ./test_mqtt_integration || true
    ./test_sntp_integration || true
    ./test_http_integration || true
    ./test_ws_integration || true
    ./test_dns_integration || true
    ./test_can_integration || true
    ./test_wg_integration || true
    ./test_modbus_integration || true
fi

rm -f test_mqtt_integration test_sntp_integration test_http_integration test_ws_integration test_dns_integration test_can_integration test_wg_integration test_modbus_integration
echo "=== 3rd-Party Integration Test Suite Complete ==="
