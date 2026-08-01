#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

echo "=== SyntropicOS Examples Cross-Compilation Check ==="

JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"

python3 -c "
import glob, subprocess, sys, os, concurrent.futures

c_files = glob.glob('${ROOT_DIR}/examples/**/*.c', recursive=True)
print(f'Checking {len(c_files)} example C files across ${JOBS} parallel workers...')

def check_file(c):
    cmd = f'arm-none-eabi-gcc -std=c99 -Wall -Wextra -Werror -I${ROOT_DIR} -I${ROOT_DIR}/src -I${ROOT_DIR}/src/port/stm32f4 -I${ROOT_DIR}/tests/unit/mocks/stm32 -I${ROOT_DIR}/tests/unit/mocks/esp32 -I${ROOT_DIR}/tests/unit/mocks/pico -mcpu=cortex-m4 -mthumb -c {c} -o /dev/null'
    res = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    if res.returncode != 0:
        return (c, res.stderr.strip())
    return None

failed = []
with concurrent.futures.ProcessPoolExecutor(max_workers=int('${JOBS}')) as executor:
    results = executor.map(check_file, sorted(c_files))
    for res in results:
        if res is not None:
            failed.append(res)

if not failed:
    print(f'=== ALL {len(c_files)} EXAMPLES COMPILE 100% CLEAN ===')
else:
    print(f'FAILURE: {len(failed)} example files failed to compile:')
    for f, err in failed:
        print(f'=== {f} ===\n{err}\n')
    sys.exit(1)
"
