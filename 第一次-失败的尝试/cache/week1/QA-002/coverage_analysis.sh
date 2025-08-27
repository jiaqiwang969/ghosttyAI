#!/bin/bash
# coverage_analysis.sh - 覆盖率分析脚本
# Author: QA-002
# Date: 2025-08-25

echo "=============================================================";
echo "           TEST COVERAGE ANALYSIS SCRIPT                     "
echo "=============================================================";
echo ""

CACHE_DIR="/Users/jqwang/98-ghosttyAI/cache/week1"
REPORT_DIR="$CACHE_DIR/QA-002/coverage_reports"
mkdir -p "$REPORT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "Starting coverage analysis at $(date '+%Y-%m-%d %H:%M:%S')"
echo ""

# Function to compile and test a component
test_component() {
    local component=$1
    local source_file=$2
    local test_file=$3
    local target_coverage=$4
    
    echo "----------------------------------------"
    echo "Testing $component"
    echo "----------------------------------------"
    
    cd "$CACHE_DIR/$component" 2>/dev/null || {
        echo -e "${RED}Error: Cannot find $component directory${NC}"
        return 1
    }
    
    # Compile with coverage flags
    echo "Compiling with coverage..."
    gcc -Wall -g -fprofile-arcs -ftest-coverage \
        -I../ARCH-001 -I../QA-001/test_framework/include \
        -o test_coverage \
        "$source_file" "$test_file" 2>/dev/null
    
    if [ $? -ne 0 ]; then
        echo -e "${YELLOW}Warning: Compilation failed for $component${NC}"
        echo "  Attempting simplified compilation..."
        
        # Try simplified compilation
        gcc -g -fprofile-arcs -ftest-coverage \
            -o test_coverage \
            "$source_file" 2>/dev/null
            
        if [ $? -ne 0 ]; then
            echo -e "${RED}Failed to compile $component${NC}"
            return 1
        fi
    fi
    
    # Run tests
    echo "Running tests..."
    ./test_coverage > /dev/null 2>&1
    
    # Generate coverage data
    echo "Generating coverage data..."
    gcov "$source_file" > /dev/null 2>&1
    
    # Extract coverage percentage
    if [ -f "${source_file}.gcov" ]; then
        local coverage=$(grep -E "^[[:space:]]*[0-9]+\.[0-9]+%.*:.*:" "${source_file}.gcov" | head -1 | sed 's/.*:\([0-9]*\.[0-9]*\)%.*/\1/')
        
        if [ -z "$coverage" ]; then
            # Alternative method to calculate coverage
            local total_lines=$(grep -c ":" "${source_file}.gcov")
            local executed_lines=$(grep -c "    #####:" "${source_file}.gcov")
            local covered_lines=$((total_lines - executed_lines))
            coverage=$(echo "scale=1; $covered_lines * 100 / $total_lines" | bc)
        fi
        
        echo "Coverage: ${coverage}% (Target: ${target_coverage}%)"
        
        # Check if target met
        if (( $(echo "$coverage >= $target_coverage" | bc -l) )); then
            echo -e "${GREEN}✓ Target met!${NC}"
        else
            echo -e "${YELLOW}⚠ Below target${NC}"
        fi
        
        # Save result
        echo "$component,$coverage,$target_coverage" >> "$REPORT_DIR/coverage_summary.csv"
        
        return 0
    else
        echo -e "${RED}Failed to generate coverage data${NC}"
        return 1
    fi
}

# Initialize summary file
echo "Component,Actual,Target" > "$REPORT_DIR/coverage_summary.csv"

# Test each component
echo "=== TESTING COMPONENTS ==="
echo ""

# CORE-001
test_component "CORE-001" "tty_write_hooks.c" "tests/test_tty_write_hooks.c" 75

# CORE-002  
test_component "CORE-002" "backend_router.c" "test_backend_router.c" 70

# INTG-001
test_component "INTG-001" "backend_ghostty.c" "test_ghostty_backend.c" 50

echo ""
echo "=== COVERAGE SUMMARY ==="
echo ""

# Calculate overall coverage
if [ -f "$REPORT_DIR/coverage_summary.csv" ]; then
    total_coverage=0
    count=0
    
    while IFS=',' read -r component actual target; do
        if [ "$component" != "Component" ]; then
            total_coverage=$(echo "$total_coverage + $actual" | bc)
            count=$((count + 1))
            printf "%-15s %6s%% (Target: %s%%)\n" "$component:" "$actual" "$target"
        fi
    done < "$REPORT_DIR/coverage_summary.csv"
    
    if [ $count -gt 0 ]; then
        overall=$(echo "scale=1; $total_coverage / $count" | bc)
        echo "----------------------------------------"
        echo "Overall Coverage: ${overall}% (Target: 65%)"
        
        if (( $(echo "$overall >= 65" | bc -l) )); then
            echo -e "${GREEN}✓ OVERALL TARGET MET!${NC}"
            exit_code=0
        else
            gap=$(echo "65 - $overall" | bc)
            echo -e "${YELLOW}⚠ Gap to target: ${gap}%${NC}"
            exit_code=1
        fi
    fi
fi

echo ""
echo "Report saved to: $REPORT_DIR/coverage_summary.csv"
echo "Completed at $(date '+%Y-%m-%d %H:%M:%S')"
echo "============================================================="

exit $exit_code