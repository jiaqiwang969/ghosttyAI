# UI Backend Architecture Design Document
**Author**: ARCH-001 (System Architect)  
**Date**: 2025-08-25  
**Version**: 1.0.0  
**Project**: Ghostty × tmux Integration (libtmuxcore)

## Executive Summary

This document describes the UI Backend abstraction layer design for integrating tmux as a library (libtmuxcore) into Ghostty. The design replaces traditional VT sequence generation with structured callbacks, enabling direct grid updates and 60+ FPS rendering performance.

## 1. Architecture Overview

### 1.1 Design Goals
- **Zero VT overhead**: Eliminate escape sequence generation/parsing
- **ABI stability**: Ensure forward/backward compatibility
- **High performance**: Support 60+ FPS with frame batching
- **Minimal invasiveness**: Concentrate changes in output layer only
- **Thread safety**: Clear concurrency model with no hidden locks

### 1.2 Key Innovation
Instead of generating VT sequences in `tty_write()`, we intercept all 22 `tty_cmd_*` functions and route them to a pluggable backend system that emits structured data directly to the UI.

## 2. Component Architecture

### 2.1 Layer Stack
```
┌─────────────────────────┐
│   tmux Application      │
├─────────────────────────┤
│   Screen/Grid Layer     │
├─────────────────────────┤
│   tty_write() [MODIFIED]│ ← Interception Point
├─────────────────────────┤
│   Backend Router        │ ← New Component
├──────────┬──────────────┤
│ UI Backend│ TTY Backend │ ← Pluggable Backends
├──────────┴──────────────┤
│  Frame Aggregator       │ ← Batching System
├─────────────────────────┤
│   Callback Interface    │ ← To Ghostty
└─────────────────────────┘
```

### 2.2 Data Flow
1. tmux generates screen updates
2. `tty_write()` receives command function and context
3. Backend Router determines target (UI or TTY)
4. UI Backend converts to spans of cells
5. Frame Aggregator batches updates (16.67ms window)
6. Callback delivers frame to Ghostty
7. Ghostty renders via GPU

## 3. Critical Design Decisions

### 3.1 ABI Stability Strategy

Every structure starts with `uint32_t size` field:
```c
typedef struct {
    uint32_t size;  // MUST be first field
    // ... other fields ...
} any_struct_t;
```

**Benefits**:
- Runtime version negotiation
- Safe structure evolution
- Backward compatibility
- Forward compatibility with degraded features

### 3.2 Frame Batching Design

**Why 16.67ms?**
- Aligns with 60 FPS display refresh
- Reduces callback overhead by ~10x
- Enables span merging optimizations
- Maintains low latency (<1 frame)

**Batching Algorithm**:
```c
if (time_since_last_frame >= 16.67ms ||
    pending_spans >= MAX_SPANS ||
    update_is_urgent) {
    emit_frame();
}
```

### 3.3 Command Mapping Table

All 22 `tty_cmd_*` functions are mapped to backend operations:

| TTY Command | Backend Operation | Batchable | Priority |
|-------------|------------------|-----------|----------|
| tty_cmd_cell | ui_cmd_cell | Yes | Normal |
| tty_cmd_cells | ui_cmd_cells | Yes | Normal |
| tty_cmd_clearline | ui_cmd_clearline | Yes | Normal |
| tty_cmd_clearscreen | ui_cmd_clearscreen | No | High |
| tty_cmd_scrollup | ui_cmd_scrollup | Yes | Normal |
| tty_cmd_linefeed | ui_cmd_linefeed | Yes | Normal |
| tty_cmd_insertcharacter | ui_cmd_insertcharacter | Yes | Normal |
| tty_cmd_deletecharacter | ui_cmd_deletecharacter | Yes | Normal |
| tty_cmd_reverseindex | ui_cmd_reverseindex | Yes | Normal |
| tty_cmd_syncstart | ui_cmd_syncstart | No | Urgent |
| ... | ... | ... | ... |

### 3.4 Thread Model

**Single-threaded design**:
- All backend operations called from tmux main thread
- Callbacks invoked synchronously
- No internal locks or thread creation
- Frame aggregation is lockless

**Guarantees**:
- Callbacks never hold locks
- Data copied before callback invocation
- Reentrancy handled (callbacks may trigger new commands)

## 4. Memory Management

### 4.1 Ownership Rules
- **Backend owns**: Internal structures, frame buffers
- **Callback receives**: Read-only view of data
- **Host must copy**: Data needed beyond callback scope
- **Zero-copy design**: Spans point to backend-owned cells

### 4.2 Lifecycle Management
```c
backend = ui_backend_create(UI_BACKEND_GHOSTTY, &caps);
ui_backend_register(tty, backend);
// ... use backend ...
ui_backend_unregister(tty);
ui_backend_destroy(backend);
```

## 5. Performance Optimizations

### 5.1 Span Merging
Adjacent cells with identical attributes are merged:
```c
// Before: 3 separate updates
[H][e][l][l][o][ ][W][o][r][l][d]

// After: 1 merged span
[Hello World]
```

### 5.2 Dirty Rectangle Tracking
Only changed regions are included in frames:
```c
dirty_rect = {
    .min_row = 5, .max_row = 10,
    .min_col = 20, .max_col = 60
}
```

### 5.3 Frame Dropping
Under pressure, non-critical frames are dropped:
```c
if (pending_frames > HIGH_WATERMARK) {
    drop_non_urgent_frames();
    emit_snapshot_frame();  // Full refresh
}
```

## 6. Capability Negotiation

