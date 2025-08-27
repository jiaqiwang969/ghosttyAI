#!/bin/bash
# verify_warnings.sh - Verify zero compilation warnings
# Author: CORE-001 (c-tmux-specialist)
# Date: 2025-08-25
# Task: CORE-001-WARNINGS

echo "============================================================="
echo "Compilation Warning Verification"
echo "Task: CORE-001-WARNINGS"
echo "============================================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check we're in a valid directory
if [ ! -f "tty_write_hooks.c" ]; then
    echo -e "${RED}Error: tty_write_hooks.c not found${NC}"
    echo "Run this script from one of these directories:"
    echo "  - cache/week1/CORE-001/clean/"
    echo "  - cache/week1/CORE-001/fixed/"
    exit 1
fi

# Track results
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to run a compilation test
run_test() {
    local description="$1"
    local flags="$2"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -n "Testing: $description... "
    
    # Run compilation with specified flags
    if gcc $flags -c tty_write_hooks.c -I. -I../../ARCH-001 -o /tmp/test.o 2>/tmp/warnings.txt; then
        # Check if there are any warnings in the output
        if [ -s /tmp/warnings.txt ]; then
            echo -e "${YELLOW}WARNINGS${NC}"
            echo "  Warnings found:"
            cat /tmp/warnings.txt | head -10
            FAILED_TESTS=$((FAILED_TESTS + 1))
        else
            echo -e "${GREEN}PASS${NC} (0 warnings)"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        fi
    else
        echo -e "${RED}FAIL${NC}"
        echo "  Compilation failed:"
        cat /tmp/warnings.txt | head -10
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    
    rm -f /tmp/test.o /tmp/warnings.txt
}

echo "Running compilation tests with various warning flags..."
echo ""

# Test 1: Basic compilation
run_test "Basic compilation (no flags)" ""

# Test 2: Standard warnings
run_test "Standard warnings (-Wall)" "-Wall"

# Test 3: Extra warnings
run_test "Extra warnings (-Wall -Wextra)" "-Wall -Wextra"

# Test 4: Treat warnings as errors
run_test "Warnings as errors (-Wall -Wextra -Werror)" "-Wall -Wextra -Werror"

# Test 5: Unused parameters specifically
run_test "Unused parameters (-Wunused-parameter)" "-Wunused-parameter"

# Test 6: Implicit function declarations
run_test "Implicit declarations (-Wimplicit-function-declaration)" "-Wimplicit-function-declaration"

# Test 7: Format string warnings
run_test "Format strings (-Wformat -Wformat-security)" "-Wformat -Wformat-security"

# Test 8: Missing prototypes
run_test "Missing prototypes (-Wmissing-prototypes)" "-Wmissing-prototypes"

# Test 9: Strict prototypes
run_test "Strict prototypes (-Wstrict-prototypes)" "-Wstrict-prototypes"

# Test 10: Everything together
run_test "All strict flags combined" "-Wall -Wextra -Werror -Wunused-parameter -Wimplicit-function-declaration -Wformat -Wformat-security"

echo ""
echo "============================================================="
echo "Test Summary"
echo "============================================================="
echo "Total Tests:  $TOTAL_TESTS"
echo -e "Passed:       ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed:       ${RED}$FAILED_TESTS${NC}"
echo ""

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}✅ SUCCESS: Zero warnings with all compiler flags!${NC}"
    echo ""
    echo "Task CORE-001-WARNINGS: COMPLETE"
    echo "Version: 1.2.0"
    echo "Status: Production Ready"
    exit 0
else
    echo -e "${RED}❌ FAILURE: Warnings detected${NC}"
    echo ""
    echo "Please fix the warnings and run this script again."
    exit 1
fi