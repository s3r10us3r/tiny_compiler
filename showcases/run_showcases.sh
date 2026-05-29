#!/bin/bash
# ═══════════════════════════════════════════════════════════════════
# tiny_compiler — Defence Showcase Runner
# Usage: ./run_showcases.sh
# Requires: clang on PATH, compiler binary at ../build/compiler
# ═══════════════════════════════════════════════════════════════════

COMPILER="../build/compiler"
BLD='\033[1m'
CYN='\033[0;36m'
GRN='\033[0;32m'
RED='\033[0;31m'
YEL='\033[1;33m'
NC='\033[0m'

header() {
    echo ""
    echo -e "${BLD}${CYN}══════════════════════════════════════════════════════${NC}"
    echo -e "${BLD}${CYN}  $1${NC}"
    echo -e "${BLD}${CYN}══════════════════════════════════════════════════════${NC}"
}

# run_feature <file>
# Compiles + runs a feature showcase, printing its output.
run_feature() {
    local file="$1"
    header "$file"
    "$COMPILER" "$file" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo -e "${RED}Compiler error — run manually to debug.${NC}"
        return
    fi
    clang output.ll -o _showcase_bin 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "${RED}Clang link error.${NC}"
        return
    fi
    echo -e "${GRN}Output:${NC}"
    ./_showcase_bin
    rm -f _showcase_bin
}

# run_error <file>
# Compiles an error showcase, printing the compiler's diagnostics.
run_error() {
    local file="$1"
    header "$file"
    echo -e "${YEL}Compiler diagnostics:${NC}"
    "$COMPILER" "$file" 2>&1
    echo ""
}

# ── Feature showcases ───────────────────────────────────────────────
run_feature showcase_1_basics.tc
run_feature showcase_2_functions.tc
run_feature showcase_3_oop.tc
run_feature showcase_4_globals.tc

# ── Error showcases ─────────────────────────────────────────────────
run_error showcase_errors_parser.tc
run_error showcase_errors_semantic.tc

# ── Cleanup ─────────────────────────────────────────────────────────
rm -f output.ll

echo ""
echo -e "${BLD}${CYN}══════════════════════════════════════════════════════${NC}"
echo -e "${BLD}  Showcase complete.${NC}"
echo -e "${BLD}${CYN}══════════════════════════════════════════════════════${NC}"
echo ""
