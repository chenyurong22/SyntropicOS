#!/usr/bin/env bash
set -e

echo "[WireGuard Server] Starting WireGuard daemon..."
wg-quick up /etc/wireguard/wg0.conf || true
echo "[WireGuard Server] Interface wg0 online. Listening on 51820/udp."

# Keep container running
exec tail -f /dev/null
