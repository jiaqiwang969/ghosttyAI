# Detailed Task Assignment Matrix
## Ghostty × tmux Integration Project - Version 0.1

### Engineer Task Assignments

#### CORE-001: Senior C Developer (tmux Specialist)
**Total Workload: 10 days**

| Task ID | Task Name | Duration | Week | Dependencies | Deliverables |
|---------|-----------|----------|------|--------------|--------------|
| T-101 | Extract tty_write hooks | 2d | W1 | None | ui_backend.h interface |
| T-201 | Event loop vtable | 3d | W2 | T-103 | loop_vtable.c |
| T-203 | Layout callbacks | 2d | W2 | T-103 | layout integration |
| - | Code reviews | 3d | W1-W5 | Ongoing | Review feedback |

**Key Responsibilities:**
- Lead tmux core refactoring
- Maintain backward compatibility
- Design event loop abstraction
- Mentor CORE-002

---

#### CORE-002: C Developer (libtmuxcore)
**Total Workload: 10 days**

| Task ID | Task Name | Duration | Week | Dependencies | Deliverables |
|---------|-----------|----------|------|--------------|--------------|
| T-103 | Backend router | 3d | W1 | T-101, T-102 | backend_router.c |
| T-202 | Grid callbacks | 2d | W2 | T-103 | Grid callback impl |
| T-404 | Bug fixes | 2d | W4 | T-401 | Patches |
| - | Integration support | 3d | W3-W5 | Ongoing | Support |

**Key Responsibilities:**
- Implement routing layer
- Grid operation callbacks
- Bug fixing and maintenance
- Support integration team

---

#### ARCH-001: Lead Architect
**Total Workload: 8 days**

| Task ID | Task Name | Duration | Week | Dependencies | Deliverables |
|---------|-----------|----------|------|--------------|--------------|
| T-102 | UI backend design | 1d | W1 | None | Design spec |
| - | Architecture reviews | 4d | W1-W4 | Ongoing | Approvals |
| T-502 | Documentation | 2d | W5 | All tasks | Docs |
| - | Final approval | 1d | W5 | T-504 | Sign-off |

**Key Responsibilities:**
- Architectural decisions
- Design reviews and approvals
- Documentation oversight
- Technical risk management

---

#### INTG-001: Senior Zig Developer (FFI Specialist)
**Total Workload: 10 days**

| Task ID | Task Name | Duration | Week | Dependencies | Deliverables |
|---------|-----------|----------|------|--------------|--------------|
| T-104 | Ghostty backend stub | 2d | W1 | T-102 | backend_ghostty.c |
| T-301 | FFI bindings | 3d | W3 | T-201 | libtmuxcore.zig |
| T-303 | Memory safety layer | 2d | W3 | T-301 | memory_safety.zig |
| - | C side support | 3d | W2-W4 | Ongoing | Integration |

**Key Responsibilities:**
- FFI bridge implementation
- Memory safety at boundaries
- C-Zig interop expert
- Performance optimization

---

#### INTG-002: Zig Developer (Ghostty Integration)
**Total Workload: 9 days**

| Task ID | Task Name | Duration | Week | Dependencies | Deliverables |
|---------|-----------|----------|------|--------------|--------------|
| T-204 | Copy-mode callbacks | 2d | W2 | T-202 | Copy-mode impl |
| T-302 | Ghostty integration | 3d | W3 | T-301 | tmux_bridge.zig |
| T-304 | Error handling | 1d | W3 | T-302 | error_handling.zig |
| - | Debug support | 3d | W4-W5 | Ongoing | Fixes |

**Key Responsibilities:**
- Ghostty-side integration
- Terminal buffer connection
- Error propagation
- UI event handling

---

#### INTG-003: Performance Engineer
**Total Workload: 8 days**

| Task ID | Task Name | Duration | Week | Dependencies | Deliverables |
|---------|-----------|----------|------|--------------|--------------|
| - | Performance baseline | 2d | W1-W2 | None | Baseline metrics |
| - | Profile callbacks | 3d | W3 | T-301 | Profile report |
| T-403 | Optimization | 3d | W4 | T-402 | Optimized code |

**Key Responsibilities:**
- Performance profiling
- Optimization implementation
- Callback frequency control
- Memory usage optimization

---

#### QA-001: Test Lead
**Total Workload: 9 days**

