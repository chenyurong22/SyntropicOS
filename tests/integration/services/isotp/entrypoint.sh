#!/usr/bin/env bash
set -e

ip link add dev vcan0 type vcan 2>/dev/null || true
ip link set up vcan0 2>/dev/null || true

exec /app/isotp_server_runner
