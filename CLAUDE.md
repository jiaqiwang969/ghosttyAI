# CLAUDE-v0.2.md - Ghostty × tmux Integration Agent Orchestration Handbook

## 🚨 WEEK 3 CRITICAL UPDATE - REAL INTEGRATION PHASE
**Status**: Week 2 prototypes 100% complete in cache/week2/  
**Next Phase**: Real source code integration into tmux/ and ghostty/  
**Resource Reuse**: 60% code from Week 2 directly reusable  

## 📍 WEEK 3 CONSTRUCTION PLAN - START HERE
**All agents MUST read these documents in order**:

### 🎯 Week 3 Essential Documents
1. **总览**: `/第三周-施工方案/第三周施工方案总览.md` - Overall plan and objectives
2. **资源**: `/第三周-施工方案/资源复用清单.md` - What you can reuse from Week 1/2
3. **执行**: `/第三周-施工方案/Agent执行手册.md` - YOUR specific instructions
4. **快速**: `/第三周-施工方案/快速启动检查清单.md` - Quick reference and troubleshooting

### 🗺️ Week 3 Agent Navigation Flowchart
```
WEEK 3 START
    ↓
[1] Read /第三周-施工方案/第三周施工方案总览.md (10 min)
    ↓
[2] Check /第三周-施工方案/资源复用清单.md for your reusable assets
    ↓
[3] Find YOUR role in /第三周-施工方案/Agent执行手册.md
    ↓
[4] Verify prerequisites in /第三周-施工方案/快速启动检查清单.md
    ↓
[5] Copy relevant Week 2 code from cache/week2/[YOUR-MODULE]/
    ↓
[6] Begin real source integration (tmux/ or ghostty/)
    ↓
[7] Test and validate against Week 2 baseline (380k ops/s)
```

### 📊 Week 2 Achievements (Baseline for Week 3)
- **Performance**: 380k ops/s (90% above target)
- **Test Coverage**: 91% with 47 passing tests
- **Core Components**: Event loop (0.8% overhead), SIMD grid ops (10x), Zero-copy FFI (<100ns)
- **Location**: All in cache/week2/ ready for integration

## 📍 LEGACY NAVIGATION (Week 1 Reference)
**Original guidance**: `/docs/README-目录说明.md`
- Week 1 task lists in `/docs/任务清单/第一周/`
- Architecture views in `/docs/architecture-view/`
- Construction blueprint in `/docs/new-architecture-施工图/`

## ⚠️ CRITICAL: Claude Startup in Tmux

**MANDATORY**: Always use `--dangerously-skip-permissions` flag when starting Claude in tmux:
```bash
claude --dangerously-skip-permissions
```
**Without this flag, Claude will NOT start properly in tmux sessions!**

## 🎯 Project Overview
The Ghostty × tmux Integration project aims to compile tmux as a library (libtmuxcore) and embed it directly into Ghostty, replacing VT/TTY output with structured callbacks and event-driven rendering.

## 📁 Source Code Locations

### Critical Source Directories
```
/Users/jqwang/98-ghosttyAI/
├── tmux/                    # tmux source code (C)
│   ├── tty.c               # Terminal output functions to intercept
│   ├── screen-write.c      # Screen writing operations
│   ├── grid.c              # Grid data structures
│   ├── window.c            # Window management
│   ├── session.c           # Session handling
│   └── cmd-*.c             # Command implementations
├── ghostty/                 # Ghostty source code (Zig)
│   ├── src/                # Main source directory
│   ├── build.zig           # Build configuration
│   └── macos/              # macOS specific code
└── cache/                   # EXPERIMENTAL WORKSPACE ONLY
    └── week1/              # TDD development & testing
        └── [ROLE]/         # Your experiments, NOT production code
```

