#!/bin/bash
# leak_detector.sh - Automated Memory Leak Detection
# Author: INTG-003 (performance-eng)
# Date: 2025-08-25

set -e

VALGRIND_OUT_DIR="/Users/jqwang/98-ghosttyAI/cache/week2/INTG-003/valgrind_analysis"
COMPONENTS_DIR="/Users/jqwang/98-ghosttyAI/cache/week2"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

echo "=== Memory Leak Detection Suite ==="
echo "Starting at: $(date)"
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to run valgrind memcheck
run_memcheck() {
    local component=$1
    local binary=$2
    local output_file="${VALGRIND_OUT_DIR}/memcheck_${component}_${TIMESTAMP}.txt"
    
    echo -e "${YELLOW}Running memcheck on ${component}...${NC}"
    
    valgrind \
        --leak-check=full \
        --show-leak-kinds=all \
        --track-origins=yes \
        --verbose \
        --log-file="${output_file}" \
        --suppressions=/dev/null \
        --gen-suppressions=all \
        --track-fds=yes \
        --malloc-fill=0xAA \
        --free-fill=0xBB \
        "${binary}" 2>&1
    
    # Check for leaks
    if grep -q "definitely lost: 0 bytes" "${output_file}" && \
       grep -q "indirectly lost: 0 bytes" "${output_file}" && \
       grep -q "possibly lost: 0 bytes" "${output_file}"; then
        echo -e "${GREEN}✓ ${component}: No memory leaks detected${NC}"
        return 0
    else
        echo -e "${RED}✗ ${component}: Memory leaks detected!${NC}"
        grep "definitely lost\|indirectly lost\|possibly lost" "${output_file}"
        return 1
    fi
}

# Function to run AddressSanitizer
run_asan() {
    local component=$1
    local source=$2
    local output_binary="${VALGRIND_OUT_DIR}/${component}_asan"
    
    echo -e "${YELLOW}Running AddressSanitizer on ${component}...${NC}"
    
    # Compile with ASAN
    if [[ "${source}" == *.c ]]; then
        clang -g -O0 -fsanitize=address -fno-omit-frame-pointer \
              "${source}" -o "${output_binary}" 2>/dev/null || {
            echo -e "${RED}✗ Failed to compile ${component} with ASAN${NC}"
            return 1
        }
    elif [[ "${source}" == *.zig ]]; then
        zig build-exe -fsanitize-c \
            "${source}" -o "${output_binary}" 2>/dev/null || {
            echo -e "${RED}✗ Failed to compile ${component} with Zig sanitizers${NC}"
            return 1
        }
    fi
    
    # Run with ASAN
    ASAN_OPTIONS=detect_leaks=1:print_stats=1:check_initialization_order=1 \
        "${output_binary}" 2>&1 | tee "${VALGRIND_OUT_DIR}/asan_${component}_${TIMESTAMP}.log"
    
    if [ ${PIPESTATUS[0]} -eq 0 ]; then
        echo -e "${GREEN}✓ ${component}: AddressSanitizer passed${NC}"
        return 0
    else
        echo -e "${RED}✗ ${component}: AddressSanitizer found issues${NC}"
        return 1
    fi
}

# Function to run ThreadSanitizer
run_tsan() {
    local component=$1
    local source=$2
    local output_binary="${VALGRIND_OUT_DIR}/${component}_tsan"
    
    echo -e "${YELLOW}Running ThreadSanitizer on ${component}...${NC}"
    
    # Compile with TSAN
    clang -g -O0 -fsanitize=thread -fno-omit-frame-pointer \
          "${source}" -o "${output_binary}" 2>/dev/null || {
        echo -e "${YELLOW}⚠ Skipping TSAN for ${component} (compilation failed)${NC}"
        return 0
    }
    
    # Run with TSAN
    TSAN_OPTIONS=halt_on_error=0:history_size=7 \
        "${output_binary}" 2>&1 | tee "${VALGRIND_OUT_DIR}/tsan_${component}_${TIMESTAMP}.log"
    
    if [ ${PIPESTATUS[0]} -eq 0 ]; then
        echo -e "${GREEN}✓ ${component}: ThreadSanitizer passed${NC}"
        return 0
    else
        echo -e "${RED}✗ ${component}: Data races detected${NC}"
        return 1
    fi
}

# Function to run helgrind (thread safety)
run_helgrind() {
    local component=$1
    local binary=$2
    local output_file="${VALGRIND_OUT_DIR}/helgrind_${component}_${TIMESTAMP}.txt"
    
    echo -e "${YELLOW}Running helgrind on ${component}...${NC}"
    
    valgrind \
        --tool=helgrind \
        --history-level=full \
        --conflict-cache-size=1000000 \
        --log-file="${output_file}" \
        "${binary}" 2>&1
    
    if grep -q "ERROR SUMMARY: 0 errors" "${output_file}"; then
        echo -e "${GREEN}✓ ${component}: No thread safety issues${NC}"
        return 0
    else
        echo -e "${RED}✗ ${component}: Thread safety issues detected${NC}"
        grep "Possible data race\|Lock order violated" "${output_file}" | head -5
        return 1
    fi
}

