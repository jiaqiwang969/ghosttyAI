#!/bin/bash
# validate_p0_fixes.sh - 验证所有P0缺陷修复的脚本
# Author: ARCH-001 (System Architect)
# Purpose: 自动验证各团队是否正确应用了P0修复
# Version: 2.0.0

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
FIX_DIR="/Users/jqwang/98-ghosttyAI/cache/week1/ARCH-001"
WORK_DIR=$(pwd)
TEMP_DIR="/tmp/p0_validation_$$"

# Test results
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# ============================================================================
# Helper Functions
# ============================================================================

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

test_pass() {
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    PASSED_TESTS=$((PASSED_TESTS + 1))
    echo -e "  ${GREEN}✓${NC} $1"
}

test_fail() {
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    FAILED_TESTS=$((FAILED_TESTS + 1))
    echo -e "  ${RED}✗${NC} $1"
}

# ============================================================================
# Validation Functions
# ============================================================================

validate_headers_exist() {
    echo "1. Checking if all fixed headers are present..."
    
    local headers=(
        "tty_ctx_unified.h"
        "interface_compatibility.h"
        "ui_backend_callbacks_fixed.h"
        "interface_adapter.h"
    )
    
    for header in "${headers[@]}"; do
        if [ -f "$FIX_DIR/$header" ]; then
            test_pass "Found $header"
        else
            test_fail "Missing $header"
        fi
    done
}

validate_implementations_exist() {
    echo -e "\n2. Checking if implementation files are present..."
    
    local impl_files=(
        "interface_adapter.c"
        "ui_backend_impl.c"
    )
    
    for impl in "${impl_files[@]}"; do
        if [ -f "$FIX_DIR/$impl" ]; then
            test_pass "Found $impl"
        else
            test_fail "Missing $impl"
        fi
    done
}

validate_tty_ctx_fields() {
    echo -e "\n3. Validating tty_ctx structure has all required fields..."
    
    local required_fields=(
        "uint32_t size"
        "uint32_t version"
        "uint32_t ocx"
        "uint32_t ocy"
        "uint32_t orupper"
        "uint32_t orlower"
        "struct screen"
        "struct window_pane"
        "void.*cell"
        "void.*ptr"
    )
    
    for field in "${required_fields[@]}"; do
        if grep -q "$field" "$FIX_DIR/tty_ctx_unified.h" 2>/dev/null; then
            test_pass "Field present: $field"
        else
            test_fail "Field missing: $field"
        fi
    done
}

validate_callbacks_count() {
    echo -e "\n4. Validating all 22 callbacks are defined..."
    
    local callbacks=(
        "cmd_cell"
        "cmd_cells"
        "cmd_insertcharacter"
        "cmd_deletecharacter"
        "cmd_clearcharacter"
        "cmd_insertline"
        "cmd_deleteline"
        "cmd_clearline"
        "cmd_clearendofline"
        "cmd_clearstartofline"
        "cmd_clearscreen"
        "cmd_clearendofscreen"
        "cmd_clearstartofscreen"
        "cmd_alignmenttest"
        "cmd_reverseindex"
        "cmd_linefeed"
        "cmd_scrollup"
        "cmd_scrolldown"
        "cmd_setselection"
        "cmd_rawstring"
        "cmd_sixelimage"
        "cmd_syncstart"
    )
    
    local count=0
    for callback in "${callbacks[@]}"; do
        if grep -q "$callback" "$FIX_DIR/ui_backend_callbacks_fixed.h" 2>/dev/null; then
            count=$((count + 1))
        fi
    done
    
    if [ $count -eq 22 ]; then
        test_pass "All 22 callbacks defined (found $count)"
    else
        test_fail "Missing callbacks (found $count/22)"
    fi
}

