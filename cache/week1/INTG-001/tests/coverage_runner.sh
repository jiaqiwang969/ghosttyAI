#!/bin/bash
# coverage_runner.sh - Run tests and generate coverage report
# Author: INTG-001
# Date: 2025-08-25

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
COVERAGE_DIR="${SCRIPT_DIR}/coverage"

echo "=== Ghostty Backend Coverage Runner ==="
echo "Target: >50% Code Coverage"
echo ""

# Clean and create directories
rm -rf "${BUILD_DIR}" "${COVERAGE_DIR}"
mkdir -p "${BUILD_DIR}" "${COVERAGE_DIR}"

# Compile with coverage flags
echo "1. Compiling with coverage instrumentation..."
clang -fprofile-instr-generate -fcoverage-mapping \
    -O0 -g -pthread \
    -I../../ \
    -o "${BUILD_DIR}/test_all" \
    test_all_callbacks.c \
    -lm

echo "✓ Compilation complete"

# Run tests
echo ""
echo "2. Running test suite..."
cd "${BUILD_DIR}"
LLVM_PROFILE_FILE="${COVERAGE_DIR}/test-%p.profraw" ./test_all

echo "✓ Tests complete"

# Process coverage data
echo ""
echo "3. Processing coverage data..."
llvm-profdata merge -sparse "${COVERAGE_DIR}"/*.profraw -o "${COVERAGE_DIR}/test.profdata"

# Generate coverage report
echo ""
echo "4. Generating coverage report..."
llvm-cov report ./test_all -instr-profile="${COVERAGE_DIR}/test.profdata" \
    -ignore-filename-regex="test_.*" \
    > "${COVERAGE_DIR}/coverage_summary.txt"

# Generate detailed HTML report
llvm-cov show ./test_all -instr-profile="${COVERAGE_DIR}/test.profdata" \
    -format=html -output-dir="${COVERAGE_DIR}/html" \
    -ignore-filename-regex="test_.*"

# Parse and display coverage
echo ""
echo "5. Coverage Summary:"
echo "==================="
cat "${COVERAGE_DIR}/coverage_summary.txt"

# Extract coverage percentage
COVERAGE=$(grep "TOTAL" "${COVERAGE_DIR}/coverage_summary.txt" | awk '{print $10}' | sed 's/%//')

echo ""
echo "==================================="
echo "Total Coverage: ${COVERAGE}%"

if (( $(echo "$COVERAGE >= 50" | bc -l) )); then
    echo "✓ TARGET MET: Coverage is >= 50%"
    echo "==================================="
    exit 0
else
    echo "✗ TARGET NOT MET: Coverage is < 50%"
    echo "==================================="
    exit 1
fi