#!/bin/bash
# ============================================================
# Test runner for the tiny_compiler
# Usage: ./run_tests.sh
# Requires: clang on PATH, compiler binary at ../build/compiler
# ============================================================

COMPILER="../build/compiler"
PASS=0
FAIL=0
RED='\033[0;31m'
GRN='\033[0;32m'
YEL='\033[1;33m'
NC='\033[0m'

# run_ok <file> <expected_output>
# Compiles + runs a .tc file, checks that stdout matches expected.
run_ok() {
    local file="$1"
    local expected="$2"
    local name="${file%.tc}"

    "$COMPILER" "$file" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL${NC} [$name] — compiler error (expected success)"
        FAIL=$((FAIL+1))
        return
    fi

    clang output.ll -o "_test_bin" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL${NC} [$name] — clang link error"
        FAIL=$((FAIL+1))
        return
    fi

    actual=$(./_test_bin | tr '\n' ' ' | sed 's/ $//')
    if [ "$actual" = "$expected" ]; then
        echo -e "${GRN}PASS${NC} [$name]  output: $actual"
        PASS=$((PASS+1))
    else
        echo -e "${RED}FAIL${NC} [$name]"
        echo "       expected: $expected"
        echo "       got:      $actual"
        FAIL=$((FAIL+1))
    fi
    rm -f _test_bin
}

# run_err <file> <error_substring...>
# Compiles a .tc file, checks that it FAILS and the error output
# contains at least one of the given substrings.
run_err() {
    local file="$1"
    shift
    local name="${file%.tc}"

    output=$("$COMPILER" "$file" 2>&1)
    if [ $? -eq 0 ]; then
        echo -e "${RED}FAIL${NC} [$name] — expected compile error, but succeeded"
        FAIL=$((FAIL+1))
        return
    fi

    for pattern in "$@"; do
        if echo "$output" | grep -q "$pattern"; then
            echo -e "${GRN}PASS${NC} [$name]  detected: \"$pattern\""
            PASS=$((PASS+1))
            return
        fi
    done

    echo -e "${RED}FAIL${NC} [$name] — error message not matched"
    echo "       output: $output"
    FAIL=$((FAIL+1))
}

echo "=================================================="
echo " tiny_compiler feature test suite"
echo "=================================================="

# ------------------------------------------------------------------
# Successful compilation + correct output tests
# ------------------------------------------------------------------

run_ok test_if.tc              "1 0 2 42"
run_ok test_while_break.tc     "0 1 2 0 1 2 5"
run_ok test_functions.tc       "7 1 0 120"
run_ok test_scope.tc           "2 1 10 3 99"
run_ok test_structs.tc         "3 4 10 4 1"
run_ok test_combined.tc        "7 1 4 0 4"
run_ok test_classes.tc         "3 4 6 8 5 1 0"
run_ok test_floats.tc          "4.000000 3.750000 1.000000 1 5.000000"
run_ok test_arrays.tc          "10 40 100 16"
run_ok test_bool_logic.tc      "1 0 1 0 0 1 0 1 1 3"
run_ok test_strings.tc         "hello world tiny"
run_ok test_nested_structs.tc  "1 6 3 10 12 1"
run_ok test_class_keyword.tc   "24 20 0 25 1"
run_ok test_sort.tc            "1 2 3 5 8 9"
run_ok test_globals.tc         "10 20 30 1 99"

# ------------------------------------------------------------------
# Error detection tests (each sub-error is checked individually)
# ------------------------------------------------------------------

# Semantic errors (TypeChecker — collects all)
run_err test_errors.tc "Return type mismatch"
run_err test_errors.tc "break.*outside"
run_err test_errors.tc "Condition in 'if' must be a boolean"
run_err test_errors.tc "Undefined function"
run_err test_errors.tc "Missing field"

# Keyword / reserved-name misuse (Parser — throws on first offence)
run_err test_errors_keyword_name.tc "built-in type name"
run_err test_errors_kw_fn.tc        "reserved keyword"
run_err test_errors_kw_var.tc       "reserved keyword"
run_err test_errors_kw_field.tc     "reserved keyword"
run_err test_errors_kw_param.tc     "reserved keyword"
run_err test_errors_kw_method.tc    "reserved keyword"
run_err test_errors_type_as_fn.tc   "built-in type name"
run_err test_errors_bool_as_name.tc "reserved literals"

# Duplicate declarations (TypeChecker — collects all)
run_err test_errors_duplicates.tc "Duplicate type declaration"
run_err test_errors_duplicates.tc "Duplicate function declaration"
run_err test_errors_duplicates.tc "Duplicate method"
run_err test_errors_duplicates.tc "Duplicate field"

# Variable and assignment type mismatches (TypeChecker — collects all)
run_err test_errors_type_mismatch.tc "does not match declared type"
run_err test_errors_type_mismatch.tc "Type mismatch in assignment"
run_err test_errors_type_mismatch.tc "Cannot declare a variable of type 'void'"

# Expression-level type errors (TypeChecker — collects all)
run_err test_errors_expressions.tc "Operator 'not' requires a boolean operand"
run_err test_errors_expressions.tc "Unary minus requires a numeric operand"
run_err test_errors_expressions.tc "Mathematical operations require numeric types"
run_err test_errors_expressions.tc "Mismatched types in binary expression"
run_err test_errors_expressions.tc "Logical operators"
run_err test_errors_expressions.tc "String operations are not supported"

# Control-flow errors (TypeChecker — collects all)
run_err test_errors_control_flow.tc "Condition in 'while' must be a boolean"
run_err test_errors_control_flow.tc "'return' used outside of a function"
run_err test_errors_control_flow.tc "Void function cannot return a value"

# Scope errors (TypeChecker — collects all)
run_err test_errors_scope.tc "Undeclared variable"
run_err test_errors_scope.tc "already declared in this scope"

# Function and method call errors (TypeChecker — collects all)
run_err test_errors_calls.tc "Function 'add' expects"
run_err test_errors_calls.tc "type mismatch in call to 'add'"
run_err test_errors_calls.tc "Method call on non-struct"
run_err test_errors_calls.tc "has no methods"
run_err test_errors_calls.tc "has no method '"
run_err test_errors_calls.tc "Method 'set' expects"
run_err test_errors_calls.tc "type mismatch in call to 'set'"

# ------------------------------------------------------------------
echo ""
echo "=================================================="
echo -e " Results: ${GRN}$PASS passed${NC}  ${RED}$FAIL failed${NC}"
echo "=================================================="
rm -f output.ll
[ $FAIL -eq 0 ]   # exit 0 on full pass, 1 otherwise
