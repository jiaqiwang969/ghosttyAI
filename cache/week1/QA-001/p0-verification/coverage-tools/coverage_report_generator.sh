#!/bin/bash

# Coverage Report Generator
# Generate detailed coverage reports with HTML output
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
HTML_DIR="${REPORT_DIR}/html"
SOURCE_DIR="/Users/jqwang/98-ghosttyAI/cache/week1/INTG-001"

# Timestamp
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Create directories
mkdir -p "$COVERAGE_DIR" "$REPORT_DIR" "$HTML_DIR"

# Function to check dependencies
check_dependencies() {
    local deps_ok=true
    
    for tool in gcov lcov genhtml; do
        if ! command -v "$tool" &> /dev/null; then
            echo -e "${RED}✗ $tool is not installed${NC}"
            deps_ok=false
        fi
    done
    
    if [ "$deps_ok" = false ]; then
        echo -e "${RED}Missing required tools. Install lcov package.${NC}"
        exit 1
    fi
}

# Function to collect coverage data
collect_coverage() {
    local component=$1
    local source_path=$2
    local output_file="${COVERAGE_DIR}/coverage_${component}_${TIMESTAMP}.info"
    
    echo -e "${BLUE}Collecting coverage data for ${component}...${NC}"
    
    # Find all gcda files
    local gcda_count=$(find "$source_path" -name "*.gcda" 2>/dev/null | wc -l)
    
    if [ $gcda_count -eq 0 ]; then
        echo -e "${YELLOW}⚠ No coverage data files found (.gcda)${NC}"
        echo "Run tests with coverage enabled first:"
        echo "  gcc -fprofile-arcs -ftest-coverage ..."
        return 1
    fi
    
    echo "Found $gcda_count coverage data files"
    
    # Capture coverage data
    if lcov --capture \
           --directory "$source_path" \
           --output-file "$output_file" \
           --quiet; then
        echo -e "${GREEN}✓ Coverage data collected${NC}"
        
        # Remove system headers and test files
        lcov --remove "$output_file" \
             '/usr/*' \
             '*/test/*' \
             '*/tests/*' \
             --output-file "$output_file" \
             --quiet
        
        echo "Output: $output_file"
        return 0
    else
        echo -e "${RED}✗ Failed to collect coverage data${NC}"
        return 1
    fi
}

# Function to generate HTML report
generate_html_report() {
    local info_file=$1
    local component=$2
    local html_output="${HTML_DIR}/${component}_${TIMESTAMP}"
    
    echo -e "${BLUE}Generating HTML report...${NC}"
    
    if genhtml "$info_file" \
              --output-directory "$html_output" \
              --title "Coverage Report: $component" \
              --legend \
              --show-details \
              --highlight \
              --demangle-cpp \
              --quiet; then
        
        echo -e "${GREEN}✓ HTML report generated${NC}"
        echo "View report: open ${html_output}/index.html"
        
        # Create symlink to latest
        ln -sfn "$(basename $html_output)" "${HTML_DIR}/latest_${component}"
        
        return 0
    else
        echo -e "${RED}✗ Failed to generate HTML report${NC}"
        return 1
    fi
}

# Function to generate text summary
generate_text_summary() {
    local info_file=$1
    local component=$2
    local summary_file="${REPORT_DIR}/coverage_summary_${component}_${TIMESTAMP}.txt"
    
    echo -e "${BLUE}Generating text summary...${NC}"
    
    # Extract metrics using lcov summary
    local summary=$(lcov --summary "$info_file" 2>&1)
    
    {
        echo "Coverage Summary Report"
        echo "Component: $component"
        echo "Generated: $(date)"
        echo "=" * 60
        echo ""
        echo "$summary"
        echo ""
        echo "=" * 60
        
        # Extract key metrics
        local line_cov=$(echo "$summary" | grep "lines\.\.\.\.\.\." | sed 's/.*: //')
        local func_cov=$(echo "$summary" | grep "functions\.\.\." | sed 's/.*: //')
        local branch_cov=$(echo "$summary" | grep "branches\.\.\." | sed 's/.*: //')
        
        echo ""
        echo "Key Metrics:"
        echo "  Line Coverage:     $line_cov"
        echo "  Function Coverage: $func_cov"
        echo "  Branch Coverage:   $branch_cov"
        echo ""
        
        # Check against targets
        echo "Target Validation:"
        
        # Extract percentage from line coverage
        local line_pct=$(echo "$line_cov" | grep -oE '[0-9]+\.[0-9]+' | head -1)
        if (( $(echo "$line_pct >= 75" | bc -l) )); then
            echo "  ✅ Line coverage meets 75% target"
        else
            echo "  ❌ Line coverage below 75% target"
        fi
        
        local func_pct=$(echo "$func_cov" | grep -oE '[0-9]+\.[0-9]+' | head -1)
        if (( $(echo "$func_pct >= 80" | bc -l) )); then
            echo "  ✅ Function coverage meets 80% target"
        else
            echo "  ❌ Function coverage below 80% target"
        fi
        
        echo ""
        echo "Files with lowest coverage:"
        echo "-" * 40
        
        # List files with lowest coverage
        lcov --list "$info_file" 2>/dev/null | \
            tail -n +2 | \
            head -n -2 | \
            sort -t'|' -k2 -n | \
            head -10 | \
            while IFS='|' read -r file line_cov func_cov branch_cov; do
                printf "  %-40s %s\n" "$(basename $file)" "$line_cov"
            done
        
    } > "$summary_file"
    
    echo -e "${GREEN}✓ Text summary saved to: $summary_file${NC}"
    
    # Display summary
    cat "$summary_file"
    
    return 0
}

