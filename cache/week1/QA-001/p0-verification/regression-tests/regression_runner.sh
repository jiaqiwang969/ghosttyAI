#!/bin/bash

# Regression Test Suite Runner
# Automated execution of all regression tests
# Version: 1.0.0

set -euo pipefail

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Test directory
TEST_DIR="/Users/jqwang/98-ghosttyAI/cache/week1/QA-001/p0-verification/regression-tests"
REPORT_DIR="/Users/jqwang/98-ghosttyAI/cache/week1/QA-001/p0-verification/reports"
LOG_DIR="/Users/jqwang/98-ghosttyAI/cache/week1/QA-001/p0-verification/logs"

# Timestamp
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
REPORT_FILE="${REPORT_DIR}/regression_report_${TIMESTAMP}.txt"
SUMMARY_FILE="${REPORT_DIR}/regression_summary_${TIMESTAMP}.json"

# Create directories
mkdir -p "${REPORT_DIR}" "${LOG_DIR}"

# Logging function
log() {
    echo -e "${1}" | tee -a "${REPORT_FILE}"
}

# Function to compile test
compile_test() {
    local test_name=$1
    local source_file="${TEST_DIR}/${test_name}.c"
    local binary_file="${TEST_DIR}/${test_name}"
    
    log "${BLUE}Compiling ${test_name}...${NC}"
    
    if gcc -o "${binary_file}" "${source_file}" \
           -pthread -Wall -Wextra -g -O2 2>&1 | tee -a "${REPORT_FILE}"; then
        log "${GREEN}✓ Compilation successful${NC}"
        return 0
    else
        log "${RED}✗ Compilation failed${NC}"
        return 1
    fi
}

# Function to run test with timeout
run_test() {
    local test_name=$1
    local binary_file="${TEST_DIR}/${test_name}"
    local test_log="${LOG_DIR}/${test_name}_${TIMESTAMP}.log"
    local timeout_seconds=300  # 5 minutes timeout
    
    log "\n${BLUE}Running ${test_name}...${NC}"
    
    # Run test with timeout
    if timeout ${timeout_seconds} "${binary_file}" > "${test_log}" 2>&1; then
        local exit_code=$?
        if [ $exit_code -eq 0 ]; then
            log "${GREEN}✅ ${test_name}: PASSED${NC}"
            cat "${test_log}" | grep -E "(✅|PASS)" | tail -5 | while read line; do
                log "  ${line}"
            done
            return 0
        else
            log "${RED}❌ ${test_name}: FAILED (exit code: ${exit_code})${NC}"
            cat "${test_log}" | grep -E "(❌|FAIL)" | tail -5 | while read line; do
                log "  ${line}"
            done
            return 1
        fi
    else
        log "${RED}❌ ${test_name}: TIMEOUT (exceeded ${timeout_seconds}s)${NC}"
        return 1
    fi
}

# Function to check system resources
check_resources() {
    log "\n${YELLOW}System Resource Check:${NC}"
    
    # Memory check
    if [[ "$OSTYPE" == "darwin"* ]]; then
        local free_mem=$(vm_stat | grep "Pages free" | awk '{print $3}' | sed 's/\.//')
        local free_mb=$((free_mem * 4096 / 1048576))
        log "  Available memory: ${free_mb}MB"
        
        if [ $free_mb -lt 500 ]; then
            log "  ${YELLOW}⚠ Warning: Low memory available${NC}"
        fi
    else
        free -h | tee -a "${REPORT_FILE}"
    fi
    
    # CPU load
    if [[ "$OSTYPE" == "darwin"* ]]; then
        local load=$(uptime | awk '{print $(NF-2) $(NF-1) $NF}')
        log "  System load: ${load}"
    else
        uptime | tee -a "${REPORT_FILE}"
    fi
    
    # Disk space
    local disk_usage=$(df -h . | awk 'NR==2 {print $5}' | sed 's/%//')
    log "  Disk usage: ${disk_usage}%"
    
    if [ $disk_usage -gt 90 ]; then
        log "  ${RED}⚠ Critical: Disk space low${NC}"
        return 1
    fi
    
    return 0
}

# Function to run tests in parallel
run_parallel_tests() {
    local pids=()
    local test_results=()
    
    # Start tests in background
    for test in test_frame_aggregator_memory test_callback_thread_safety test_existing_functionality; do
        (
            compile_test "${test}" && run_test "${test}"
            echo $? > "${TEST_DIR}/.${test}.result"
        ) &
        pids+=($!)
        test_results+=("${test}")
    done
    
    # Wait for all tests
    local all_passed=true
    for i in "${!pids[@]}"; do
        wait ${pids[$i]}
        local test_name="${test_results[$i]}"
        local result_file="${TEST_DIR}/.${test_name}.result"
        
        if [ -f "${result_file}" ]; then
            local result=$(cat "${result_file}")
            if [ "$result" != "0" ]; then
                all_passed=false
            fi
            rm -f "${result_file}"
        else
            all_passed=false
        fi
    done
    
    if [ "$all_passed" = true ]; then
        return 0
    else
        return 1
    fi
}

