#!/usr/bin/env bash
set -e

# SyntropicOS Valgrind Memcheck Dynamic Analysis Runner
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"

echo "=== SyntropicOS Valgrind Memcheck Memory Leak & Error Audit ==="

# Build test runner
make -f tests/Makefile.unity clean test_unity

# Execute Valgrind Memcheck
valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./build/tests/test_unity

echo "=== Valgrind Memcheck Audit Completed Cleanly (0 Memory Errors / 0 Leaks) ==="
