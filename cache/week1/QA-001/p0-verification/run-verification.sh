#!/bin/bash

# P0 Verification Quick Start
# One-command runner for all P0 verifications
# Version: 1.0.0

set -euo pipefail

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

BASE_DIR="/Users/jqwang/98-ghosttyAI/cache/week1/QA-001/p0-verification"

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}P0 VERIFICATION SUITE${NC}"
echo -e "${CYAN}Production Readiness Check${NC}"
echo -e "${CYAN}========================================${NC}"
echo ""

# Check if manual mode requested
MODE=${1:-menu}

show_menu() {
    echo -e "${BLUE}Select verification mode:${NC}"
    echo ""
    echo "  1) Setup Environment Only"
    echo "  2) Run P0 Defect Verifications (DEFECT-001 & 002)"
    echo "  3) Run Coverage Validation (INTG-001)"
    echo "  4) Run Full Regression Suite"
    echo "  5) Run Complete Verification (All Tests)"
    echo "  6) Generate Coverage Reports"
    echo "  7) Check Verification Status"
    echo "  8) Emergency Rollback"
    echo "  9) Exit"
    echo ""
    read -p "Enter selection [1-9]: " selection
    
    case $selection in
        1)
            echo -e "\n${BLUE}Setting up test environment...${NC}"
            "${BASE_DIR}/integration-test-env.sh"
            ;;
        2)
            echo -e "\n${BLUE}Running P0 defect verifications...${NC}"
            "${BASE_DIR}/verification-timeline.sh" manual
            ;;
        3)
            echo -e "\n${BLUE}Running coverage validation...${NC}"
            python3 "${BASE_DIR}/coverage-tools/coverage_validator.py" \
                "/Users/jqwang/98-ghosttyAI/cache/week1/INTG-001" 75
            ;;
        4)
            echo -e "\n${BLUE}Running regression suite...${NC}"
            "${BASE_DIR}/regression-tests/regression_runner.sh"
            ;;
        5)
            echo -e "\n${BLUE}Running complete verification suite...${NC}"
            "${BASE_DIR}/integration-test-env.sh" && \
            "${BASE_DIR}/verification-timeline.sh" manual
            ;;
        6)
            echo -e "\n${BLUE}Generating coverage reports...${NC}"
            "${BASE_DIR}/coverage-tools/coverage_report_generator.sh" INTG-001
            ;;
        7)
            echo -e "\n${BLUE}Checking verification status...${NC}"
            check_status
            ;;
        8)
            echo -e "\n${RED}EMERGENCY ROLLBACK${NC}"
            echo "Are you sure? (yes/no): "
            read confirm
            if [ "$confirm" = "yes" ]; then
                emergency_rollback
            fi
            ;;
        9)
            echo -e "${GREEN}Exiting...${NC}"
            exit 0
            ;;
        *)
            echo -e "${RED}Invalid selection${NC}"
            exit 1
            ;;
    esac
}

check_status() {
    echo -e "${CYAN}Verification Status Report${NC}"
    echo "======================================"
    
    # Check for pass/fail markers
    if ls "${BASE_DIR}"/reports/REGRESSION_PASS_* 2>/dev/null | head -1 > /dev/null; then
        echo -e "Regression Tests: ${GREEN}✅ PASSED${NC}"
    elif ls "${BASE_DIR}"/reports/REGRESSION_FAIL_* 2>/dev/null | head -1 > /dev/null; then
        echo -e "Regression Tests: ${RED}❌ FAILED${NC}"
    else
        echo -e "Regression Tests: ${YELLOW}⚠️  NOT RUN${NC}"
    fi
    
    # Check for rollback markers
    if ls "${BASE_DIR}"/reports/ROLLBACK_REQUIRED_* 2>/dev/null | head -1 > /dev/null; then
        echo -e "\n${RED}⚠️  ROLLBACK REQUIRED${NC}"
        echo "Rollback markers found:"
        ls -1 "${BASE_DIR}"/reports/ROLLBACK_REQUIRED_* 2>/dev/null
    fi
    
    # Show recent reports
    echo -e "\n${BLUE}Recent Reports:${NC}"
    ls -lt "${BASE_DIR}"/reports/*.txt 2>/dev/null | head -5
    
    # Show recent logs
    echo -e "\n${BLUE}Recent Logs:${NC}"
    ls -lt "${BASE_DIR}"/logs/*/*.log 2>/dev/null | head -5
}

emergency_rollback() {
    echo -e "${RED}Initiating emergency rollback...${NC}"
    
    # Create rollback marker
    touch "${BASE_DIR}/reports/EMERGENCY_ROLLBACK_$(date +%Y%m%d_%H%M%S)"
    
    # Show rollback instructions
    cat << EOF

EMERGENCY ROLLBACK PROCEDURE:
==============================

1. IMMEDIATE ACTIONS:
   - Stop all deployments
   - Notify team leads
   - Check system status

2. ROLLBACK COMMANDS:
   git revert HEAD~1
   git push origin main --force-with-lease
   
3. RESTORE PREVIOUS BUILD:
   cd /path/to/deployment
   ./restore-previous-version.sh
   
4. VERIFY ROLLBACK:
   - Run smoke tests
   - Check system health
   - Monitor error rates

5. POST-MORTEM:
   - Document failure
   - Update test suite
   - Schedule review meeting

EOF
    
    echo -e "${YELLOW}Manual intervention required${NC}"
}

# Quick commands
case "$MODE" in
    setup)
        "${BASE_DIR}/integration-test-env.sh"
        ;;
    defects)
        "${BASE_DIR}/verification-timeline.sh" manual
        ;;
    coverage)
        python3 "${BASE_DIR}/coverage-tools/coverage_validator.py" \
            "/Users/jqwang/98-ghosttyAI/cache/week1/INTG-001" 75
        ;;
    regression)
        "${BASE_DIR}/regression-tests/regression_runner.sh"
        ;;
    all)
        "${BASE_DIR}/integration-test-env.sh" && \
        "${BASE_DIR}/verification-timeline.sh" manual
        ;;
    status)
        check_status
        ;;
    menu|*)
        show_menu
        ;;
esac

echo -e "\n${GREEN}P0 Verification Complete${NC}"
echo -e "Check reports in: ${BASE_DIR}/reports/"