# Function to generate JSON summary
generate_json_summary() {
    local total=$1
    local passed=$2
    local failed=$3
    local duration=$4
    
    cat > "${SUMMARY_FILE}" << EOF
{
    "timestamp": "${TIMESTAMP}",
    "date": "$(date -Iseconds)",
    "results": {
        "total_tests": ${total},
        "passed": ${passed},
        "failed": ${failed},
        "success_rate": $(echo "scale=2; ${passed}*100/${total}" | bc)
    },
    "duration_seconds": ${duration},
    "tests": {
        "DEFECT-001": {
            "name": "Memory Leak Verification",
            "status": "$(grep -q 'DEFECT-001.*PASSED' ${REPORT_FILE} && echo 'PASSED' || echo 'FAILED')"
        },
        "DEFECT-002": {
            "name": "Race Condition Verification",
            "status": "$(grep -q 'DEFECT-002.*PASSED' ${REPORT_FILE} && echo 'PASSED' || echo 'FAILED')"
        },
        "REGRESSION": {
            "name": "Existing Functionality",
            "status": "$(grep -q 'REGRESSION.*PASSED' ${REPORT_FILE} && echo 'PASSED' || echo 'FAILED')"
        }
    }
}
EOF
    
    log "\n${GREEN}JSON summary saved to: ${SUMMARY_FILE}${NC}"
}

# Function to check if tests can proceed
pre_flight_check() {
    log "${BLUE}========================================${NC}"
    log "${BLUE}Pre-flight Checks${NC}"
    log "${BLUE}========================================${NC}"
    
    # Check for required tools
    local tools_ok=true
    for tool in gcc make timeout; do
        if ! command -v "$tool" &> /dev/null; then
            log "${RED}✗ ${tool} is not installed${NC}"
            tools_ok=false
        else
            log "${GREEN}✓ ${tool} is available${NC}"
        fi
    done
    
    if [ "$tools_ok" = false ]; then
        log "${RED}Missing required tools. Cannot proceed.${NC}"
        return 1
    fi
    
    # Check resources
    if ! check_resources; then
        log "${RED}System resources insufficient. Cannot proceed.${NC}"
        return 1
    fi
    
    log "${GREEN}✓ All pre-flight checks passed${NC}"
    return 0
}

# Main execution
main() {
    log "${BLUE}========================================${NC}"
    log "${BLUE}Regression Test Suite Runner${NC}"
    log "${BLUE}Started: $(date)${NC}"
    log "${BLUE}========================================${NC}"
    
    # Pre-flight checks
    if ! pre_flight_check; then
        exit 1
    fi
    
    log "\n${BLUE}========================================${NC}"
    log "${BLUE}Compiling and Running Tests${NC}"
    log "${BLUE}========================================${NC}"
    
    local start_time=$(date +%s)
    local total_tests=3
    local passed_tests=0
    local failed_tests=0
    
    # Test 1: Memory Leak (DEFECT-001)
    if compile_test "test_frame_aggregator_memory" && \
       run_test "test_frame_aggregator_memory"; then
        ((passed_tests++))
    else
        ((failed_tests++))
    fi
    
    # Test 2: Thread Safety (DEFECT-002)
    if compile_test "test_callback_thread_safety" && \
       run_test "test_callback_thread_safety"; then
        ((passed_tests++))
    else
        ((failed_tests++))
    fi
    
    # Test 3: Existing Functionality
    if compile_test "test_existing_functionality" && \
       run_test "test_existing_functionality"; then
        ((passed_tests++))
    else
        ((failed_tests++))
    fi
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    # Generate reports
    log "\n${BLUE}========================================${NC}"
    log "${BLUE}Test Results Summary${NC}"
    log "${BLUE}========================================${NC}"
    log "Total Tests: ${total_tests}"
    log "Passed: ${passed_tests}"
    log "Failed: ${failed_tests}"
    log "Duration: ${duration} seconds"
    log "Success Rate: $(echo "scale=1; ${passed_tests}*100/${total_tests}" | bc)%"
    
    # Generate JSON summary
    generate_json_summary ${total_tests} ${passed_tests} ${failed_tests} ${duration}
    
    # Determine overall result
    if [ ${passed_tests} -eq ${total_tests} ]; then
        log "\n${GREEN}========================================${NC}"
        log "${GREEN}✅ ALL REGRESSION TESTS PASSED${NC}"
        log "${GREEN}System is ready for deployment${NC}"
        log "${GREEN}========================================${NC}"
        
        # Create success marker
        touch "${REPORT_DIR}/REGRESSION_PASS_${TIMESTAMP}"
        exit 0
    else
        log "\n${RED}========================================${NC}"
        log "${RED}❌ REGRESSION TESTS FAILED${NC}"
        log "${RED}DO NOT PROCEED WITH DEPLOYMENT${NC}"
        log "${RED}Failed tests: ${failed_tests}/${total_tests}${NC}"
        log "${RED}========================================${NC}"
        
        # Create failure marker
        touch "${REPORT_DIR}/REGRESSION_FAIL_${TIMESTAMP}"
        
        # Show which tests failed
        log "\n${RED}Failed Test Details:${NC}"
        grep -E "❌.*FAILED" "${REPORT_FILE}" | while read line; do
            log "  ${line}"
        done
        
        exit 1
    fi
}

# Handle interrupts
trap 'log "${RED}Interrupted by user${NC}"; exit 1' INT TERM

# Run main function
main "$@"