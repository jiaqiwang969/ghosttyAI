# CLAUDE.md - Ghostty × tmux Integration Management Standards
## Project Execution Handbook v3.0 - 2025-01-06

---

## 🎯 Management Philosophy

This document defines HOW work gets done, not WHAT needs to be done. All task definitions are in `/docs/任务清单/`. This is the operational handbook for disciplined, test-driven, cache-based development.

**Core Principle**: Every line of code must be preceded by a test. Every test must verify acceptance criteria. Every commit must advance measurable progress.

---

## 🏗️ Organizational Command Structure

```
Architect (Independent Consultant) - ghostty-tmux-architect
    ├── Consulted for: Design decisions, interface definitions
    └── Not involved in: Daily execution, task management

Project Manager (Execution Authority) - You
    ├── CORE-001 (Senior C Developer - tmux specialist)
    ├── CORE-002 (C Developer - libtmuxcore)
    ├── INTG-001 (Senior Zig Developer - FFI)
    ├── INTG-002 (Zig Developer - Integration)
    ├── INTG-003 (Performance Engineer)
    ├── QA-001 (Test Framework Engineer)
    └── QA-002 (Grid Test Engineer)

DevOps (Independent Consultant) - OPS-001
    ├── Consulted for: Build issues, CI/CD
    └── Not involved in: Feature development
```

**Critical**: Engineers report ONLY to PM. No direct engineer-to-engineer communication except through PM coordination.

---

## 📂 Cache-Based Development Workflow

### Mandatory Directory Structure

```
/Users/jqwang/98-ghosttyAI/cache/
├── week1/                          # Current sprint work
│   ├── CORE-001/
│   │   ├── tests/                  # Test files FIRST
│   │   │   ├── test_tty_hooks.c    # Written before implementation
│   │   │   └── coverage.html       # Coverage reports
│   │   ├── wip/                    # Work in progress
│   │   │   └── tty_write_hooks.c   # Implementation after test
│   │   ├── daily-reports/          # Daily progress
│   │   │   └── 2025-01-06.md       # Structured report
│   │   └── handoffs/               # Ready for next engineer
│   │       └── hooks_inventory.md  # Documented deliverable
│   ├── CORE-002/
│   ├── INTG-001/
│   ├── INTG-002/
│   ├── INTG-003/
│   ├── QA-001/
│   └── QA-002/
├── shared/                          # Cross-team resources
│   ├── interfaces/                 # Shared API definitions
│   │   ├── ui_backend.h            # v1.0.0 (versioned)
│   │   └── versions.json           # Version tracking
│   ├── dependencies/               # Dependency tracking
│   │   └── dependency_graph.json   # Who needs what
│   └── benchmarks/                 # Performance baselines
│       └── baseline_metrics.json   # Reference measurements
└── validated/                      # PM-approved code
    └── ready_for_src/              # Awaiting integration
```

**Rule**: NO code exists outside cache until PM validates. NO exceptions.

---

## 🧪 Test-Driven Development (TDD) Execution Standards

### The Sacred TDD Cycle

Every engineer MUST follow this cycle for EVERY task:

```
1. READ acceptance criteria from /docs/任务清单/
2. WRITE test that verifies AC (must fail)
3. COMMIT test to cache/week1/[ROLE]/tests/
4. IMPLEMENT minimal code to pass test
5. COMMIT implementation to cache/week1/[ROLE]/wip/
6. REFACTOR for quality
7. VERIFY coverage >80% for new code
8. MOVE to cache/week1/[ROLE]/handoffs/ when complete
9. NOTIFY PM for validation
```

### Test File Requirements

```c
// cache/week1/CORE-001/tests/test_tty_hooks.c
// TEST MUST BE WRITTEN FIRST - Before ANY implementation

/*
 * Task: T-101
 * AC-1: All tty_cmd_* functions identified
 * AC-2: Each function has working hook
 * AC-3: Performance overhead <5%
 */

void test_hook_inventory_complete() {
    // This test MUST fail initially
    assert(count_tty_cmd_functions() >= 30);
    assert(count_implemented_hooks() == count_tty_cmd_functions());
}

void test_performance_overhead() {
    clock_t baseline = measure_direct_tty();
    clock_t with_hooks = measure_hooked_tty();
    assert((with_hooks - baseline) / baseline < 0.05);
}
```

### Implementation Only After Test

```c
// cache/week1/CORE-001/wip/tty_write_hooks.c
// This file is created ONLY after test_tty_hooks.c exists and fails

// Implementation to make tests pass
```

### Test Coverage Metrics

| Component | Minimum Coverage | Target Coverage | Measurement Tool |
|-----------|-----------------|-----------------|------------------|
| New Code | 80% | 95% | lcov + genhtml |
| Modified Code | 70% | 85% | gcov |
| Integration Points | 90% | 100% | custom harness |

