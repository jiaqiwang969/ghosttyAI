# Team Structure and Personnel Assignment
## Ghostty × tmux Integration Project

### Team Organization

```
Project Lead (PL-001)
├── Architecture Team (ARCH)
│   └── ARCH-001: Lead Architect (ghostty-tmux-architect)
├── Core Development Team (CORE)
│   ├── CORE-001: Senior C Developer (tmux specialist)
│   ├── CORE-002: C Developer (libtmuxcore)
│   └── CORE-003: Build System Engineer
├── Integration Team (INTG)
│   ├── INTG-001: Senior Zig Developer (FFI specialist)
│   ├── INTG-002: Zig Developer (Ghostty integration)
│   └── INTG-003: Performance Engineer
├── Quality Assurance Team (QA)
│   ├── QA-001: Test Lead
│   ├── QA-002: Test Engineer (Grid operations)
│   └── QA-003: Performance Test Engineer
└── DevOps Team (OPS)
    ├── OPS-001: CI/CD Engineer
    └── OPS-002: Release Engineer
```

### Personnel Assignments by Sprint

#### Week 1: UI Backend Foundation
- **CORE-001**: Lead tty_write extraction (T-101, T-102)
- **CORE-002**: Implement backend router (T-103)
- **ARCH-001**: Design review and validation
- **QA-001**: Test plan creation

#### Week 2: Event Loop & Callbacks
- **CORE-001**: Event loop vtable (T-201)
- **CORE-002**: Grid callbacks (T-202)
- **INTG-001**: Initial FFI design (T-203)
- **QA-002**: Grid operation tests

#### Week 3: Zig FFI Bridge
- **INTG-001**: FFI implementation (T-301)
- **INTG-002**: Ghostty integration (T-302)
- **CORE-001**: C side support
- **QA-002**: Integration tests

#### Week 4: Testing & Performance
- **QA-001**: Full test suite execution
- **QA-003**: Performance benchmarks
- **INTG-003**: Performance optimization
- **CORE-002**: Bug fixes

#### Week 5: Documentation & Release
- **OPS-001**: CI/CD pipeline
- **OPS-002**: Release preparation
- **ARCH-001**: Documentation review
- **All**: Final testing

### Responsibility Matrix (RACI)

| Task Category | Responsible | Accountable | Consulted | Informed |
|--------------|-------------|-------------|-----------|----------|
| Architecture | ARCH-001 | PL-001 | CORE-001, INTG-001 | All |
| tmux Core Changes | CORE-001 | ARCH-001 | CORE-002 | INTG-001 |
| libtmuxcore Library | CORE-002 | CORE-001 | ARCH-001 | QA-001 |
| Zig FFI Bridge | INTG-001 | ARCH-001 | INTG-002 | CORE-001 |
| Ghostty Integration | INTG-002 | INTG-001 | ARCH-001 | QA-002 |
| Testing | QA-001 | PL-001 | QA-002, QA-003 | All |
| Performance | INTG-003 | QA-003 | CORE-001 | ARCH-001 |
| CI/CD | OPS-001 | OPS-002 | QA-001 | All |

### Communication Protocols

1. **Daily Sync**: Each team lead reports to PL-001
2. **Architecture Reviews**: Weekly with ARCH-001
3. **Integration Points**: CORE-001 ↔ INTG-001 direct communication
4. **Test Feedback**: QA-001 → Responsible Developer
5. **Blocker Escalation**: Any → Team Lead → PL-001 (10min SLA)