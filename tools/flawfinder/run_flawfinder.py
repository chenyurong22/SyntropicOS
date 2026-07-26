#!/usr/bin/env python3
import os
import re
import sys

# SyntropicOS C Security & Flaw Finder Static Analysis Scanner
UNSAFE_PATTERNS = [
    (r"\bstrcpy\b", "CWE-120: Dangerous function strcpy() - use strncpy or syn_strlcpy"),
    (r"\bstrcat\b", "CWE-120: Dangerous function strcat() - use strncat or syn_strlcat"),
    (r"\bsprintf\b", "CWE-134: Unbounded sprintf() - use snprintf or syn_snprintf"),
    (r"\bvsprintf\b", "CWE-134: Unbounded vsprintf() - use vsnprintf"),
    (r"\bgets\b", "CWE-242: Highly dangerous function gets()"),
]

def scan_files(root_dir):
    violations = 0
    c_h_files = []
    for dirpath, _, filenames in os.walk(os.path.join(root_dir, "src/syntropic")):
        for f in filenames:
            if f.endswith(".c") or f.endswith(".h"):
                c_h_files.append(os.path.join(dirpath, f))

    print(f"=== SyntropicOS Security Flaw & Safety Scanner (Scanning {len(c_h_files)} files) ===")

    for filepath in c_h_files:
        rel_path = os.path.relpath(filepath, root_dir)
        with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
            for line_num, line in enumerate(f, 1):
                # Skip comments
                stripped = line.strip()
                if stripped.startswith("//") or stripped.startswith("*") or stripped.startswith("/*"):
                    continue
                for pattern, msg in UNSAFE_PATTERNS:
                    if re.search(pattern, line):
                        print(f"{rel_path}:{line_num}: [SECURITY HAZARD] {msg}\n  Line: {stripped}")
                        violations += 1

    if violations == 0:
        print("=== Security Flaw Analysis Completed Cleanly (0 Flaws Found) ===")
        sys.exit(0)
    else:
        print(f"=== Security Flaw Analysis FAILED ({violations} Security Violations Found) ===")
        sys.exit(1)

if __name__ == "__main__":
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), "../.."))
    scan_files(root)
