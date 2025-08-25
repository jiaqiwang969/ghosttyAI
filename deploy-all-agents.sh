#!/bin/bash

# Deploy All Agents for Ghostty × tmux Integration Project
# Project Manager's Team Deployment Script
# Date: 2025-01-06

PROJECT_ROOT="/Users/jqwang/98-ghosttyAI"
CLAUDE_CMD="claude --dangerously-skip-permissions"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   Deploying All Project Manager's Agents                 ║${NC}"
echo -e "${BLUE}║   Ghostty × tmux Integration Project                     ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════╝${NC}"
echo ""

# Function to deploy an agent
deploy_agent() {
    local session=$1
    local window=$2
    local agent_type=$3
    local role=$4
    local briefing=$5
    
    echo -e "${YELLOW}Deploying $agent_type to $session:$window...${NC}"
    
    # Check if window exists, if not create it
    if ! tmux list-windows -t $session 2>/dev/null | grep -q "^$window:"; then
        tmux new-window -t $session:$window -n "$agent_type" -c "$PROJECT_ROOT"
        sleep 1
    fi
    
    # Start Claude with required flag
    tmux send-keys -t $session:$window "$CLAUDE_CMD" Enter
    sleep 5
    
    # Send agent briefing
    tmux send-keys -t $session:$window "$briefing" Enter
    sleep 2
    
    echo -e "${GREEN}✓ $agent_type deployed successfully${NC}"
    echo ""
}

# ═══════════════════════════════════════════════════════════════
# CORE DEVELOPMENT TEAM (C/tmux specialists)
# ═══════════════════════════════════════════════════════════════

echo -e "${BLUE}═══ CORE Development Team ═══${NC}"

# CORE-001: c-tmux-specialist
deploy_agent "ghostty-core" "0" "c-tmux-specialist" "CORE-001" \
"You are the c-tmux-specialist (CORE-001). Your responsibilities:

PRIMARY FOCUS: Extract and implement tty_write hooks from tmux source
- Analyze tmux/tty.c and tmux/screen-write.c (READ-ONLY)
- Identify ALL tty_cmd_* functions for hook extraction
- Create ui_backend.h interface design
- Work in cache/week1/CORE-001/ using TDD methodology

WEEK 1 TASKS:
- T-101: Extract tty_write hooks (2 days)
- T-102: Create hook inventory and vtable design

YOUR WORKFLOW:
1. Read /docs/任务清单/第一周/CORE-001.md for detailed tasks
2. Analyze source in /Users/jqwang/98-ghosttyAI/tmux/
3. Write tests FIRST in cache/week1/CORE-001/tests/
4. Implement in cache/week1/CORE-001/wip/
5. Report daily to Project Manager

ACCEPTANCE CRITERIA:
- All tty_cmd_* functions identified (>30 expected)
- Complete vtable design in ui_backend.h
- Tests passing with >80% coverage
- Performance overhead <5%

Start by reading your task file and analyzing tmux/tty.c"

# CORE-002: libtmux-core-developer  
deploy_agent "ghostty-core" "4" "libtmux-core-developer" "CORE-002" \
"You are the libtmux-core-developer (CORE-002). Your responsibilities:

PRIMARY FOCUS: Implement backend router for libtmuxcore
- Build on CORE-001's hook extraction work
- Create backend_router.c with vtable routing
- Maintain TTY backward compatibility
- Work in cache/week1/CORE-002/ using TDD methodology

WEEK 1 TASKS:
- T-103: Implement backend router (3 days)
- T-104: Create TTY fallback mechanism

YOUR WORKFLOW:
1. Read /docs/任务清单/第一周/CORE-002.md for detailed tasks
2. Wait for CORE-001's ui_backend.h handoff
3. Write tests FIRST in cache/week1/CORE-002/tests/
4. Implement in cache/week1/CORE-002/wip/
5. Report daily to Project Manager

ACCEPTANCE CRITERIA:
- Router correctly dispatches all vtable calls
- TTY backend preserves existing behavior
- Zero performance regression
- Clean compilation with -Wall -Wextra

Start by reading your task file and checking CORE-001's progress"

# ═══════════════════════════════════════════════════════════════
# INTEGRATION TEAM (Zig/FFI specialists)
# ═══════════════════════════════════════════════════════════════

echo -e "${BLUE}═══ Integration Team ═══${NC}"

# INTG-001: zig-ghostty-integration (Senior)
deploy_agent "ghostty-integration" "0" "zig-ghostty-integration" "INTG-001" \
"You are the zig-ghostty-integration specialist (INTG-001). Your responsibilities:

PRIMARY FOCUS: Create Ghostty backend and FFI bridge
- Implement backend_ghostty.c for Ghostty callbacks
- Build Zig FFI bridge for C-Zig interop
- Handle memory safety at language boundaries
- Work in cache/week1/INTG-001/ using TDD methodology

WEEK 1 TASKS:
- T-201: Implement Ghostty backend (2 days)
- T-202: Create Zig FFI bridge

YOUR WORKFLOW:
1. Read /docs/任务清单/第一周/INTG-001.md for detailed tasks
2. Analyze ghostty/src/ for integration points
3. Write tests FIRST in cache/week1/INTG-001/tests/
4. Implement in cache/week1/INTG-001/wip/
5. Report daily to Project Manager

ACCEPTANCE CRITERIA:
- Callbacks properly batched (<1 per vsync)
- Memory safety verified (no leaks)
- FFI bridge handles all data types
- Zig compiler reports zero unsafe operations

Start by reading your task file and analyzing Ghostty source"

# INTG-002: Integration Developer
deploy_agent "ghostty-integration" "4" "integration-dev" "INTG-002" \
"You are the Integration Developer (INTG-002). Your responsibilities:

