# Handoff Document Template

## Handoff Metadata

- **From**: [ROLE-ID]
- **To**: [ROLE-ID]
- **Date**: [DATE]
- **Time**: [TIME]
- **Task**: T-XXX
- **Next Task**: T-YYY

## Deliverables

### Primary Deliverable
- **File**: `[filename]`
- **Location**: `cache/week1/[ROLE]/handoffs/[filename]`
- **Type**: [Source Code/Header/Documentation]
- **Lines of Code**: XXX
- **Language**: [C/Zig/Shell]

### Test Suite
- **Test File**: `test_[component].c`
- **Location**: `cache/week1/[ROLE]/tests/`
- **Tests Written**: XX
- **Tests Passing**: XX/XX
- **Coverage**: XX%

### Supporting Files
1. `file1.h` - Header definitions
2. `file2.md` - Documentation
3. `coverage.html` - Coverage report

## Interface Specification

```c
// Primary interface/API being handed off
struct ui_backend {
    void (*init)(void);
    void (*cell)(int x, int y, char c);
    void (*clear_line)(int y);
    // ... etc
};
```

## Usage Example

```c
// How to use this component
#include "backend_router.h"

// Initialize
struct ui_backend *backend = backend_router_init();

// Use the interface
backend->cell(0, 0, 'A');
backend->clear_line(1);

// Cleanup
backend_router_cleanup(backend);
```

## Test Verification

```bash
# Commands to verify the handoff works
cd cache/week1/[ROLE]/tests/
make test
./test_[component]

# Expected output:
# All tests passing
# Coverage: XX%
```

## Dependencies

### Required Inputs
- [ ] `ui_backend.h` v1.0.0 from cache/shared/interfaces/
- [ ] `config.h` from project root

### External Dependencies
- libevent 2.1+
- ncurses 6.0+

## Performance Characteristics

| Metric | Value | Target | Status |
|--------|-------|--------|--------|
| Function calls/sec | XXXX | >10000 | ✅ |
| Memory usage | XX MB | <55 MB | ✅ |
| Latency | X.X ms | <1 ms | ✅ |

## Known Limitations

1. **Limitation 1**: Description and workaround
2. **TODO**: Items that need future work
3. **FIXME**: Known issues to address

## Integration Notes

### For the Recipient
1. This component expects [specific setup]
2. Initialize before [other component]
3. Error handling assumes [behavior]

### Critical Path Items
- This is on the critical path for T-106
- Blocks integration testing if delayed
- Dependencies must be validated by [date]

## Validation Checklist

### Self-Validation (By Sender)
- [x] All tests passing
- [x] Coverage >80%
- [x] No compiler warnings
- [x] Memory leak free (valgrind)
- [x] Performance benchmarks met
- [x] Documentation complete

### PM Validation Required
- [ ] Test-first methodology verified
- [ ] Coverage standards met
- [ ] Interface stability confirmed
- [ ] Ready for integration

## Migration Instructions

If replacing existing component:
1. Step 1: Backup existing
2. Step 2: Update includes
3. Step 3: Recompile
4. Step 4: Run tests

## Support

**Primary Contact**: [ROLE-ID]
**Backup Contact**: PM
**Escalation**: Architect (for design issues)

## Appendix

### A. Test Output
```
[Paste actual test run output]
```

### B. Coverage Report Summary
```
[Paste coverage summary]
```

### C. Performance Benchmark
```
[Paste benchmark results]
```

---

**Handoff Status**: Ready for PM Validation
**Digital Signature**: [ROLE-ID] / [TIMESTAMP]