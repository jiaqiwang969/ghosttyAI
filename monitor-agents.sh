#!/bin/bash

# Monitor All Agents Status
# Project: Ghostty × tmux Integration
# Date: 2025-01-06

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${BLUE}╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║          Agent Status Monitor - Week 1                    ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${CYAN}Time: $(date '+%Y-%m-%d %H:%M:%S')${NC}"
echo ""

# Function to check agent status
check_agent() {
    local session=$1
    local window=$2
    local role=$3
    local name=$4
    
    echo -e "${YELLOW}[$role] $name${NC} ($session:$window)"
    
    # Check if Claude is running
    if tmux capture-pane -t $session:$window -p 2>/dev/null | tail -5 | grep -q "INSERT"; then
        echo -e "  Status: ${GREEN}✓ Running${NC}"
        
        # Check for recent activity (look for task-related keywords)
        if tmux capture-pane -t $session:$window -p 2>/dev/null | tail -50 | grep -qE "(task|test|implement|analyze|T-[0-9]+)"; then
            echo -e "  Activity: ${GREEN}Working on tasks${NC}"
        else
            echo -e "  Activity: ${YELLOW}Idle/Waiting${NC}"
        fi
        
        # Check cache directory
        if [ -d "/Users/jqwang/98-ghosttyAI/cache/week1/$role" ]; then
            local file_count=$(find /Users/jqwang/98-ghosttyAI/cache/week1/$role -type f 2>/dev/null | wc -l | tr -d ' ')
            echo -e "  Cache Files: ${CYAN}$file_count files${NC}"
        else
            echo -e "  Cache: ${RED}Directory not found${NC}"
        fi
    else
        echo -e "  Status: ${RED}✗ Not running or error${NC}"
    fi
    echo ""
}

# Check all managed agents
echo -e "${BLUE}═══ Project Manager's Team ═══${NC}"
echo ""

echo -e "${CYAN}CORE Development Team:${NC}"
check_agent "ghostty-core" "0" "CORE-001" "c-tmux-specialist"
check_agent "ghostty-core" "4" "CORE-002" "libtmux-core-developer"

echo -e "${CYAN}Integration Team:${NC}"
check_agent "ghostty-integration" "0" "INTG-001" "zig-ghostty-integration"
check_agent "ghostty-integration" "4" "INTG-002" "integration-dev"
check_agent "ghostty-integration" "5" "INTG-003" "performance-eng"

echo -e "${CYAN}Quality Assurance Team:${NC}"
check_agent "ghostty-quality" "0" "QA-001" "qa-test-lead"
check_agent "ghostty-quality" "1" "QA-002" "qa-test-engineer"

echo -e "${BLUE}═══ Independent Consultants ═══${NC}"
echo ""
check_agent "ghostty-tmux-architect" "0" "ARCH-001" "system-architect"
check_agent "ghostty-devops" "0" "OPS-001" "devops-engineer"

# Summary statistics
echo -e "${BLUE}═══ Summary ═══${NC}"
total_agents=9
running_agents=$(tmux list-windows -a -F "#{session_name}:#{window_index}" 2>/dev/null | \
    grep -E "(ghostty-core:(0|4)|ghostty-integration:(0|4|5)|ghostty-quality:(0|1)|ghostty-tmux-architect:0|ghostty-devops:0)" | \
    while read win; do
        if tmux capture-pane -t $win -p 2>/dev/null | tail -5 | grep -q "INSERT"; then
            echo "1"
        fi
    done | wc -l | tr -d ' ')

echo -e "Total Agents: ${CYAN}$total_agents${NC}"
echo -e "Running: ${GREEN}$running_agents${NC}"
echo -e "Not Running: ${RED}$((total_agents - running_agents))${NC}"
echo ""

# Check cache activity
echo -e "${BLUE}═══ Cache Activity (Last 10 minutes) ═══${NC}"
find /Users/jqwang/98-ghosttyAI/cache/week1 -type f -mmin -10 2>/dev/null | while read file; do
    role=$(echo $file | sed 's|.*/week1/\([^/]*\)/.*|\1|')
    filename=$(basename $file)
    echo -e "  ${GREEN}[NEW]${NC} $role: $filename"
done

if [ -z "$(find /Users/jqwang/98-ghosttyAI/cache/week1 -type f -mmin -10 2>/dev/null)" ]; then
    echo -e "  ${YELLOW}No recent cache activity${NC}"
fi

echo ""
echo -e "${BLUE}═══ Recommendations ═══${NC}"

# Check for blockers
if [ $running_agents -lt $total_agents ]; then
    echo -e "${RED}⚠ Some agents are not running. Run ./deploy-all-agents.sh to restart them.${NC}"
fi

# Check for handoffs
if [ -d "/Users/jqwang/98-ghosttyAI/cache/week1" ]; then
    handoff_count=$(find /Users/jqwang/98-ghosttyAI/cache/week1/*/handoffs -type f 2>/dev/null | wc -l | tr -d ' ')
    if [ $handoff_count -gt 0 ]; then
        echo -e "${GREEN}✓ $handoff_count handoffs ready for validation${NC}"
    fi
fi

# Time-based recommendations
hour=$(date +%H)
if [ $hour -eq 9 ]; then
    echo -e "${YELLOW}⏰ Time for morning standup!${NC}"
elif [ $hour -eq 17 ]; then
    echo -e "${YELLOW}⏰ Time to collect daily reports!${NC}"
fi

echo ""
echo -e "${CYAN}Next check: Run this script again in 30 minutes${NC}"
echo -e "${CYAN}Full restart: ./deploy-all-agents.sh${NC}"