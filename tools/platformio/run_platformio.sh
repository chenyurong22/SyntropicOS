#!/usr/bin/env bash
set -euo pipefail

# Script to validate SyntropicOS PlatformIO library manifest and packaging
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"

echo "=== SyntropicOS PlatformIO Package Validation & Build Audit ==="

if ! command -v pio >/dev/null 2>&1; then
    echo "Installing PlatformIO CLI..."
    python3 -m pip install --break-system-packages platformio
fi

echo "=== 1. Validating library.json Manifest & Packaging ==="
mkdir -p build
pio pkg pack --output build/SyntropicOS-pio.tar.gz

echo "=== 2. Inspecting PlatformIO Package Contents ==="
tar -tzf build/SyntropicOS-pio.tar.gz > build/tar_contents.txt
head -n 30 build/tar_contents.txt

echo "=== PlatformIO Packaging Audit PASS ==="
