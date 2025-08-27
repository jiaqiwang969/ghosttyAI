#!/bin/bash
# compile_test.sh - Test compilation with strict warning flags
# Author: libtmux-core-developer
# Date: 2025-08-25
# Task: CORE-002-WARNINGS

set -e  # Exit on error

echo "================================================"
echo "  Compilation Warning Test Suite"
echo "  Testing with -Wall -Wextra -Werror"
echo "================================================"
echo

# Compiler settings
CC=gcc
CFLAGS="-Wall -Wextra -Werror -std=c11 -pthread -O2 -g"
INCLUDES="-I. -I../CORE-001 -I../ARCH-001"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to compile a file
compile_file() {
    local file=$1
    local name=$(basename "$file" .c)
    
    echo -n "Compiling $name... "
    
    if $CC $CFLAGS $INCLUDES -c "$file" -o "${name}.o" 2>/tmp/compile_${name}.log; then
        echo -e "${GREEN}✓ SUCCESS${NC}"
        return 0
    else
        echo -e "${RED}✗ FAILED${NC}"
        echo "Error log:"
        cat /tmp/compile_${name}.log
        return 1
    fi
}

# Function to test directory
test_directory() {
    local dir=$1
    local dir_name=$(basename "$dir")
    
    echo
    echo "Testing directory: $dir_name"
    echo "----------------------------------------"
    
    cd "$dir"
    
    local total=0
    local passed=0
    local failed=0
    
    # Test each C file
    for file in *.c; do
        if [ -f "$file" ]; then
            total=$((total + 1))
            if compile_file "$file"; then
                passed=$((passed + 1))
            else
                failed=$((failed + 1))
            fi
        fi
    done
    
    echo
    echo "Results for $dir_name:"
    echo "  Total files: $total"
    echo -e "  Passed: ${GREEN}$passed${NC}"
    echo -e "  Failed: ${RED}$failed${NC}"
    
    return $failed
}

# Main test execution
main() {
    local base_dir="/Users/jqwang/98-ghosttyAI/cache/week1/CORE-002"
    local total_failed=0
    
    # Test main directory
    echo "================================================"
    echo "Testing Main Directory"
    echo "================================================"
    
    if test_directory "$base_dir"; then
        echo -e "${GREEN}Main directory: All files compile cleanly${NC}"
    else
        total_failed=$?
        echo -e "${YELLOW}Main directory: Some warnings found${NC}"
    fi
    
    # Test fixed directory
    echo
    echo "================================================"
    echo "Testing Fixed Directory"
    echo "================================================"
    
    if test_directory "$base_dir/fixed"; then
        echo -e "${GREEN}Fixed directory: All files compile cleanly${NC}"
    else
        failed=$?
        total_failed=$((total_failed + failed))
        echo -e "${YELLOW}Fixed directory: Some warnings found${NC}"
    fi
    
    # Summary
    echo
    echo "================================================"
    echo "                 SUMMARY"
    echo "================================================"
    
    if [ $total_failed -eq 0 ]; then
        echo -e "${GREEN}✅ ALL FILES COMPILE WITHOUT WARNINGS${NC}"
        echo "Successfully compiled with -Wall -Wextra -Werror"
    else
        echo -e "${RED}❌ COMPILATION WARNINGS DETECTED${NC}"
        echo "Total files with warnings: $total_failed"
        exit 1
    fi
    
    # Test library build
    echo
    echo "================================================"
    echo "Testing Library Build"
    echo "================================================"
    
    cd "$base_dir"
    echo -n "Building library... "
    
    if ar rcs libbackend_router_clean.a backend_router.o 2>/dev/null; then
        echo -e "${GREEN}✓ Library built successfully${NC}"
    else
        echo -e "${RED}✗ Library build failed${NC}"
    fi
    
    # Clean up object files
    rm -f *.o fixed/*.o /tmp/compile_*.log
    
    echo
    echo "================================================"
    echo "Task CORE-002-WARNINGS Complete"
    echo "All compilation warnings have been cleaned"
    echo "================================================"
}

# Run main function
main