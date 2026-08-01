#!/usr/bin/env python3
"""
SyntropicOS Code Coverage Analysis Tool

Parses LCOV coverage_src.info output to provide:
- Comprehensive line, function, and branch coverage summaries.
- Identification of uncovered functions and lines per source file.
- Module-level coverage ranking.
"""

import os
import sys


def analyze_lcov_info(filepath):
    if not os.path.exists(filepath):
        print(f"Error: Coverage file '{filepath}' not found.")
        sys.exit(1)

    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        lines = f.readlines()

    files_data = {}
    current_file = None

    for line in lines:
        line = line.strip()
        if line.startswith("SF:"):
            current_file = line[3:]
            if current_file.startswith("/workspace/"):
                current_file = current_file[11:]
            files_data[current_file] = {
                "lines_instrumented": 0,
                "lines_covered": 0,
                "uncovered_lines": [],
                "functions_instrumented": 0,
                "functions_covered": 0,
                "uncovered_functions": [],
                "branches_instrumented": 0,
                "branches_covered": 0,
            }
        elif current_file and line.startswith("FN:"):
            parts = line[3:].split(",", 1)
            if len(parts) == 2:
                fn_line, fn_name = parts[0], parts[1]
                files_data[current_file]["functions_instrumented"] += 1
                files_data[current_file]["uncovered_functions"].append(
                    (fn_line, fn_name)
                )
        elif current_file and line.startswith("FNDA:"):
            parts = line[5:].split(",", 1)
            if len(parts) == 2:
                count, fn_name = int(parts[0]), parts[1]
                if count > 0:
                    files_data[current_file]["functions_covered"] += 1
                    files_data[current_file]["uncovered_functions"] = [
                        (l, name)
                        for (l, name) in files_data[current_file][
                            "uncovered_functions"
                        ]
                        if name != fn_name
                    ]
        elif current_file and line.startswith("DA:"):
            parts = line[3:].split(",", 1)
            if len(parts) == 2:
                line_num, count = int(parts[0]), int(parts[1])
                files_data[current_file]["lines_instrumented"] += 1
                if count > 0:
                    files_data[current_file]["lines_covered"] += 1
                else:
                    files_data[current_file]["uncovered_lines"].append(line_num)
        elif current_file and line.startswith("BRDA:"):
            parts = line[5:].split(",")
            if len(parts) >= 4:
                files_data[current_file]["branches_instrumented"] += 1
                taken = parts[3]
                if taken != "-" and int(taken) > 0:
                    files_data[current_file]["branches_covered"] += 1

    total_lines_inst = sum(d["lines_instrumented"] for d in files_data.values())
    total_lines_cov = sum(d["lines_covered"] for d in files_data.values())
    total_fns_inst = sum(d["functions_instrumented"] for d in files_data.values())
    total_fns_cov = sum(d["functions_covered"] for d in files_data.values())
    total_br_inst = sum(d["branches_instrumented"] for d in files_data.values())
    total_br_cov = sum(d["branches_covered"] for d in files_data.values())

    line_pct = (
        (total_lines_cov / total_lines_inst * 100) if total_lines_inst else 100.0
    )
    fn_pct = (total_fns_cov / total_fns_inst * 100) if total_fns_inst else 100.0
    br_pct = (total_br_cov / total_br_inst * 100) if total_br_inst else 100.0

    print(
        "==============================================================================="
    )
    print(
        "                    SyntropicOS Coverage Analysis Report                       "
    )
    print(
        "==============================================================================="
    )
    print(
        f"  Line Coverage    : {line_pct:6.2f}% ({total_lines_cov:,} / {total_lines_inst:,} lines)"
    )
    print(
        f"  Function Coverage: {fn_pct:6.2f}% ({total_fns_cov:,} / {total_fns_inst:,} functions)"
    )
    print(
        f"  Branch Coverage  : {br_pct:6.2f}% ({total_br_cov:,} / {total_br_inst:,} branches)"
    )
    print(
        "-------------------------------------------------------------------------------"
    )

    uncovered_fn_list = []
    for sf, d in files_data.items():
        for l, fn in d["uncovered_functions"]:
            uncovered_fn_list.append((sf, l, fn))

    print("\n=== Uncovered Functions ===")
    if uncovered_fn_list:
        for sf, l, fn in uncovered_fn_list:
            print(f"  [UNC] {sf}:{l} -> {fn}()")
    else:
        print("  None! (100% Function Coverage)")

    print("\n=== Top Files with Uncovered Lines ===")
    sorted_files = sorted(
        [(sf, d) for sf, d in files_data.items() if d["uncovered_lines"]],
        key=lambda x: len(x[1]["uncovered_lines"]),
        reverse=True,
    )

    if not sorted_files:
        print("  None! (100% Line Coverage)")
    else:
        for sf, d in sorted_files[:15]:
            unc = d["uncovered_lines"]
            pct = (
                (d["lines_covered"] / d["lines_instrumented"] * 100)
                if d["lines_instrumented"]
                else 100.0
            )
            print(
                f"  {sf:<45} {pct:6.2f}% | {len(unc):3d} uncovered lines -> {unc[:12]}"
            )

    print(
        "==============================================================================="
    )


if __name__ == "__main__":
    info_path = (
        sys.argv[1] if len(sys.argv) > 1 else "build/cov/coverage_src.info"
    )
    analyze_lcov_info(info_path)
