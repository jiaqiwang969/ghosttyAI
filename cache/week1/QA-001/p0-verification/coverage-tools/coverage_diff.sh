#!/bin/bash

# Coverage Diff Script
# Compare before/after coverage
# Version: 1.0.0

set -euo pipefail

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Directories
BASE_DIR="/Users/jqwang/98-ghosttyAI/cache/week1/QA-001/p0-verification"
COVERAGE_DIR="${BASE_DIR}/coverage"
REPORT_DIR="${BASE_DIR}/reports"

# Timestamp
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Function to extract coverage percentage from lcov info
extract_coverage() {
    local info_file=$1
    local metric=$2
    
    if [ ! -f "$info_file" ]; then
        echo "0"
        return
    fi
    
    case $metric in
        "line")
            local lines_found=$(grep "^LF:" "$info_file" | awk -F: '{sum+=$2} END {print sum}')
            local lines_hit=$(grep "^LH:" "$info_file" | awk -F: '{sum+=$2} END {print sum}')
            if [ "$lines_found" -gt 0 ]; then
                echo "scale=2; $lines_hit * 100 / $lines_found" | bc
            else
                echo "0"
            fi
            ;;
        "function")
            local funcs_found=$(grep "^FNF:" "$info_file" | awk -F: '{sum+=$2} END {print sum}')
            local funcs_hit=$(grep "^FNH:" "$info_file" | awk -F: '{sum+=$2} END {print sum}')
            if [ "$funcs_found" -gt 0 ]; then
                echo "scale=2; $funcs_hit * 100 / $funcs_found" | bc
            else
                echo "0"
            fi
            ;;
        "branch")
            local branches_found=$(grep "^BRF:" "$info_file" | awk -F: '{sum+=$2} END {print sum}')
            local branches_hit=$(grep "^BRH:" "$info_file" | awk -F: '{sum+=$2} END {print sum}')
            if [ "$branches_found" -gt 0 ]; then
                echo "scale=2; $branches_hit * 100 / $branches_found" | bc
            else
                echo "0"
            fi
            ;;
    esac
}

# Function to compare two coverage files
compare_coverage() {
    local before_file=$1
    local after_file=$2
    local component=$3
    
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}Coverage Comparison: ${component}${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    # Extract metrics
    local before_line=$(extract_coverage "$before_file" "line")
    local after_line=$(extract_coverage "$after_file" "line")
    local before_func=$(extract_coverage "$before_file" "function")
    local after_func=$(extract_coverage "$after_file" "function")
    local before_branch=$(extract_coverage "$before_file" "branch")
    local after_branch=$(extract_coverage "$after_file" "branch")
    
    # Calculate differences
    local line_diff=$(echo "$after_line - $before_line" | bc)
    local func_diff=$(echo "$after_func - $before_func" | bc)
    local branch_diff=$(echo "$after_branch - $before_branch" | bc)
    
    # Display results
    echo -e "\n${YELLOW}Metric        Before    After     Change${NC}"
    echo "----------------------------------------"
    
    # Line coverage
    printf "Line Coverage    %6.2f%%  %6.2f%%  " "$before_line" "$after_line"
    if (( $(echo "$line_diff > 0" | bc -l) )); then
        printf "${GREEN}+%.2f%%${NC}\n" "$line_diff"
    elif (( $(echo "$line_diff < 0" | bc -l) )); then
        printf "${RED}%.2f%%${NC}\n" "$line_diff"
    else
        printf "%.2f%%\n" "$line_diff"
    fi
    
    # Function coverage
    printf "Function Coverage %6.2f%%  %6.2f%%  " "$before_func" "$after_func"
    if (( $(echo "$func_diff > 0" | bc -l) )); then
        printf "${GREEN}+%.2f%%${NC}\n" "$func_diff"
    elif (( $(echo "$func_diff < 0" | bc -l) )); then
        printf "${RED}%.2f%%${NC}\n" "$func_diff"
    else
        printf "%.2f%%\n" "$func_diff"
    fi
    
    # Branch coverage
    printf "Branch Coverage  %6.2f%%  %6.2f%%  " "$before_branch" "$after_branch"
    if (( $(echo "$branch_diff > 0" | bc -l) )); then
        printf "${GREEN}+%.2f%%${NC}\n" "$branch_diff"
    elif (( $(echo "$branch_diff < 0" | bc -l) )); then
        printf "${RED}%.2f%%${NC}\n" "$branch_diff"
    else
        printf "%.2f%%\n" "$branch_diff"
    fi
    
    # Generate diff report
    local diff_report="${REPORT_DIR}/coverage_diff_${component}_${TIMESTAMP}.txt"
    {
        echo "Coverage Diff Report for $component"
        echo "Generated: $(date)"
        echo "========================================="
        echo ""
        echo "Before: $before_file"
        echo "After:  $after_file"
        echo ""
        echo "Line Coverage:     $before_line% -> $after_line% (${line_diff:+}$line_diff%)"
        echo "Function Coverage: $before_func% -> $after_func% (${func_diff:+}$func_diff%)"
        echo "Branch Coverage:   $before_branch% -> $after_branch% (${branch_diff:+}$branch_diff%)"
        echo ""
        
        # Check if improvement targets met
        echo "Validation:"
        if (( $(echo "$after_line >= 75" | bc -l) )); then
            echo "✅ Line coverage meets 75% target"
        else
            echo "❌ Line coverage below 75% target"
        fi
        
        if (( $(echo "$after_func >= 80" | bc -l) )); then
            echo "✅ Function coverage meets 80% target"
        else
            echo "❌ Function coverage below 80% target"
        fi
        
        if (( $(echo "$after_branch >= 70" | bc -l) )); then
            echo "✅ Branch coverage meets 70% target"
        else
            echo "⚠️  Branch coverage below 70% target (warning)"
        fi
        
    } > "$diff_report"
    
    echo -e "\n${GREEN}Diff report saved to: $diff_report${NC}"
    
    # Return success if line coverage improved and meets target
    if (( $(echo "$line_diff >= 0 && $after_line >= 75" | bc -l) )); then
        return 0
    else
        return 1
    fi
}

