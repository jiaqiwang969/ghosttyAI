#!/bin/bash

# Integration Test Environment Setup Script
# Version: 1.0.0
# Purpose: Prepare complete test environment for P0 verification
# Date: 2025-08-25

set -euo pipefail

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging setup
LOG_DIR="/Users/jqwang/98-ghosttyAI/cache/week1/QA-001/p0-verification/logs"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_FILE="${LOG_DIR}/${TIMESTAMP}_env_setup.log"

# Create log directory
mkdir -p "${LOG_DIR}"

# Logging function
log() {
    echo -e "${1}" | tee -a "${LOG_FILE}"
}

log "${BLUE}========================================${NC}"
log "${BLUE}Integration Test Environment Setup${NC}"
log "${BLUE}Started: $(date)${NC}"
log "${BLUE}========================================${NC}"

# Function to check command availability
check_command() {
    if ! command -v "$1" &> /dev/null; then
        log "${RED}ERROR: $1 is not installed${NC}"
        return 1
    else
        log "${GREEN}✓ $1 is available${NC}"
        return 0
    fi
}

# Function to check and install dependencies
check_dependencies() {
    log "\n${YELLOW}Checking dependencies...${NC}"
    
    local deps_ok=true
    
    # Core dependencies
    for cmd in gcc make valgrind git tmux; do
        if ! check_command "$cmd"; then
            deps_ok=false
        fi
    done
    
    # Coverage tools
    for cmd in gcov lcov genhtml; do
        if ! check_command "$cmd"; then
            log "${YELLOW}Installing $cmd...${NC}"
            if [[ "$OSTYPE" == "darwin"* ]]; then
                brew install lcov 2>/dev/null || true
            else
                sudo apt-get install -y lcov 2>/dev/null || true
            fi
        fi
    done
    
    # Memory profiling tools (macOS specific)
    if [[ "$OSTYPE" == "darwin"* ]]; then
        check_command "leaks" || deps_ok=false
        check_command "vmmap" || deps_ok=false
    fi
    
    if [ "$deps_ok" = false ]; then
        log "${RED}Some dependencies are missing. Please install them first.${NC}"
        exit 1
    fi
    
    log "${GREEN}All dependencies satisfied${NC}"
}

# Function to setup test directories
setup_directories() {
    log "\n${YELLOW}Setting up test directories...${NC}"
    
    local BASE_DIR="/Users/jqwang/98-ghosttyAI/cache/week1/QA-001/p0-verification"
    
    # Create required directories
    mkdir -p "${BASE_DIR}"/{logs,reports,data,backups,coverage}
    mkdir -p "${BASE_DIR}"/logs/{memory,thread,coverage}
    mkdir -p "${BASE_DIR}"/data/{input,output,mock}
    
    # Set permissions
    chmod -R 755 "${BASE_DIR}"
    
    log "${GREEN}✓ Directory structure created${NC}"
}

