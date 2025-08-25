# CLAUDE-v0.1.md - Ghostty × tmux Integration Agent Orchestration Handbook

## 🎯 Project Overview
The Ghostty × tmux Integration project aims to compile tmux as a library (libtmuxcore) and embed it directly into Ghostty, replacing VT/TTY output with structured callbacks and event-driven rendering.

## 🏗️ Agent System Architecture

### Corrected Agent Hierarchy

```
┌─────────────────────────────────────────────────────────────┐
│                    ORCHESTRATOR (You)                        │
│                 Strategic oversight & coordination           │
└──────────────┬──────────────────────────┬───────────────────┘
               │                          │
    ┌──────────▼────────────┐  ┌─────────▼──────────┐
    │  INDEPENDENT AGENTS   │  │  PROJECT MANAGER   │
    │   (Consultative)      │  │  (Execution Lead)  │
    └───────────────────────┘  └────────────────────┘
               │                          │
    ┌──────────┴──────────┐              │
    │                     │              │
┌───▼─────────┐ ┌────────▼──────┐        │
│  ARCHITECT  │ │    DEVOPS     │        │
│  CONSULTANT │ │   CONSULTANT  │        │
└─────────────┘ └───────────────┘        │
                                          │
              ┌───────────────────────────┴──────────────────────────┐
              │                  TASK TEAMS                          │
              │            (Report to PM Only)                       │
              └───────────────────────────────────────────────────────┘
                                    │
    ┌───────────┬───────────┬──────┴──────┬────────────┬────────────┐
    │           │           │              │            │            │
┌───▼────┐ ┌───▼────┐ ┌────▼────┐ ┌──────▼─────┐ ┌───▼────┐ ┌─────▼──────┐
│ CORE    │ │ INTG   │ │  QA     │ │   SYSTEM   │ │  C/    │ │    ZIG     │
│ TEAMS   │ │ TEAMS  │ │  TEAMS  │ │ ARCHITECT  │ │  TMUX  │ │ INTEGRATION│
└─────────┘ └────────┘ └─────────┘ └────────────┘ └────────┘ └────────────┘
```

### Specialized Agent Roles (Created)

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

### Starting a Specialized Agent

```bash
# 1. Navigate to correct session/window
tmux select-window -t ghostty-core:0

# 2. Start Claude with appropriate agent
tmux send-keys -t ghostty-core:0 "claude --dangerously-skip-permissions" Enter
sleep 5

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

### Agent Working Directories

```
cache/week1/
├── CORE-001/           # c-tmux-specialist workspace
│   ├── tests/          # C unit tests
│   ├── wip/            # tty_write extraction
│   └── handoffs/       # To CORE-002
├── CORE-002/           # libtmux-core-developer workspace
│   ├── tests/          # Library tests
│   ├── wip/            # Router implementation
│   └── handoffs/       # To INTG-001
├── INTG-001/           # zig-ghostty-integration workspace
│   ├── tests/          # FFI tests
│   ├── wip/            # Bridge code
│   └── handoffs/       # To QA
└── QA-001/             # qa-test-engineer workspace
    ├── tests/          # Integration tests
    ├── reports/        # Coverage reports
    └── validation/     # AC verification
```

## 🚀 Daily Agent Coordination

### Morning Deployment (09:00)

```bash
# PM deploys daily tasks to agents
for agent in c-tmux-specialist libtmux-core-developer zig-ghostty-integration; do
  tmux send-keys -t [session:window] "DAILY TASK: Review /docs/任务清单/第一周/
  Focus on today's acceptance criteria
  Write tests first, then implement
  Report progress by 17:00" Enter
done
```

### Status Collection (17:00)

```bash
# PM collects status from all agents
for session in ghostty-core ghostty-integration ghostty-quality; do
  tmux capture-pane -t $session:0 -p | grep "STATUS" | tail -5
done
```

## 📈 Success Metrics by Agent Type

| Agent | Primary Metric | Target | Measurement |
|-------|---------------|--------|-------------|
| c-tmux-specialist | Hook extraction | 100% tty_cmd_* | Count functions |
| libtmux-core-developer | Library compilation | Clean build | No warnings |
| zig-ghostty-integration | FFI safety | Zero unsafe | Zig compiler |
| qa-test-engineer | Test coverage | >80% new | lcov/gcov |
| devops-engineer-ops001 | Build time | <2 min | Time command |
| system-architect | Design approval | 100% reviewed | PM validation |

## 🔧 Agent-Specific Tools and Commands

### For C/tmux Agents
```bash
# Analyze tmux source
grep -r "tty_cmd_" tmux/
ctags -R tmux/
gcc -Wall -Wextra -c test.c
valgrind --leak-check=full ./test
```

### For Zig Integration Agents
```bash
# Zig FFI development
zig build-lib -dynamic
zig test ffi_bridge.zig
zig fmt --check src/
```

### For QA Agents
```bash
# Test execution
make test
gcov *.c
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

## 🎮 Emergency Agent Management

### Agent Not Responding
```bash
# Check agent status
tmux capture-pane -t [session:window] -p | tail -50

# Restart agent if needed
tmux send-keys -t [session:window] C-c  # Interrupt
tmux send-keys -t [session:window] "clear" Enter
# Redeploy agent briefing
```

### Agent Task Switch
```bash
# Save current work
tmux send-keys -t [session:window] "git add -A && git commit -m 'WIP: Switching tasks'" Enter

# Assign new task
tmux send-keys -t [session:window] "NEW TASK: [description]" Enter
```

## 📝 Agent Performance Review

Weekly review for each agent:
1. Task completion rate
2. Test coverage achieved
3. Code quality metrics
4. Communication effectiveness
5. Git discipline adherence

---

**Remember**: Each specialized agent has unique expertise. Deploy the right agent for the right task. The Project Manager coordinates execution while Architect and DevOps remain independent consultants.