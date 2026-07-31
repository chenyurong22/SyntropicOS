#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

echo "=== SyntropicOS Examples Cross-Compilation Check ==="

python3 -c "
import glob, subprocess, sys

c_files = glob.glob('${ROOT_DIR}/examples/**/*.c', recursive=True)
print(f'Checking {len(c_files)} example C files...')

failed = []
for c in sorted(c_files):
    cmd = f'arm-none-eabi-gcc -std=c99 -Wall -Wextra -Werror -I${ROOT_DIR} -I${ROOT_DIR}/src -I${ROOT_DIR}/src/port/stm32f4 -I${ROOT_DIR}/tests/unit/mocks/stm32 -I${ROOT_DIR}/tests/unit/mocks/esp32 -I${ROOT_DIR}/tests/unit/mocks/pico -mcpu=cortex-m4 -mthumb -c {c} -o /dev/null'
    res = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    if res.returncode != 0:
        failed.append((c, res.stderr.strip()))

if not failed:
    print('=== ALL 65 EXAMPLES COMPILE 100% CLEAN ===')
else:
    print(f'FAILURE: {len(failed)} example files failed to compile:')
    for f, err in failed:
        print(f'=== {f} ===\n{err}\n')
    sys.exit(1)
"