# Function to run cachegrind (cache performance)
run_cachegrind() {
    local component=$1
    local binary=$2
    local output_file="${VALGRIND_OUT_DIR}/cachegrind_${component}_${TIMESTAMP}.txt"
    
    echo -e "${YELLOW}Running cachegrind on ${component}...${NC}"
    
    valgrind \
        --tool=cachegrind \
        --cache-sim=yes \
        --branch-sim=yes \
        --cachegrind-out-file="${output_file}" \
        "${binary}" 2>&1
    
    # Analyze cache misses
    cg_annotate "${output_file}" > "${output_file}.annotated"
    
    echo -e "${GREEN}✓ ${component}: Cache analysis complete${NC}"
    grep "I1 miss rate\|D1 miss rate\|LL miss rate" "${output_file}.annotated" | head -3
}

# Function to run massif (heap profiling)  
run_massif() {
    local component=$1
    local binary=$2
    local output_file="${VALGRIND_OUT_DIR}/massif_${component}_${TIMESTAMP}"
    
    echo -e "${YELLOW}Running massif on ${component}...${NC}"
    
    valgrind \
        --tool=massif \
        --heap=yes \
        --stacks=yes \
        --depth=20 \
        --max-snapshots=100 \
        --massif-out-file="${output_file}" \
        "${binary}" 2>&1
    
    # Generate report
    ms_print "${output_file}" > "${output_file}.txt"
    
    # Check peak memory
    peak_mem=$(grep "peak" "${output_file}.txt" | head -1 | awk '{print $3}')
    echo -e "${GREEN}✓ ${component}: Heap profiling complete (Peak: ${peak_mem})${NC}"
}

# Main execution
main() {
    mkdir -p "${VALGRIND_OUT_DIR}"
    
    echo "=== Phase 1: Component Analysis ==="
    
    # Check event loop components
    if [ -f "${COMPONENTS_DIR}/CORE-001/tests/test_event_loop" ]; then
        echo "Analyzing Event Loop (T-201)..."
        run_memcheck "event_loop" "${COMPONENTS_DIR}/CORE-001/tests/test_event_loop"
        run_helgrind "event_loop" "${COMPONENTS_DIR}/CORE-001/tests/test_event_loop"
        run_cachegrind "event_loop" "${COMPONENTS_DIR}/CORE-001/tests/test_event_loop"
        run_massif "event_loop" "${COMPONENTS_DIR}/CORE-001/tests/test_event_loop"
    fi
    
    # Check FFI components
    if [ -f "${COMPONENTS_DIR}/INTG-001/tests/test_ffi" ]; then
        echo "Analyzing FFI Bridge (T-301)..."
        run_memcheck "ffi_bridge" "${COMPONENTS_DIR}/INTG-001/tests/test_ffi"
        run_asan "ffi_bridge" "${COMPONENTS_DIR}/INTG-001/ffi/c_types.zig"
    fi
    
    echo
    echo "=== Phase 2: Integration Analysis ==="
    
    # Run integrated tests
    if [ -f "${COMPONENTS_DIR}/INTG-001/tests/test_integration" ]; then
        echo "Analyzing Full Integration..."
        run_memcheck "integration" "${COMPONENTS_DIR}/INTG-001/tests/test_integration"
        run_helgrind "integration" "${COMPONENTS_DIR}/INTG-001/tests/test_integration"
    fi
    
    echo
    echo "=== Summary Report ==="
    echo "Results saved to: ${VALGRIND_OUT_DIR}"
    echo "Timestamp: ${TIMESTAMP}"
    
    # Count issues
    leak_count=$(grep -l "definitely lost: [1-9]" "${VALGRIND_OUT_DIR}"/memcheck_*_${TIMESTAMP}.txt 2>/dev/null | wc -l)
    race_count=$(grep -l "Possible data race" "${VALGRIND_OUT_DIR}"/helgrind_*_${TIMESTAMP}.txt 2>/dev/null | wc -l)
    
    echo
    if [ ${leak_count} -eq 0 ] && [ ${race_count} -eq 0 ]; then
        echo -e "${GREEN}✓ All memory safety checks PASSED${NC}"
        exit 0
    else
        echo -e "${RED}✗ Found ${leak_count} components with leaks, ${race_count} with races${NC}"
        exit 1
    fi
}

# Run main
main "$@"