# Function to clean previous test data
clean_state() {
    log "\n${YELLOW}Cleaning previous test state...${NC}"
    
    local BASE_DIR="/Users/jqwang/98-ghosttyAI/cache/week1/QA-001/p0-verification"
    
    # Remove old coverage data
    find "${BASE_DIR}" -name "*.gcda" -delete 2>/dev/null || true
    find "${BASE_DIR}" -name "*.gcno" -delete 2>/dev/null || true
    find "${BASE_DIR}" -name "*.info" -delete 2>/dev/null || true
    
    # Clean old logs (keep last 5 runs)
    if [ -d "${BASE_DIR}/logs" ]; then
        ls -t "${BASE_DIR}"/logs/*.log 2>/dev/null | tail -n +6 | xargs rm -f 2>/dev/null || true
    fi
    
    # Kill any lingering test processes
    pkill -f "test_frame_aggregator" 2>/dev/null || true
    pkill -f "test_callback" 2>/dev/null || true
    
    # Clear tmux test sessions
    tmux kill-session -t test-session 2>/dev/null || true
    
    log "${GREEN}✓ Clean state achieved${NC}"
}

# Function to setup mock services
setup_mock_services() {
    log "\n${YELLOW}Setting up mock services...${NC}"
    
    local MOCK_DIR="/Users/jqwang/98-ghosttyAI/cache/week1/QA-001/p0-verification/data/mock"
    
    # Create mock tmux server configuration
    cat > "${MOCK_DIR}/tmux_mock.conf" << 'EOF'
# Mock tmux configuration for testing
set -g default-terminal "screen-256color"
set -g history-limit 10000
set -g status off
set -g aggressive-resize on
EOF
    
    # Create mock callback handler
    cat > "${MOCK_DIR}/mock_callback_handler.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// Mock callback handler for testing
void* mock_callback_handler(void* arg) {
    int id = *(int*)arg;
    printf("Mock callback %d executed\n", id);
    usleep(rand() % 1000);  // Random delay
    return NULL;
}

int start_mock_service() {
    printf("Mock callback service started\n");
    return 0;
}

void stop_mock_service() {
    printf("Mock callback service stopped\n");
}
EOF
    
    # Compile mock service
    gcc -shared -fPIC -o "${MOCK_DIR}/libmock_callback.so" \
        "${MOCK_DIR}/mock_callback_handler.c" -pthread 2>/dev/null || true
    
    log "${GREEN}✓ Mock services configured${NC}"
}

# Function to prepare test data
prepare_test_data() {
    log "\n${YELLOW}Preparing test data...${NC}"
    
    local DATA_DIR="/Users/jqwang/98-ghosttyAI/cache/week1/QA-001/p0-verification/data/input"
    
    # Generate frame test data
    cat > "${DATA_DIR}/frame_test_data.txt" << 'EOF'
FRAME:001:TYPE:TEXT:SIZE:1024:CONTENT:Sample frame data for testing
FRAME:002:TYPE:CONTROL:SIZE:64:CONTENT:ESC[2J
FRAME:003:TYPE:TEXT:SIZE:2048:CONTENT:Large frame with extended content...
EOF
    
    # Generate callback test sequences
    cat > "${DATA_DIR}/callback_sequence.txt" << 'EOF'
CALLBACK:001:PRIORITY:HIGH:THREAD:1
CALLBACK:002:PRIORITY:LOW:THREAD:2
CALLBACK:003:PRIORITY:HIGH:THREAD:1
CALLBACK:004:PRIORITY:MEDIUM:THREAD:3
EOF
    
    # Create stress test configuration
    cat > "${DATA_DIR}/stress_config.json" << 'EOF'
{
    "memory_test": {
        "duration_minutes": 30,
        "frame_rate": 1000,
        "max_memory_growth_kb": 100
    },
    "thread_test": {
        "concurrent_threads": 100,
        "callbacks_per_second": 1000,
        "duration_seconds": 600
    },
    "coverage_target": {
        "line_coverage": 75,
        "branch_coverage": 70,
        "function_coverage": 80
    }
}
EOF
    
    log "${GREEN}✓ Test data prepared${NC}"
}

# Function to verify environment
verify_environment() {
    log "\n${YELLOW}Verifying environment setup...${NC}"
    
    local BASE_DIR="/Users/jqwang/98-ghosttyAI/cache/week1/QA-001/p0-verification"
    local all_good=true
    
    # Check directories exist
    for dir in logs reports data backups coverage; do
        if [ ! -d "${BASE_DIR}/${dir}" ]; then
            log "${RED}✗ Directory ${dir} missing${NC}"
            all_good=false
        fi
    done
    
    # Check mock services
    if [ ! -f "${BASE_DIR}/data/mock/libmock_callback.so" ]; then
        log "${YELLOW}⚠ Mock callback library not built${NC}"
    fi
    
    # Check test data
    if [ ! -f "${BASE_DIR}/data/input/frame_test_data.txt" ]; then
        log "${RED}✗ Test data missing${NC}"
        all_good=false
    fi
    
    # Check available memory
    if [[ "$OSTYPE" == "darwin"* ]]; then
        local free_mem=$(vm_stat | grep "Pages free" | awk '{print $3}' | sed 's/\.//')
        local free_mb=$((free_mem * 4096 / 1048576))
        if [ $free_mb -lt 1000 ]; then
            log "${YELLOW}⚠ Low memory available: ${free_mb}MB${NC}"
        fi
    fi
    
    if [ "$all_good" = true ]; then
        log "${GREEN}✓ Environment verification passed${NC}"
        return 0
    else
        log "${RED}Environment verification failed${NC}"
        return 1
    fi
}

# Function to create environment snapshot
create_snapshot() {
    log "\n${YELLOW}Creating environment snapshot...${NC}"
    
    local SNAPSHOT_DIR="/Users/jqwang/98-ghosttyAI/cache/week1/QA-001/p0-verification/backups"
    local SNAPSHOT_FILE="${SNAPSHOT_DIR}/env_snapshot_${TIMESTAMP}.txt"
    
    {
        echo "Environment Snapshot - $(date)"
        echo "================================"
        echo ""
        echo "System Information:"
        uname -a
        echo ""
        echo "Memory Status:"
        if [[ "$OSTYPE" == "darwin"* ]]; then
            vm_stat
        else
            free -h
        fi
        echo ""
        echo "Process Count:"
        ps aux | wc -l
        echo ""
        echo "Disk Usage:"
        df -h /Users/jqwang/98-ghosttyAI
        echo ""
        echo "Environment Variables:"
        env | grep -E "(PATH|LD_|DYLD_)" | sort
        echo ""
        echo "Compiler Version:"
        gcc --version | head -1
        echo ""
        echo "Git Status:"
        cd /Users/jqwang/98-ghosttyAI && git status --short
    } > "${SNAPSHOT_FILE}"
    
    log "${GREEN}✓ Snapshot saved to ${SNAPSHOT_FILE}${NC}"
}

# Function to setup monitoring
setup_monitoring() {
    log "\n${YELLOW}Setting up monitoring...${NC}"
    
    local MONITOR_SCRIPT="/Users/jqwang/98-ghosttyAI/cache/week1/QA-001/p0-verification/monitor.sh"
    
    cat > "${MONITOR_SCRIPT}" << 'EOF'
#!/bin/bash
# Background monitoring script
INTERVAL=5
LOG_DIR="/Users/jqwang/98-ghosttyAI/cache/week1/QA-001/p0-verification/logs/memory"

while true; do
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)
    
    # Memory monitoring
    if [[ "$OSTYPE" == "darwin"* ]]; then
        vm_stat > "${LOG_DIR}/vmstat_${TIMESTAMP}.log"
        ps aux | head -20 > "${LOG_DIR}/top_processes_${TIMESTAMP}.log"
    else
        free -h > "${LOG_DIR}/memory_${TIMESTAMP}.log"
        top -b -n 1 | head -20 > "${LOG_DIR}/top_${TIMESTAMP}.log"
    fi
    
    sleep $INTERVAL
done
EOF
    
    chmod +x "${MONITOR_SCRIPT}"
    
    log "${GREEN}✓ Monitoring setup complete${NC}"
}

# Main execution
main() {
    log "${BLUE}Step 1: Checking dependencies${NC}"
    check_dependencies
    
    log "\n${BLUE}Step 2: Setting up directories${NC}"
    setup_directories
    
    log "\n${BLUE}Step 3: Cleaning previous state${NC}"
    clean_state
    
    log "\n${BLUE}Step 4: Setting up mock services${NC}"
    setup_mock_services
    
    log "\n${BLUE}Step 5: Preparing test data${NC}"
    prepare_test_data
    
    log "\n${BLUE}Step 6: Creating environment snapshot${NC}"
    create_snapshot
    
    log "\n${BLUE}Step 7: Setting up monitoring${NC}"
    setup_monitoring
    
    log "\n${BLUE}Step 8: Verifying environment${NC}"
    if verify_environment; then
        log "\n${GREEN}========================================${NC}"
        log "${GREEN}Environment setup completed successfully${NC}"
        log "${GREEN}Ready for P0 verification testing${NC}"
        log "${GREEN}Log file: ${LOG_FILE}${NC}"
        log "${GREEN}========================================${NC}"
        exit 0
    else
        log "\n${RED}========================================${NC}"
        log "${RED}Environment setup failed${NC}"
        log "${RED}Please check the log file: ${LOG_FILE}${NC}"
        log "${RED}========================================${NC}"
        exit 1
    fi
}

# Trap errors
trap 'log "${RED}Error occurred at line $LINENO${NC}"; exit 1' ERR

# Run main function
main "$@"