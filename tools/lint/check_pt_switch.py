#!/usr/bin/env python3
"""
SyntropicOS Protothread Switch Statement Linter

Enforces that no bare 'switch' statements are used inside protothread function
bodies (between PT_BEGIN and PT_END). Bare switch statements break Duff's device
line continuation in coroutines.
"""

import sys
import re
import os

def check_file(filepath):
    errors = []
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()

    in_pt = False
    pt_start_line = 0

    # Match switch statement ignoring comments and macro definitions
    switch_pattern = re.compile(r'\bswitch\s*\(')
    pt_begin_pattern = re.compile(r'\bPT_BEGIN\s*\(')
    pt_end_pattern = re.compile(r'\bPT_END\s*\(')
    macro_def_pattern = re.compile(r'^\s*#\s*define\s+PT_BEGIN')

    for idx, line in enumerate(lines, 1):
        cleaned = re.sub(r'//.*$', '', line)

        # Ignore macro definition of PT_BEGIN itself in syn_pt.h
        if macro_def_pattern.search(line):
            continue

        if pt_begin_pattern.search(cleaned):
            in_pt = True
            pt_start_line = idx
            continue

        if in_pt:
            if switch_pattern.search(cleaned):
                errors.append(f"{filepath}:{idx}: error: 'switch' statement inside protothread body (started at line {pt_start_line}). Bare switch statements break protothread continuation (Duff's device). Use if/else if chains instead.")
            if pt_end_pattern.search(cleaned):
                in_pt = False

    return errors

def main():
    if len(sys.argv) > 1:
        target_dirs = sys.argv[1:]
    else:
        target_dirs = ['src', 'examples', 'tests']

    all_errors = []
    extensions = ('.c', '.h', '.cpp', '.hpp', '.ino')

    for root_dir in target_dirs:
        for root, _, files in os.walk(root_dir):
            for file in files:
                if file.endswith(extensions):
                    path = os.path.join(root, file)
                    errors = check_file(path)
                    all_errors.extend(errors)

    if all_errors:
        print("=== Protothread Switch Linter Errors ===")
        for err in all_errors:
            print(err)
        print(f"Total violations: {len(all_errors)}")
        sys.exit(1)
    else:
        print("Protothread Switch Linter: PASS (0 violations)")
        sys.exit(0)

if __name__ == '__main__':
    main()