### ⚠️ WEEK 3 CRITICAL: Real Source Integration
- **cache/week2/**: Your VALIDATED prototypes (100% complete, 91% tested)
- **tmux/**: NOW MODIFY - Add backend router to tty.c (line 1234)
- **ghostty/**: NOW MODIFY - Add tmux integration to src/tmux/
- **Migration Path**: cache/week2/ → real source files (60% code reusable)

### 🔄 Week 3 Resource Reuse Map
```
cache/week2/CORE-001/src/event_loop_backend.h (8KB)
    ↓ COPY TO
tmux/ui_backend/event_loop_backend.h

cache/week2/INTG-001/callbacks.zig (31KB)
    ↓ COPY TO
ghostty/src/tmux/callbacks.zig

cache/week2/CORE-002/grid_operations_neon.c (45KB)
    ↓ INTEGRATE INTO
tmux/grid_simd.c
```

## 🏗️ Agent System Architecture

### Updated Agent Hierarchy - All 9 Agents at Same Level

```
┌─────────────────────────────────────────────────────────────┐
│                    ORCHESTRATOR (You)                        │
│                 Strategic oversight & coordination           │
└─────────────────────┬────────────────────────────────────────┘
                      │
          ┌───────────┴────────────┐
          │   PROJECT MANAGER       │
          │  (Execution Authority)  │
          └───────────┬────────────┘
                      │
    ┌─────────────────┴──────────────────────────────────────────────┐
    │                 ALL 9 AGENTS (Same Directory Level)            │
    │         .claude/agents/ directory - all at same level          │
    └─────────────────────────────────────────────────────────────────┘
                      │
    ┌─────────────────┴──────────────────────────────────────────────┐
    │                                                                 │
    │  7 Managed Agents (Report to PM):    2 Independent Agents:     │
    │  ├── c-tmux-specialist.md           ├── system-architect.md    │
    │  ├── libtmux-core-developer.md      └── devops-engineer-ops001.md
    │  ├── zig-ghostty-integration.md                                │
    │  ├── integration-dev.md                                        │
    │  ├── performance-eng.md                                        │
    │  ├── qa-test-lead.md                                          │
    │  └── qa-test-engineer.md                                       │
    └─────────────────────────────────────────────────────────────────┘
```

### Specialized Agent Roles (All 9 Agents - Same Directory Level)

| Agent File | Role | Type | Session Location | Reports To |
|------------|------|------|------------------|------------|
| **c-tmux-specialist.md** | CORE-001 - tmux hooks extraction | Managed | ghostty-core:0 | PM |
| **libtmux-core-developer.md** | CORE-002 - Library development | Managed | ghostty-core:4 | PM |
| **zig-ghostty-integration.md** | INTG-001 - FFI bridge | Managed | ghostty-integration:0 | PM |
| **integration-dev.md** | INTG-002 - Integration support | Managed | ghostty-integration:4 | PM |
| **performance-eng.md** | INTG-003 - Performance optimization | Managed | ghostty-integration:5 | PM |
| **qa-test-lead.md** | QA-001 - Test strategy | Managed | ghostty-quality:0 | PM |
| **qa-test-engineer.md** | QA-002 - Test execution | Managed | ghostty-quality:1 | PM |
| **system-architect.md** | ARCH-001 - Design consultation | Independent | ghostty-tmux-architect:0 | Self |
| **devops-engineer-ops001.md** | OPS-001 - Build & deployment | Independent | ghostty-devops:0 | Self |

**Directory Structure Reality**:
```
.claude/agents/                       # All agents at same level
├── c-tmux-specialist.md             # Same directory level
├── devops-engineer-ops001.md        # Same directory level
├── integration-dev.md               # Same directory level
├── libtmux-core-developer.md        # Same directory level
├── performance-eng.md               # Same directory level
├── qa-test-engineer.md              # Same directory level
├── qa-test-lead.md                  # Same directory level
├── system-architect.md              # Same directory level
├── tmux-project-manager.md          # Same directory level
└── zig-ghostty-integration.md       # Same directory level
```

#### 1. **c-tmux-specialist** 
- **Purpose**: C-based tmux integration and terminal multiplexer functionality
- **Expertise**: PTY/terminal emulation, low-level terminal control
- **Assigned To**: CORE-001, CORE-002 tasks
- **Location**: `ghostty-core` session

#### 2. **libtmux-core-developer**
- **Purpose**: Develop libtmuxcore C library extraction
- **Expertise**: C memory management, native tmux bindings, system programming
- **Assigned To**: T-101 through T-104 (Week 1 core tasks)
- **Location**: `ghostty-core` session

#### 3. **zig-ghostty-integration**
- **Purpose**: Zig-based FFI bridge and Ghostty integration
- **Expertise**: Zig C interop, callback function pointers, error handling
- **Assigned To**: INTG-001, INTG-002 tasks
- **Location**: `ghostty-integration` session

#### 4. **system-architect**
- **Purpose**: Design system architecture and review decisions
- **Expertise**: Microservices, API structures, scalability patterns
- **Role**: Independent consultant (not in task execution)
- **Location**: `ghostty-tmux-architect` session

#### 5. **devops-engineer-ops001**
- **Purpose**: CI/CD pipeline, build automation, deployment
- **Expertise**: GitHub Actions, macOS ARM64 builds, PlantUML
- **Assigned To**: OPS-001 tasks
- **Location**: `ghostty-devops` session

#### 6. **qa-test-lead**
- **Purpose**: Test strategy and quality oversight
- **Expertise**: Test frameworks, coverage analysis, quality gates
- **Role**: Manages QA-001 and QA-002
- **Location**: `ghostty-quality` session

#### 7. **qa-test-engineer**
- **Purpose**: Test implementation and execution
- **Expertise**: Unit tests, integration tests, performance testing
- **Assigned To**: QA-001, QA-002 tasks
- **Location**: `ghostty-quality` session

## 📚 Documentation Quick Reference

### Week 3 Essential Documents for Each Agent

| Agent Type | Week 3 Task Location | Reusable Week 2 Code | Target Source Files |
|------------|---------------------|---------------------|-------------------|
| **c-tmux-specialist** | `/第三周-施工方案/Agent执行手册.md#CORE-001` | `cache/week2/CORE-001/src/event_loop_backend.h` | `tmux/tty.c:1234` |
| **libtmux-core-developer** | `/第三周-施工方案/Agent执行手册.md#CORE-002` | `cache/week2/CORE-002/src/grid_operations_neon.c` | `tmux/Makefile.libtmuxcore` |
| **zig-ghostty-integration** | `/第三周-施工方案/Agent执行手册.md#INTG-001` | `cache/week2/INTG-001/callbacks.zig (31KB)` | `ghostty/src/tmux/` |
| **integration-dev** | `/第三周-施工方案/Agent执行手册.md#INTG-002` | `cache/week2/INTG-002/terminal_integration_poc.zig` | `ghostty/src/terminal/Terminal.zig` |
| **qa-test-engineer** | `/第三周-施工方案/Agent执行手册.md#QA-002` | `cache/week2/TESTS/ (91% coverage)` | Integration testing |

### Week 3 Performance Baseline (MUST MAINTAIN)
```yaml
From Week 2:
  Throughput: 380k ops/s
  Event Loop Overhead: 0.8%
  FFI Latency: <100ns
  Memory per Session: 8.3MB
  Test Coverage: 91%
  
Week 3 Requirements:
  Performance Degradation: <5%
  All Tests Must Pass
  No Memory Leaks
  Backward Compatible
```

### Legacy Week 1 Documents (Reference)
| Agent Type | Primary Task Doc | Source Code to Analyze | Work Directory |
|------------|-----------------|------------------------|----------------|
| **c-tmux-specialist** | `/docs/任务清单/第一周/CORE-001.md` | `tmux/tty.c`, `tmux/screen-write.c` | `cache/week1/CORE-001/` |
| **libtmux-core-developer** | `/docs/任务清单/第一周/CORE-002.md` | `tmux/grid.c`, `tmux/window.c`, `tmux/session.c` | `cache/week1/CORE-002/` |
| **zig-ghostty-integration** | `/docs/任务清单/第一周/INTG-001.md` | `ghostty/src/`, `ghostty/build.zig` | `cache/week1/INTG-001/` |
| **qa-test-engineer** | `/docs/任务清单/第一周/QA-002.md` | Both `tmux/` and `ghostty/` for testing | `cache/week1/QA-002/` |
| **system-architect** | `/docs/任务清单/第一周/ARCH-001.md` | All source for architecture review | Design docs only |
| **devops-engineer-ops001** | `/docs/任务清单/第一周/OPS-001.md` | `Makefile`, build configs | Build scripts |

### Daily Workflow Documents
1. **Morning**: Check `/docs/任务清单/第一周/[YOUR-ROLE].md`
2. **Planning**: Review `/docs/任务清单/第一周/协作计划.md` for handoffs
3. **Status**: Update `/docs/project-manager-view/task-ownership-week1.puml`
4. **EOD**: Submit report to `cache/week1/[ROLE]/daily-reports/`

### Critical Paths
- **Task Assignments**: `/docs/任务清单/第一周/` → Your specific role file
- **Technical Understanding**: `/docs/architecture-view/` → Component you're working on
- **How to Build**: `/docs/new-architecture-施工图/` → Step-by-step implementation
- **Who Owns What**: `/docs/project-manager-view/` → Responsibility matrix

## 📊 Communication Flow

### Hub-and-Spoke Model v2.0

```
                    ORCHESTRATOR
                         │
            ┌────────────┼────────────┐
            │            │            │
       ARCHITECT    PROJECT MGR    DEVOPS
      (Design Only)  (Execution)  (Build Only)
            │            │            │
            │      ┌─────┴─────┐      │
            │      │ TASK TEAMS│      │
            │      └─────┬─────┘      │
            │            │            │
      Consulted    Direct Report  Consulted
       Only           Daily          Only
```

**Critical Rules**:
1. Task teams report ONLY to Project Manager
2. Architect consulted for design decisions only
3. DevOps consulted for build/deployment only
4. No direct engineer-to-engineer communication
5. PM aggregates all status for Orchestrator

## 🔄 Tmux Session Management

### Active Sessions and Agents

```bash
# Core Development Teams
ghostty-core:
  ├── 0: c-tmux-specialist (CORE-001)
  ├── 1: Dev-Server
  ├── 2: Shell
  ├── 3: Project-Manager
  └── 4: libtmux-core-developer (CORE-002)

# Integration Teams
ghostty-integration:
  ├── 0: zig-ghostty-integration (INTG-001)
  ├── 1: Claude-Flow-Server
  ├── 2: Test-Runner
  ├── 3: Claude-QA
  └── 4: Integration-Engineer-2 (INTG-002)

# Quality Assurance
ghostty-quality:
  ├── 0: qa-test-lead
  └── 1: qa-test-engineer

# Independent Consultants
ghostty-tmux-architect:
  └── 0: system-architect

ghostty-devops:
  ├── 0: devops-engineer-ops001
  ├── 1: Build-Pipeline
  ├── 2: Security-Scan
  └── 3: Deploy-Monitor

# Management
tmux-orchestrator:
  ├── 0: Orchestrator (You)
  ├── 1: Status-Monitor
  └── 2: Cross-Team-Comm
```

## 📋 Agent Deployment Protocol

### 🚀 QUICK START FOR NEW AGENTS

**Step 1: Read Documentation Overview**
```bash
cat /Users/jqwang/98-ghosttyAI/docs/README-目录说明.md  # Understand doc structure
```

**Step 2: Find Your Task**
```bash
cat /Users/jqwang/98-ghosttyAI/docs/任务清单/第一周/[YOUR-ROLE].md  # Your tasks
```

**Step 3: Analyze Source Code (READ-ONLY)**
```bash
# For C/tmux specialists - analyze tmux source
cd /Users/jqwang/98-ghosttyAI/tmux/
grep -r "tty_cmd_" .  # Find all tty command functions
cat tty.c | grep "tty_write"  # Understand output mechanism

# For Zig integration - analyze Ghostty source
cd /Users/jqwang/98-ghosttyAI/ghostty/
find src -name "*.zig"  # Explore Zig structure
cat build.zig  # Understand build configuration
```

**Step 4: Set Up Experimental Workspace**
```bash
cd /Users/jqwang/98-ghosttyAI/cache/week1/[YOUR-ROLE]/
mkdir -p tests wip daily-reports handoffs
# This is where you EXPERIMENT and write tests
```

**Step 5: Begin TDD Cycle in Cache**
```bash
# Write test first (in cache, not source!)
vim cache/week1/[YOUR-ROLE]/tests/test_[feature].c

# Then experimental implementation
vim cache/week1/[YOUR-ROLE]/wip/[feature].c

# After PM validation, code moves to proper source location
```

### Starting a Specialized Agent

```bash
# 1. Navigate to correct session/window
tmux select-window -t ghostty-core:0

# 2. Start Claude with REQUIRED permission flag
tmux send-keys -t ghostty-core:0 "claude --dangerously-skip-permissions" Enter
sleep 5  # Wait for Claude to initialize

# ⚠️ IMPORTANT: --dangerously-skip-permissions is REQUIRED in tmux
# Without this flag, Claude will not start properly in tmux sessions

# 3. Deploy agent briefing based on role
tmux send-keys -t ghostty-core:0 "You are the c-tmux-specialist. Your role:
- Focus: C-based tmux integration for CORE-001 tasks
- Expertise: Terminal multiplexer functionality, PTY handling
- Tasks: Extract tty_write hooks, implement backend router
- Method: TDD with tests in cache/week1/CORE-001/tests/
- Report to: Project Manager only
Read your tasks from /docs/任务清单/第一周/CORE-001.md" Enter
```

### Agent-Specific Briefings

#### For c-tmux-specialist:
```
Focus on tmux internals (grid.c, screen.c, tty.c)
Extract and implement UI backend hooks
Maintain C ABI compatibility
```

#### For libtmux-core-developer:
```
Create libtmuxcore library extraction
Implement vtable abstractions
Ensure no external dependencies
```

#### For zig-ghostty-integration:
```
Build FFI bridge between C and Zig
Handle memory safety at boundaries
Implement callback mechanisms
```

#### For qa-test-engineer:
```
Write tests BEFORE implementation
Maintain >80% coverage for new code
Validate all acceptance criteria
```

## 🎯 Task Assignment Matrix

| Agent Type | Session | Window | Week 1 Tasks | Reports To |
|------------|---------|--------|--------------|------------|
| c-tmux-specialist | ghostty-core | 0 | T-101, T-102 | PM |
| libtmux-core-developer | ghostty-core | 4 | T-103, T-104 | PM |
| zig-ghostty-integration | ghostty-integration | 0 | T-201, T-202 | PM |
| qa-test-engineer | ghostty-quality | 1 | T-301, T-302 | qa-test-lead |
| qa-test-lead | ghostty-quality | 0 | Oversight | PM |
| devops-engineer-ops001 | ghostty-devops | 0 | Build tasks | Independent |
| system-architect | ghostty-tmux-architect | 0 | Design review | Independent |

## 🔐 Git Discipline Enhancement

### Agent-Specific Commit Patterns

```bash
# C/tmux agents
git commit -m "[CORE-XXX] Component: Description
- Extracted N hooks from tty.c
- Coverage: XX%
- Tests: passing"

# Zig integration agents
git commit -m "[INTG-XXX] FFI: Description
- Implemented callback for function
- Memory safety verified
- Tests: passing"

# QA agents
git commit -m "[QA-XXX] Tests: Description
- Added N test cases
- Coverage increased to XX%
- All AC verified"
```

## 📂 Cache-Based Workflow (Agent Specific)

### ⚠️ Cache = Experimental Sandbox, NOT Production

```
WORKFLOW: Source Analysis → Cache Experiments → PM Validation → Source Integration
```

### Agent Working Directories

```
/Users/jqwang/98-ghosttyAI/
├── tmux/                       # SOURCE (READ-ONLY for analysis)
│   ├── tty.c                  # Analyze: tty output functions
│   ├── screen-write.c         # Analyze: screen operations
│   └── grid.c                 # Analyze: grid structures
├── ghostty/                    # SOURCE (READ-ONLY for analysis)
│   ├── src/                   # Analyze: Zig implementation
│   └── build.zig              # Analyze: build config
└── cache/week1/                # EXPERIMENTAL WORKSPACE
    ├── CORE-001/              # c-tmux-specialist experiments
    │   ├── tests/             # TDD: Write tests FIRST
    │   ├── wip/               # TDD: Experimental implementations
    │   └── handoffs/          # Ready for PM validation
    ├── CORE-002/              # libtmux-core-developer experiments
    ├── INTG-001/              # zig-ghostty-integration experiments
    └── QA-001/                # qa-test-engineer test suites
```

### Source Code Analysis Commands

```bash
# Analyzing tmux source (CORE teams)
cd /Users/jqwang/98-ghosttyAI/tmux/
ctags -R .                      # Generate tags for navigation
grep -n "tty_cmd_" *.c          # Find tty command functions
grep -n "grid_" grid.c          # Understand grid operations

# Analyzing Ghostty source (INTG teams)
cd /Users/jqwang/98-ghosttyAI/ghostty/
find src -name "*.zig" -exec grep -l "terminal" {} \;
zig fmt --check src/            # Understand code style

# Your experiments go in cache ONLY
cd /Users/jqwang/98-ghosttyAI/cache/week1/[YOUR-ROLE]/
# This is your sandbox for TDD development
```

## 🚀 Week 3 Daily Agent Coordination

### Morning Deployment (09:00) - Week 3 Version

```bash
# Start Claude agents with REQUIRED flag
for window in 0 4; do
  tmux send-keys -t ghostty-core:$window "claude --dangerously-skip-permissions" Enter
  sleep 5
done

# PM deploys Week 3 tasks to agents
for agent in c-tmux-specialist libtmux-core-developer zig-ghostty-integration; do
  tmux send-keys -t [session:window] "WEEK 3 TASK: 
  1. Read /第三周-施工方案/Agent执行手册.md
  2. Copy your Week 2 code from cache/week2/
  3. Begin REAL source integration
  4. Maintain performance baseline (380k ops/s)
  Report progress by 17:00" Enter
done
```

### Week 3 Specific Commands

```bash
# For CORE-001: Copy Week 2 vtable to tmux
cp cache/week2/CORE-001/src/event_loop_backend.h tmux/ui_backend/

# For CORE-002: Build dynamic library
cd tmux && make -f Makefile.libtmuxcore

# For INTG-001: Copy FFI to Ghostty
cp cache/week2/INTG-001/callbacks.zig ghostty/src/tmux/

# For QA-002: Validate integration
cd tests/week3 && ./run_integration_tests.sh
```

### Status Collection (17:00)

```bash
# PM collects status from all agents
for session in ghostty-core ghostty-integration ghostty-quality; do
  tmux capture-pane -t $session:0 -p | grep "STATUS" | tail -5
done
```

## 📈 Week 3 Success Metrics by Agent Type

| Agent | Week 3 Primary Goal | Success Metric | Validation |
|-------|-------------------|---------------|------------|
| c-tmux-specialist | Modify real tty.c | Backend router working | tmux compiles with -DLIBTMUXCORE_BUILD |
| libtmux-core-developer | Build libtmuxcore.so | Dynamic library loads | ldd shows correct symbols |
| zig-ghostty-integration | Integrate FFI to Ghostty | src/tmux/ compiles | zig build succeeds |
| integration-dev | Connect Terminal.zig | tmux commands work | Basic new/split functional |
| qa-test-engineer | Validate integration | 85%+ coverage maintained | No performance regression |
| system-architect | Review integration | Architecture approved | No design violations |

## 🔧 Week 3 Agent-Specific Tools and Commands

### For C/tmux Agents (NOW MODIFYING tmux source)
```bash
# Week 3: MODIFY tmux source
cd /Users/jqwang/98-ghosttyAI/tmux/

# Copy Week 2 prototypes
cp ~/98-ghosttyAI/cache/week2/CORE-001/src/event_loop_backend.h ui_backend/
cp ~/98-ghosttyAI/cache/week2/CORE-002/src/grid_operations_neon.c .

# Modify tty.c (line 1234)
vim tty.c
# Add: #ifdef LIBTMUXCORE_BUILD ... #endif

# Build dynamic library
make -f Makefile.libtmuxcore
nm -D libtmuxcore.so | grep tmc_  # Verify symbols
```

### For Zig Integration Agents (NOW MODIFYING Ghostty)
```bash
# Week 3: CREATE tmux integration
cd /Users/jqwang/98-ghosttyAI/ghostty/

# Create tmux module
mkdir -p src/tmux
cp ~/98-ghosttyAI/cache/week2/INTG-001/callbacks.zig src/tmux/
cp ~/98-ghosttyAI/cache/week2/INTG-001/ffi_safety.zig src/tmux/

# Update build.zig
vim build.zig
# Add: exe.linkSystemLibrary("tmuxcore");

# Build with tmux support
zig build
```

### For QA Agents (Testing REAL Integration)
```bash
# Week 3: Test real binaries
cd /Users/jqwang/98-ghosttyAI/

# Copy Week 2 test suite
cp -r cache/week2/TESTS/ tests/week3/

# Test dynamic library
ldd tmux/libtmuxcore.so
./tests/week3/integration_test

# Performance validation
./tests/week3/benchmark | grep ops/s
# Must be >350k ops/s (Week 2: 380k)
```

## 🎮 Emergency Agent Management

### Agent Not Responding
```bash
# Check agent status
tmux capture-pane -t [session:window] -p | tail -50

# Restart agent if needed
tmux send-keys -t [session:window] C-c  # Interrupt
tmux send-keys -t [session:window] "clear" Enter

# Restart Claude with REQUIRED flag
tmux send-keys -t [session:window] "claude --dangerously-skip-permissions" Enter
sleep 5

# Redeploy agent briefing
tmux send-keys -t [session:window] "[Agent briefing message]" Enter
```

### Agent Task Switch
```bash
# Save current work
tmux send-keys -t [session:window] "git add -A && git commit -m 'WIP: Switching tasks'" Enter

# Assign new task
tmux send-keys -t [session:window] "NEW TASK: [description]" Enter
```

## 📝 Week 3 Agent Performance Review

### Week 3 Key Achievements to Date
1. **Week 1 (85% Complete)**: Design documents, POC code, architecture decisions
2. **Week 2 (100% Complete)**: Prototypes validated, 380k ops/s, 91% test coverage
3. **Week 3 (Starting Now)**: Real source integration - tmux/tty.c + ghostty/src/tmux/

### Critical Week 3 Success Factors
- **60% code reusable** from cache/week2/
- **Performance baseline**: 380k ops/s (max 5% degradation allowed)
- **Test coverage**: Maintain 91% from Week 2
- **Integration path**: cache/week2/ → real source files

### Week 3 Deliverables
1. libtmuxcore.so dynamic library (from static .a)
2. Modified tmux/tty.c with backend router
3. New ghostty/src/tmux/ integration module
4. End-to-end integration tests passing
5. Performance within 5% of baseline

---

**Remember**: Each specialized agent has unique expertise. Deploy the right agent for the right task. The Project Manager coordinates execution while Architect and DevOps remain independent consultants.

**WEEK 3 MOTTO**: "Not reinventing, but integrating. 60% is already done!"