# Function to combine multiple coverage files
combine_coverage() {
    local output_file="${COVERAGE_DIR}/combined_${TIMESTAMP}.info"
    local files=("$@")
    
    echo -e "${BLUE}Combining ${#files[@]} coverage files...${NC}"
    
    if lcov "${files[@]/#/--add-tracefile }" \
           --output-file "$output_file" \
           --quiet; then
        echo -e "${GREEN}✓ Coverage files combined${NC}"
        echo "$output_file"
        return 0
    else
        echo -e "${RED}✗ Failed to combine coverage files${NC}"
        return 1
    fi
}

# Function to generate component report
generate_component_report() {
    local component=$1
    local source_path=${2:-$SOURCE_DIR}
    
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}Generating Coverage Report: ${component}${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    # Collect coverage data
    if ! collect_coverage "$component" "$source_path"; then
        return 1
    fi
    
    local info_file="${COVERAGE_DIR}/coverage_${component}_${TIMESTAMP}.info"
    
    # Generate HTML report
    generate_html_report "$info_file" "$component"
    
    # Generate text summary
    generate_text_summary "$info_file" "$component"
    
    echo -e "\n${GREEN}✅ Coverage report generation complete${NC}"
    
    return 0
}

# Function to generate master report
generate_master_report() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}Generating Master Coverage Report${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    # Find all recent coverage files
    local files=($(find "$COVERAGE_DIR" -name "*.info" -mtime -1 | head -10))
    
    if [ ${#files[@]} -eq 0 ]; then
        echo -e "${YELLOW}No recent coverage files found${NC}"
        return 1
    fi
    
    echo "Found ${#files[@]} coverage files to combine"
    
    # Combine all coverage files
    local combined_file=$(combine_coverage "${files[@]}")
    
    if [ -z "$combined_file" ]; then
        return 1
    fi
    
    # Generate master HTML report
    generate_html_report "$combined_file" "MASTER"
    
    # Generate master text summary
    generate_text_summary "$combined_file" "MASTER"
    
    return 0
}

# Main function
main() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}Coverage Report Generator${NC}"
    echo -e "${BLUE}Started: $(date)${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    # Check dependencies
    check_dependencies
    
    # Parse arguments
    local component=${1:-INTG-001}
    local source_path=${2:-$SOURCE_DIR}
    
    if [ "$component" = "--help" ] || [ "$component" = "-h" ]; then
        echo "Usage: $0 [component] [source_path]"
        echo "  component: Component name (default: INTG-001)"
        echo "  source_path: Path to source files (default: $SOURCE_DIR)"
        echo ""
        echo "Special commands:"
        echo "  $0 --master    Generate master report combining all components"
        echo "  $0 --clean     Clean all coverage data"
        exit 0
    fi
    
    if [ "$component" = "--clean" ]; then
        echo -e "${YELLOW}Cleaning coverage data...${NC}"
        find "$BASE_DIR" -name "*.gcda" -delete
        find "$BASE_DIR" -name "*.gcno" -delete
        find "$BASE_DIR" -name "*.gcov" -delete
        echo -e "${GREEN}✓ Coverage data cleaned${NC}"
        exit 0
    fi
    
    if [ "$component" = "--master" ]; then
        generate_master_report
        exit $?
    fi
    
    # Generate component report
    generate_component_report "$component" "$source_path"
    
    exit $?
}

# Run main
main "$@"