---

## ⏰ Daily Execution Protocol

### Engineer's Daily Workflow

```
09:00-09:30  Daily Planning
  - Read task from /docs/任务清单/第一周/[ROLE].md
  - Review acceptance criteria
  - Check cache/shared/interfaces/ for dependencies
  - Write test plan in cache/week1/[ROLE]/tests/

09:30-12:00  TDD Morning Sprint
  - Write failing tests (30 min)
  - Implement to pass (90 min)
  - Refactor and optimize (60 min)
  - Commit to cache every 30 minutes

12:00-13:00  Lunch + Code Review
  - Self-review morning work
  - Update cache/week1/[ROLE]/daily-reports/

13:00-16:00  TDD Afternoon Sprint
  - Continue implementation
  - Run coverage analysis
  - Update documentation
  - Prepare handoffs if needed

16:00-17:00  Integration & Handoff
  - Test with shared interfaces
  - Move completed work to handoffs/
  - Update dependency tracking
  - Notify PM of completion

17:00-17:30  Daily Report
  - Write structured report to daily-reports/
  - Update progress percentage
  - Flag any blockers to PM
```

### PM's Daily Workflow

```
08:30-09:00  Pre-Standup Review
  - Check all cache/week1/*/daily-reports/
  - Identify blockers and dependencies
  - Prepare task adjustments

09:00-09:30  Virtual Standup Coordination
  - Review each engineer's status
  - Resolve blockers
  - Coordinate handoffs
  - Update task assignments

09:30-12:00  Quality Gates & Validation
  - Review cache/week1/*/handoffs/
  - Run test suites
  - Check coverage reports
  - Validate against AC

13:00-16:00  Cross-Team Coordination
  - Manage interface versions
  - Resolve dependency conflicts
  - Consult with Architect if needed
  - Update shared/dependencies/

16:00-17:00  Progress Tracking
  - Update project metrics
  - Prepare executive summary
  - Plan next day's priorities

17:00-17:30  End-of-Day Checkpoint
  - Ensure all engineers submitted reports
  - Archive validated code
  - Tag stable checkpoints
```

---

## 📊 Document Management Standards

### Cache Document Types

#### 1. Test Files (ALWAYS FIRST)
```
Location: cache/week1/[ROLE]/tests/
Naming: test_[component]_[feature].c
Required Headers:
  - Task ID
  - Acceptance Criteria covered
  - Expected coverage percentage
```

#### 2. Work-in-Progress Code
```
Location: cache/week1/[ROLE]/wip/
Naming: [component]_[feature].[ext]
Required Headers:
  - Task ID
  - Test file reference
  - Current coverage percentage
```

#### 3. Daily Reports
```markdown
# Daily Report - [ROLE] - [DATE]

## Task Progress
- Task ID: T-XXX
- Status: [0-100]% complete
- Tests Written: X/Y
- Tests Passing: X/Y
- Coverage: XX%

## Completed Today
- [ ] Specific deliverable 1
- [ ] Specific deliverable 2

## Blockers
- [ ] Blocker description (if any)

## Tomorrow's Plan
- [ ] Specific goal 1
- [ ] Specific goal 2

## Handoff Ready
- File: [filename] -> [recipient]
```

#### 4. Handoff Documents
```
Location: cache/week1/[ROLE]/handoffs/
Must Include:
  - Input/Output specification
  - Test results
  - Coverage report
  - Usage examples
  - Known limitations
```

---

## 🔄 Task Execution Protocol

### Task Lifecycle

```
ASSIGNED -> TEST_WRITING -> TEST_FAILING -> IMPLEMENTING -> 
TEST_PASSING -> REFACTORING -> COVERAGE_CHECK -> HANDOFF_READY -> 
PM_REVIEW -> VALIDATED -> INTEGRATED
```

### Task Assignment Format

```
TASK ASSIGNMENT [TIMESTAMP]
=====================================
Task ID: T-XXX
Engineer: [ROLE-ID]
Sprint: Week 1
Priority: P0 (Critical Path)

Inputs Required:
- [ ] File/Interface from cache/shared/
- [ ] Dependencies from other engineers

Outputs Expected:
- [ ] Test file in cache/week1/[ROLE]/tests/
- [ ] Implementation in cache/week1/[ROLE]/wip/
- [ ] Coverage report >80%
- [ ] Handoff document

Acceptance Criteria:
- [ ] AC-1: Specific, measurable criterion
- [ ] AC-2: Specific, measurable criterion
- [ ] AC-3: Performance benchmark

Deadline: [DATE TIME]
```

### Handoff Protocol

