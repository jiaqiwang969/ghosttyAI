#!/bin/bash
# check_coverage.sh - Check if coverage meets requirements
# Author: QA-001 (Test Lead)
# Date: 2025-08-25

OVERALL_TARGET=$1
CRITICAL_TARGET=$2

if [ -z "$OVERALL_TARGET" ] || [ -z "$CRITICAL_TARGET" ]; then
    echo "Usage: $0 <overall_target> <critical_target>"
    exit 1
fi

COVERAGE_FILE="reports/coverage/coverage.info"

if [ ! -f "$COVERAGE_FILE" ]; then
    echo "ERROR: Coverage file not found: $COVERAGE_FILE"
    exit 1
fi

# Extract coverage percentages
LINE_COVERAGE=$(lcov --summary "$COVERAGE_FILE" 2>&1 | grep "lines" | sed 's/.*: \([0-9.]*\)%.*/\1/')
FUNC_COVERAGE=$(lcov --summary "$COVERAGE_FILE" 2>&1 | grep "functions" | sed 's/.*: \([0-9.]*\)%.*/\1/')

echo "==================================="
echo "Coverage Report"
echo "==================================="
echo "Line Coverage: ${LINE_COVERAGE}%"
echo "Function Coverage: ${FUNC_COVERAGE}%"
echo "Required Overall: ${OVERALL_TARGET}%"
echo "Required Critical: ${CRITICAL_TARGET}%"
echo "==================================="

# Check if coverage meets requirements
if (( $(echo "$LINE_COVERAGE < $OVERALL_TARGET" | bc -l) )); then
    echo "FAIL: Overall coverage below target (${LINE_COVERAGE}% < ${OVERALL_TARGET}%)"
    exit 1
fi

# Check critical functions (all 22 tty_cmd_* functions)
CRITICAL_FUNCS=(
    "cmd_cell"
    "cmd_cells"
    "cmd_clearline"
    "cmd_clearscreen"
    "cmd_scrollup"
    "cmd_scrolldown"
    "cmd_insertline"
    "cmd_deleteline"
    "cmd_insertcharacter"
    "cmd_deletecharacter"
    "cmd_clearcharacter"
    "cmd_clearendofline"
    "cmd_clearstartofline"
    "cmd_clearendofscreen"
    "cmd_clearstartofscreen"
    "cmd_reverseindex"
    "cmd_linefeed"
    "cmd_alignmenttest"
    "cmd_setselection"
    "cmd_rawstring"
    "cmd_sixelimage"
    "cmd_syncstart"
)

CRITICAL_COVERED=0
CRITICAL_TOTAL=${#CRITICAL_FUNCS[@]}

for func in "${CRITICAL_FUNCS[@]}"; do
    if grep -q "$func" "$COVERAGE_FILE"; then
        CRITICAL_COVERED=$((CRITICAL_COVERED + 1))
    else
        echo "WARNING: Critical function not covered: $func"
    fi
done

CRITICAL_PERCENTAGE=$((CRITICAL_COVERED * 100 / CRITICAL_TOTAL))

echo "Critical Functions: ${CRITICAL_COVERED}/${CRITICAL_TOTAL} (${CRITICAL_PERCENTAGE}%)"

if [ $CRITICAL_PERCENTAGE -lt $CRITICAL_TARGET ]; then
    echo "FAIL: Critical function coverage below target (${CRITICAL_PERCENTAGE}% < ${CRITICAL_TARGET}%)"
    exit 1
fi

echo "==================================="
echo "SUCCESS: All coverage targets met!"
echo "==================================="
exit 0