| Task ID | Task Name | Duration | Week | Dependencies | Deliverables |
|---------|-----------|----------|------|--------------|--------------|
| - | Test plan creation | 2d | W1 | None | Test plan |
| T-106 | Week 1 integration | 1d | W1 | All W1 | Integration report |
| T-401 | Integration tests | 3d | W4 | All W3 | Test suite |
| T-504 | Final validation | 2d | W5 | T-503 | Final report |
| - | Test coordination | 1d | W2-W3 | Ongoing | Coordination |

**Key Responsibilities:**
- Test strategy and planning
- Integration test execution
- Quality gate enforcement
- Release validation

---

#### QA-002: Test Engineer (Grid Operations)
**Total Workload: 8 days**

| Task ID | Task Name | Duration | Week | Dependencies | Deliverables |
|---------|-----------|----------|------|--------------|--------------|
| T-105 | Grid operation tests | 2d | W1 | T-103 | test_grid_ops.c |
| - | Callback testing | 2d | W2 | T-202 | Callback tests |
| - | FFI testing | 2d | W3 | T-301 | FFI tests |
| - | Regression testing | 2d | W4-W5 | Ongoing | Regression suite |

**Key Responsibilities:**
- Component testing
- Grid operation validation
- Unicode/emoji testing
- Regression prevention

---

#### QA-003: Performance Test Engineer
**Total Workload: 7 days**

| Task ID | Task Name | Duration | Week | Dependencies | Deliverables |
|---------|-----------|----------|------|--------------|--------------|
| - | Performance baseline | 1d | W1 | None | Baseline |
| - | Benchmark setup | 2d | W2 | None | Benchmark suite |
| - | Profiling | 2d | W3 | T-301 | Profile data |
| T-402 | Performance benchmarks | 2d | W4 | T-401 | Benchmark report |

**Key Responsibilities:**
- Performance benchmarking
- Profiling and analysis
- Metric collection
- Performance validation

---

#### OPS-001: CI/CD Engineer
**Total Workload: 6 days**

| Task ID | Task Name | Duration | Week | Dependencies | Deliverables |
|---------|-----------|----------|------|--------------|--------------|
| - | Build system setup | 2d | W1 | None | Build config |
| - | CI pipeline | 2d | W2-W3 | None | CI setup |
| T-501 | CI/CD pipeline | 2d | W5 | All tests | Pipeline |

**Key Responsibilities:**
- Build automation
- CI/CD pipeline
- Test automation
- Deployment scripts

---

#### OPS-002: Release Engineer
**Total Workload: 5 days**

| Task ID | Task Name | Duration | Week | Dependencies | Deliverables |
|---------|-----------|----------|------|--------------|--------------|
| - | Version control | 1d | W1 | None | Git setup |
| - | Build artifacts | 3d | W2-W4 | Ongoing | Artifacts |
| T-503 | Release package | 1d | W5 | T-501, T-502 | Release |

**Key Responsibilities:**
- Release packaging
- Version management
- Distribution setup
- Release notes

---

### Critical Path Ownership

**Critical Path:** T-101 → T-103 → T-201 → T-301 → T-401 → T-504

| Segment | Owner | Risk Mitigation |
|---------|-------|-----------------|
| T-101 → T-103 | CORE-001 → CORE-002 | Daily sync, pair programming |
| T-103 → T-201 | CORE-002 → CORE-001 | Handoff meeting, clear docs |
| T-201 → T-301 | CORE-001 → INTG-001 | C-Zig boundary review |
| T-301 → T-401 | INTG-001 → QA-001 | Early test involvement |
| T-401 → T-504 | QA-001 → QA-001 | Continuous validation |

### Escalation Matrix

| Issue Type | First Contact | Escalation (10min) | Final Escalation |
|------------|---------------|-------------------|------------------|
| Technical Block | Task Owner | Team Lead | ARCH-001 |
| Integration Issue | Both Owners | ARCH-001 | PL-001 |
| Performance Issue | INTG-003 | QA-003 | ARCH-001 |
| Test Failure | QA-002 | QA-001 | Task Owner |
| Build Issue | OPS-001 | CORE-002 | ARCH-001 |

### Communication Schedule

| Meeting | Frequency | Participants | Purpose |
|---------|-----------|--------------|---------|
| Daily Standup | Daily 9am | All active | Status update |
| Architecture Review | Mon/Thu | ARCH-001, Leads | Design decisions |
| Integration Sync | Tue/Fri | CORE, INTG teams | Handoffs |
| Test Review | Wed | QA team, Owners | Test results |
| Sprint Review | Friday | All | Week completion |