```
HANDOFF NOTICE [TIMESTAMP]
=====================================
From: [ROLE-A]
To: [ROLE-B]
Task: T-XXX -> T-YYY

Deliverables:
- File: cache/week1/[ROLE-A]/handoffs/[file]
- Tests: Passing [X/X]
- Coverage: XX%

Interface Version: v1.0.0
Location: cache/shared/interfaces/[interface]

Usage Example:
```c
// Example code
```

Known Limitations:
- Limitation 1
- TODO items

Validation:
- [ ] PM reviewed
- [ ] Tests pass
- [ ] Coverage meets standard
```

---

## 🚦 Quality Gates

### Code Promotion Criteria

Code moves from cache to src ONLY when:

1. **Test Coverage**
   - [ ] New code: >80% coverage
   - [ ] Modified code: >70% coverage
   - [ ] All tests passing

2. **Code Review**
   - [ ] PM has reviewed
   - [ ] No critical issues
   - [ ] Follows project standards

3. **Performance**
   - [ ] Meets benchmark requirements
   - [ ] No memory leaks (valgrind clean)
   - [ ] No race conditions

4. **Documentation**
   - [ ] API documented
   - [ ] Usage examples provided
   - [ ] Handoff complete

### Daily Quality Metrics

```json
{
  "date": "2025-01-06",
  "metrics": {
    "tests_written": 45,
    "tests_passing": 42,
    "coverage_average": 83.5,
    "code_lines": 890,
    "test_lines": 445,
    "test_ratio": 0.5,
    "blockers_resolved": 3,
    "handoffs_completed": 2
  }
}
```

---

## 🔐 Git Discipline

### Commit Rules for Cache

```bash
# Every 30 minutes - NO EXCEPTIONS
cd /Users/jqwang/98-ghosttyAI/cache
git add -A
git commit -m "[ROLE] [T-XXX] Progress: [specific description]
- Tests: X/Y passing
- Coverage: XX%
- Status: [TEST_WRITING|IMPLEMENTING|REFACTORING]"

# Example:
git commit -m "[CORE-001] [T-101] Progress: Implemented 5 tty hooks
- Tests: 5/8 passing  
- Coverage: 75%
- Status: IMPLEMENTING"
```

### Branch Strategy

```
main (protected)
  └── week1 (current sprint)
      ├── week1-CORE-001-T101
      ├── week1-CORE-002-T103
      └── week1-staging (integration)
```

---

## 📈 Performance Standards

### Execution Metrics

| Metric | Standard | Measurement |
|--------|----------|-------------|
| Test-first compliance | 100% | No code without test |
| 30-minute commit | >95% | Git log analysis |
| Daily report submission | 100% | 17:30 deadline |
| Test coverage new code | >80% | lcov reports |
| Handoff documentation | 100% | Complete before transfer |
| Blocker resolution | <30 min | Time to unblock |

### TDD Metrics

| Phase | Time Allocation | Output |
|-------|-----------------|--------|
| Test Writing | 20% | Failing tests |
| Implementation | 40% | Passing tests |
| Refactoring | 25% | Quality code |
| Documentation | 15% | Handoff ready |

---

## 🚨 Escalation Matrix

| Issue Type | Response Time | Escalation Path |
|------------|---------------|-----------------|
| Test Failure | Immediate | Fix or rollback |
| Coverage Drop | 30 min | PM review |
| Blocker | 10 min | PM → Architect |
| Performance Regression | 1 hour | PM → INTG-003 |
| Build Failure | 30 min | PM → OPS-001 |
| Interface Conflict | 1 hour | PM → Architect |

---

## 📐 Success Metrics

### Sprint Success Criteria

- [ ] 100% tasks have tests written FIRST
- [ ] >80% average test coverage
- [ ] 100% daily reports submitted
- [ ] >95% 30-minute commits
- [ ] 0 code in src/ without PM validation
- [ ] 100% handoffs documented
- [ ] All AC verified by tests

### Individual Success Criteria

- [ ] Test-to-code ratio >0.5
- [ ] Zero untested code commits
- [ ] Daily cache updates
- [ ] Clear handoff documentation
- [ ] No direct engineer communication
- [ ] All work in cache structure

---

## 🎯 Critical Success Factors

1. **Test First, Always**: Not a single line of implementation before test
2. **Cache Discipline**: All work happens in cache, promotion only after validation
3. **30-Minute Rule**: Commit or lose work
4. **PM Gateway**: All coordination through PM
5. **Measurement**: If it's not measured, it's not managed
6. **Documentation**: Undocumented work doesn't exist

---

## 📝 Enforcement

**This is not optional**. Engineers who bypass TDD or cache workflow will have their work rejected. Quality is not negotiable. Process is not flexible. Success depends on discipline.

**Remember**: We are building critical infrastructure. Every shortcut today becomes tomorrow's technical debt. Follow the process, trust the process, succeed with the process.

---

**Document Version**: 3.0  
**Authority**: Project Manager  
**Enforcement**: Mandatory  
**Review Cycle**: Daily  
**Last Updated**: 2025-01-06