# Function to find coverage files
find_coverage_files() {
    local component=$1
    local pattern="${COVERAGE_DIR}/*${component}*.info"
    
    ls -t $pattern 2>/dev/null | head -10
}

# Main function
main() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}Coverage Diff Tool${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    # Parse arguments
    if [ $# -lt 1 ]; then
        echo "Usage: $0 <component> [before_file] [after_file]"
        echo "  component: Component name (e.g., INTG-001)"
        echo "  before_file: Optional path to before coverage file"
        echo "  after_file: Optional path to after coverage file"
        echo ""
        echo "If files not specified, will use latest two coverage files for component"
        exit 1
    fi
    
    local component=$1
    local before_file=${2:-}
    local after_file=${3:-}
    
    # Create directories if needed
    mkdir -p "$COVERAGE_DIR" "$REPORT_DIR"
    
    # If files not specified, find them
    if [ -z "$before_file" ] || [ -z "$after_file" ]; then
        echo -e "\n${YELLOW}Finding coverage files for ${component}...${NC}"
        
        local files=($(find_coverage_files "$component"))
        
        if [ ${#files[@]} -lt 2 ]; then
            echo -e "${RED}Error: Need at least 2 coverage files to compare${NC}"
            echo "Found files:"
            printf '%s\n' "${files[@]}"
            exit 1
        fi
        
        # Use two most recent files
        after_file="${files[0]}"
        before_file="${files[1]}"
        
        echo "Using files:"
        echo "  Before: $(basename $before_file)"
        echo "  After:  $(basename $after_file)"
    fi
    
    # Verify files exist
    if [ ! -f "$before_file" ]; then
        echo -e "${RED}Error: Before file not found: $before_file${NC}"
        exit 1
    fi
    
    if [ ! -f "$after_file" ]; then
        echo -e "${RED}Error: After file not found: $after_file${NC}"
        exit 1
    fi
    
    # Run comparison
    if compare_coverage "$before_file" "$after_file" "$component"; then
        echo -e "\n${GREEN}✅ Coverage improved and meets targets${NC}"
        exit 0
    else
        echo -e "\n${RED}❌ Coverage does not meet requirements${NC}"
        exit 1
    fi
}

# Run main
main "$@"