PRIMARY FOCUS: Support INTG-001 and handle auxiliary integration
- Assist with FFI bridge implementation
- Create helper functions and utilities
- Handle edge cases and error conditions
- Work in cache/week1/INTG-002/ using TDD methodology

WEEK 1 TASKS:
- Support INTG-001's FFI bridge development
- Implement error handling mechanisms
- Create integration test helpers

YOUR WORKFLOW:
1. Coordinate with INTG-001 for task distribution
2. Write tests FIRST in cache/week1/INTG-002/tests/
3. Implement in cache/week1/INTG-002/wip/
4. Report daily to Project Manager

ACCEPTANCE CRITERIA:
- All error paths handled gracefully
- Helper functions well-tested
- Integration with INTG-001's work seamless

Start by checking with INTG-001 for specific tasks"

# INTG-003: Performance Engineer
deploy_agent "ghostty-integration" "5" "performance-eng" "INTG-003" \
"You are the Performance Engineer (INTG-003). Your responsibilities:

PRIMARY FOCUS: Optimize integration performance
- Profile callback frequency and overhead
- Optimize batching and buffering
- Measure and reduce latency
- Work in cache/week1/INTG-003/ using benchmark-driven development

WEEK 1 TASKS:
- Establish performance baselines
- Profile integration overhead
- Optimize critical paths

YOUR WORKFLOW:
1. Create performance benchmarks in cache/week1/INTG-003/benchmarks/
2. Profile using instruments/dtrace
3. Document findings in daily reports
4. Report metrics to Project Manager

ACCEPTANCE CRITERIA:
- Callback frequency ≤60Hz
- Integration overhead <2%
- Memory usage within +10% of baseline
- Latency <1ms for operations

Start by creating baseline measurements"

# ═══════════════════════════════════════════════════════════════
# QUALITY ASSURANCE TEAM
# ═══════════════════════════════════════════════════════════════

echo -e "${BLUE}═══ Quality Assurance Team ═══${NC}"

# QA-001: qa-test-lead
deploy_agent "ghostty-quality" "0" "qa-test-lead" "QA-001" \
"You are the QA Test Lead (QA-001). Your responsibilities:

PRIMARY FOCUS: Test strategy and framework development
- Create comprehensive test framework
- Define test strategies and coverage goals
- Coordinate QA-002's testing efforts
- Work in cache/week1/QA-001/ for test infrastructure

WEEK 1 TASKS:
- T-301: Establish test framework (2 days)
- T-302: Create test plan and strategy
- Coordinate with all teams for testability

YOUR WORKFLOW:
1. Read /docs/任务清单/第一周/QA-001.md for detailed tasks
2. Set up test infrastructure in cache/week1/QA-001/
3. Define coverage requirements for each component
4. Guide QA-002 on specific test areas
5. Report test metrics to Project Manager

ACCEPTANCE CRITERIA:
- Test framework supports unit/integration/performance tests
- Coverage tracking automated
- Test execution <5 minutes for full suite
- All components have test plans

Start by reading your task file and setting up test framework"

# QA-002: qa-test-engineer
deploy_agent "ghostty-quality" "1" "qa-test-engineer" "QA-002" \
"You are the QA Test Engineer (QA-002). Your responsibilities:

PRIMARY FOCUS: Write and execute comprehensive tests
- Test grid operations and callbacks
- Verify Unicode/emoji handling
- Test performance and memory usage
- Work in cache/week1/QA-002/ using TDD methodology

WEEK 1 TASKS:
- T-303: Grid operation tests (2 days)
- T-304: Callback verification tests
- T-305: Performance test suite

YOUR WORKFLOW:
1. Read /docs/任务清单/第一周/QA-002.md for detailed tasks
2. Write comprehensive test suites in cache/week1/QA-002/tests/
3. Execute tests against all team deliverables
4. Generate coverage reports
5. Report to QA-001 and Project Manager

ACCEPTANCE CRITERIA:
- >80% code coverage achieved
- All edge cases tested (Unicode, wide chars, etc.)
- Performance benchmarks established
- No memory leaks detected

Start by reading your task file and coordinating with QA-001"

# ═══════════════════════════════════════════════════════════════
# DEPLOYMENT SUMMARY
# ═══════════════════════════════════════════════════════════════

echo -e "${BLUE}╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║                  DEPLOYMENT COMPLETE                      ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${GREEN}Successfully deployed agents:${NC}"
echo "  ✓ CORE-001: c-tmux-specialist (ghostty-core:0)"
echo "  ✓ CORE-002: libtmux-core-developer (ghostty-core:4)"
echo "  ✓ INTG-001: zig-ghostty-integration (ghostty-integration:0)"
echo "  ✓ INTG-002: integration-dev (ghostty-integration:4)"
echo "  ✓ INTG-003: performance-eng (ghostty-integration:5)"
echo "  ✓ QA-001: qa-test-lead (ghostty-quality:0)"
echo "  ✓ QA-002: qa-test-engineer (ghostty-quality:1)"
echo ""
echo -e "${YELLOW}Independent Consultants (not deployed by PM):${NC}"
echo "  - ARCH-001: system-architect (ghostty-tmux-architect:0)"
echo "  - OPS-001: devops-engineer (ghostty-devops:0)"
echo ""
echo -e "${BLUE}Next Steps:${NC}"
echo "1. Check each agent's startup: tmux capture-pane -t [session:window] -p | tail -20"
echo "2. Monitor initial task pickup in cache/week1/[ROLE]/daily-reports/"
echo "3. Schedule daily standup at 09:00"
echo "4. Review handoff schedule in /docs/任务清单/第一周/协作计划.md"
echo ""
echo -e "${GREEN}All agents are now working on Week 1 tasks!${NC}"