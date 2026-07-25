#!/usr/bin/env bash
set -e

# SyntropicOS Cyclomatic Control-Flow Complexity Analyzer
# Measures decision points (V(G)) per function to keep event handlers non-blocking and lean.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"

MAX_COMPLEXITY=${MAX_COMPLEXITY:-15}

echo "=== SyntropicOS Cyclomatic Control-Flow Complexity Scan ==="
echo "Max Allowed Complexity V(G): ${MAX_COMPLEXITY}"

if command -v lizard >/dev/null 2>&1; then
    echo "Running lizard complexity scanner..."
    lizard -C "${MAX_COMPLEXITY}" -x "src/syntropic/port_stubs/*" src/syntropic
elif command -v pmccabe >/dev/null 2>&1; then
    echo "Running pmccabe complexity scanner..."
    pmccabe $(find src/syntropic -name "*.c") | sort -nr | head -n 20
else
    echo "Notice: Neither lizard nor pmccabe installed. Running awk control-flow estimator..."
    printf "%-35s %-30s %s\n" "File" "Function/Block" "Estimated V(G)"
    echo "-------------------------------------------------------------------------------"
    
    # Awk heuristic estimator scanning decision keywords (if, else if, for, while, case, &&, ||)
    find src/syntropic -name "*.c" ! -path "*/port_stubs/*" | while read -r f; do
        awk -v fname="$(basename "${f}")" '
        /^[a-zA-Z_0-9]+[ \t]+[a-zA-Z_0-9]+\(/ { func_name = $0 }
        /\b(if|else if|for|while|case|\&\&|\|\|)\b/ { count++ }
        /^}/ {
            if (count > 0 && func_name != "") {
                vg = count + 1;
                if (vg > 15) {
                    print fname, func_name, vg;
                }
            }
            count = 0; func_name = "";
        }' "${f}"
    done | sort -t' ' -k3 -nr | head -n 15
fi

echo "=== Cyclomatic Complexity Audit Complete ==="