### 6.1 Feature Discovery
```c
ui_capabilities_t caps = {
    .supported = UI_CAP_FRAME_BATCH | 
                UI_CAP_24BIT_COLOR |
                UI_CAP_BORDERS_BY_UI,
    .max_fps = 60,
    .optimal_batch_size = 100
};
```

### 6.2 Graceful Degradation
- Missing features are detected and worked around
- Performance hints guide optimization decisions
- Compatibility mode for older hosts

## 7. Integration Points

### 7.1 tty.c Modification
```c
// Before
void tty_write(void (*cmdfn)(...), struct tty_ctx *ctx) {
    (*cmdfn)(tty, ctx);  // Direct call
}

// After
void tty_write(void (*cmdfn)(...), struct tty_ctx *ctx) {
    if (global_router && global_router->enabled) {
        backend_route_command(global_router, tty, cmdfn, ctx);
    } else {
        (*cmdfn)(tty, ctx);  // Fallback
    }
}
```

### 7.2 Build Configuration
```makefile
# Enable UI backend
CFLAGS += -DENABLE_UI_BACKEND

# Link with backend
libtmuxcore_la_SOURCES += \
    ui_backend.c \
    backend_router.c \
    frame_aggregator.c
```

## 8. Testing Strategy

### 8.1 Unit Tests
- Each backend operation tested independently
- Frame aggregation timing tests
- Span merging correctness
- Memory leak detection (Valgrind)

### 8.2 Integration Tests
- Full tmux commands through backend
- Performance benchmarks (>1M updates/sec)
- Application compatibility (vim, htop, etc.)

### 8.3 Stress Tests
- 1000+ panes
- Rapid scrolling
- Unicode/emoji handling
- Memory pressure scenarios

## 9. Risk Mitigation

| Risk | Mitigation Strategy |
|------|-------------------|
| ABI breakage | Size field + version negotiation |
| Performance regression | Early benchmarking, profiling |
| Memory leaks | ASAN, Valgrind, automated testing |
| Thread safety issues | Single-threaded design |
| Compatibility problems | Maintain TTY backend fallback |

## 10. Implementation Phases

### Phase 1: Foundation (Days 1-2)
- [ ] Extract tty_write hooks
- [ ] Implement backend router
- [ ] Create UI backend stub

### Phase 2: Core Implementation (Days 3-5)
- [ ] Implement all 22 cmd operations
- [ ] Add frame aggregation
- [ ] Integrate span merging

### Phase 3: Optimization (Days 6-7)
- [ ] Performance profiling
- [ ] Memory optimization
- [ ] Latency tuning

### Phase 4: Testing (Days 8-10)
- [ ] Unit test suite
- [ ] Integration tests
- [ ] Application compatibility

## 11. Success Metrics

- **Performance**: 60 FPS stable rendering
- **Latency**: <16.67ms frame delivery
- **Memory**: <100 bytes per cell overhead
- **Compatibility**: 100% tmux command support
- **Reliability**: Zero crashes in 24-hour test

## 12. Open Questions

1. **Shared memory optimization**: Should we use shared memory for large grids?
2. **Compression**: Would span compression benefit high-latency scenarios?
3. **Partial updates**: How to handle overlapping dirty regions efficiently?

## 13. References

- tmux source code: `/Users/jqwang/98-ghosttyAI/tmux/`
- Ghostty source code: `/Users/jqwang/98-ghosttyAI/ghostty/`
- Project specification: `/Users/jqwang/98-ghosttyAI/project_spec.md`
- Architecture documents: `/Users/jqwang/98-ghosttyAI/docs/architecture-view/`

## Appendix A: Command Function Analysis

Based on analysis of tmux/tty.c, here are all 22 tty_cmd_* functions that need backend implementations:

1. **tty_cmd_alignmenttest** - Line 2124
2. **tty_cmd_cell** - Line 2147
3. **tty_cmd_cells** - Line 2188
4. **tty_cmd_clearcharacter** - Line 1811
5. **tty_cmd_clearendofline** - Line 1885
6. **tty_cmd_clearendofscreen** - Line 2055
7. **tty_cmd_clearline** - Line 1876
8. **tty_cmd_clearscreen** - Line 2105
9. **tty_cmd_clearstartofline** - Line 1896
10. **tty_cmd_clearstartofscreen** - Line 2080
11. **tty_cmd_deletecharacter** - Line 1788
12. **tty_cmd_deleteline** - Line 1848
13. **tty_cmd_insertcharacter** - Line 1765
14. **tty_cmd_insertline** - Line 1820
15. **tty_cmd_linefeed** - Line 1939
16. **tty_cmd_rawstring** - Line 2261
17. **tty_cmd_reverseindex** - Line 1905
18. **tty_cmd_scrolldown** - Line 2021
19. **tty_cmd_scrollup** - Line 1982
20. **tty_cmd_setselection** - Line 2233
21. **tty_cmd_sixelimage** - Line 2270
22. **tty_cmd_syncstart** - Line 2321

Each function operates on a `struct tty_ctx` which contains:
- Grid position (ocx, ocy)
- Cell data
- Number of cells/lines affected
- Region bounds
- Screen reference

## Appendix B: Performance Model

**Theoretical throughput**:
- Cell update: ~100ns per cell
- Span creation: ~500ns per span
- Frame emission: ~10μs per frame
- Callback overhead: ~1μs

**Expected performance**:
- 10M cells/second update rate
- 600K frames/second max (limited to 60 FPS)
- <1ms latency for urgent updates
- <16.67ms for batched updates

---

**END OF DESIGN DOCUMENT**