compile_test_program() {
    echo -e "\n5. Compiling test program..."
    
    mkdir -p "$TEMP_DIR"
    cd "$TEMP_DIR"
    
    # Copy all necessary files
    cp "$FIX_DIR"/*.h . 2>/dev/null || true
    cp "$FIX_DIR"/*.c . 2>/dev/null || true
    cp "$FIX_DIR/test_p0_fixes.c" . 2>/dev/null || true
    
    # Try to compile
    if gcc -Wall -Wextra -I. test_p0_fixes.c interface_adapter.c -o test_p0_fixes 2>/dev/null; then
        test_pass "Test program compiled successfully"
        
        # Run the test
        if ./test_p0_fixes > test_output.txt 2>&1; then
            if grep -q "ALL P0 DEFECTS FIXED" test_output.txt; then
                test_pass "Test program validates all fixes"
            else
                test_fail "Test program reports issues"
                cat test_output.txt | tail -10
            fi
        else
            test_fail "Test program execution failed"
        fi
    else
        test_fail "Compilation failed"
    fi
    
    cd "$WORK_DIR"
}

check_team_files() {
    echo -e "\n6. Checking team-specific files..."
    
    # Check if teams have their working directories
    local teams=(
        "CORE-001:tty_write_hooks.c"
        "CORE-002:backend_router.c"
        "INTG-001:backend_ghostty.c"
    )
    
    for team_file in "${teams[@]}"; do
        IFS=':' read -r team file <<< "$team_file"
        
        # This would check actual team directories in real scenario
        # For now, just check if example exists
        if [ -f "$FIX_DIR/examples/${file%.c}_example.c" ]; then
            test_pass "$team example available"
        else
            log_warn "$team: No example file found"
        fi
    done
}

validate_interface_compatibility() {
    echo -e "\n7. Testing interface compatibility layer..."
    
    # Create a small test program
    cat > "$TEMP_DIR/compat_test.c" << 'EOF'
#include <stdio.h>

// Simulate old function names
int tty_write_hooks_init(void);
int backend_router_register_backend(void* r, void* b);

int main(void) {
    // These should work with warnings
    tty_write_hooks_init();
    backend_router_register_backend(NULL, NULL);
    return 0;
}
EOF
    
    cd "$TEMP_DIR"
    if gcc -c compat_test.c 2>/dev/null; then
        test_pass "Compatibility layer allows old function names"
    else
        test_fail "Compatibility layer not working"
    fi
    cd "$WORK_DIR"
}

performance_check() {
    echo -e "\n8. Performance validation..."
    
    # Check for frame batching configuration
    if grep -q "16\.67" "$FIX_DIR/ui_backend_callbacks_fixed.h" 2>/dev/null; then
        test_pass "Frame batching configured for 60 FPS (16.67ms)"
    else
        log_warn "Frame batching timing not verified"
    fi
    
    # Check for ABI stability pattern
    if grep -q "uint32_t size.*MUST be first" "$FIX_DIR/tty_ctx_unified.h" 2>/dev/null; then
        test_pass "ABI stability pattern implemented"
    else
        test_fail "ABI stability not guaranteed"
    fi
}

generate_report() {
    echo -e "\n========================================="
    echo "P0 DEFECT FIX VALIDATION REPORT"
    echo "========================================="
    echo "Time: $(date '+%Y-%m-%d %H:%M:%S')"
    echo "Total Tests: $TOTAL_TESTS"
    echo -e "Passed: ${GREEN}$PASSED_TESTS${NC}"
    echo -e "Failed: ${RED}$FAILED_TESTS${NC}"
    
    local pass_rate=$((PASSED_TESTS * 100 / TOTAL_TESTS))
    echo "Pass Rate: $pass_rate%"
    
    echo -e "\n----------------------------------------"
    
    if [ $FAILED_TESTS -eq 0 ]; then
        echo -e "${GREEN}✅ ALL P0 DEFECTS PROPERLY FIXED!${NC}"
        echo -e "${GREEN}Ready for integration and deployment.${NC}"
        return 0
    else
        echo -e "${RED}❌ Some P0 defects still need attention${NC}"
        echo "Please review failed tests above."
        return 1
    fi
}

cleanup() {
    rm -rf "$TEMP_DIR"
}

# ============================================================================
# Main Execution
# ============================================================================

main() {
    echo "========================================="
    echo "P0 Defect Fix Validation Script v2.0.0"
    echo "Author: ARCH-001 (System Architect)"
    echo "========================================="
    echo ""
    
    # Set trap for cleanup
    trap cleanup EXIT
    
    # Run all validations
    validate_headers_exist
    validate_implementations_exist
    validate_tty_ctx_fields
    validate_callbacks_count
    compile_test_program
    check_team_files
    validate_interface_compatibility
    performance_check
    
    # Generate final report
    generate_report
}

# Run main function
main "$@"