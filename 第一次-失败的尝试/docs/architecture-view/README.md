# Ghostty × tmux Integration Architecture Summary

## Project Vision
Embed tmux directly into Ghostty as libtmuxcore, eliminating VT sequence overhead and enabling native GPU-accelerated rendering of tmux sessions.

## Architecture Documents

### 1. Core Architecture
- **[Integration Architecture](./integration-architecture.md)**: Complete architectural overview and design decisions
- **[tty_write Interception](./tty-write-interception.md)**: Detailed technical specification for the core modification point
- **[Implementation Roadmap](./implementation-roadmap.md)**: 20-day step-by-step implementation plan

### 2. PlantUML Diagrams

#### System Architecture
- **[High-Level Integration](./high-level-integration.puml)**: Bird's-eye view of the complete system
- **[Component Interaction](./component-interaction.puml)**: Detailed component relationships
- **[Deployment Diagram](./deployment-diagram.puml)**: Runtime architecture and process layout

#### Data and Control Flow
- **[Data Flow](./data-flow.puml)**: Input/output data paths through the system
- **[Event Loop Integration](./event-loop-integration.puml)**: How tmux events integrate with Ghostty
- **[Callback Sequences](./callback-sequences.puml)**: Detailed callback interaction sequences

#### Structural Design
- **[Class Diagram](./class-diagram.puml)**: Core classes and structures
- **[Session Lifecycle](./session-lifecycle.puml)**: Session state management

## Key Design Decisions

### 1. UI Backend Abstraction
Replace tmux's tty_write() with a pluggable backend system that routes to either traditional TTY or structured callbacks.

### 2. Direct Grid Updates
Bypass VT sequence generation/parsing entirely, transferring grid updates as structured data (spans of cells).

### 3. Event Loop Delegation
Use vtable abstraction to delegate tmux's libevent calls to Ghostty's native event loop.

### 4. Minimal Invasiveness
Preserve tmux's core logic, command system, and data structures. Changes concentrated in output layer only.

## Implementation Strategy

### Phase 1: Foundation (Days 1-5)
- UI backend interface creation
- Backend router implementation
- Grid aggregation system
- Initial FFI bridge

### Phase 2: Core Integration (Days 6-10)
- Event loop abstraction
- Callback system implementation
- Ghostty FFI layer
- Grid synchronization
- Input routing

### Phase 3: Advanced Features (Days 11-15)
- Layout management
- Copy mode integration
- Session management
- Command execution API
- Performance optimization

### Phase 4: Polish (Days 16-20)
- Configuration system
- Error handling
- Test suite
- Documentation
- Final integration

## Critical Integration Points

### 1. tty_write() Interception
```c
// Primary modification point in tty.c
void tty_write(void (*cmdfn)(struct tty *, const struct tty_ctx *),
              struct tty_ctx *ctx) {
    if (ctx->tty->backend_mode == UI_BACKEND) {
        ui_backend_write(ctx->tty->backend, cmdfn, ctx);
        return;
    }
    // Original TTY path
    cmdfn(ctx->tty, ctx);
}
```

### 2. Grid Update Callback
```c
// Structured grid updates instead of escape sequences
typedef struct {
    uint32_t pane_id;
    uint32_t span_count;
    const tmc_span* spans;  // Contiguous cells with same attributes
} tmc_grid_update;
```

### 3. Event Loop Integration
```c
// Delegate to host event loop
typedef struct {
    int (*add_fd)(int fd, int events, callback, user_data);
    int (*add_timer)(uint64_t ms, callback, user_data);
    void (*post)(void (*fn)(void*), void* data);
} tmc_loop_vtable;
```

## Performance Optimizations

### 1. Span Aggregation
Combine adjacent cells with identical attributes into single spans, reducing callback overhead.

### 2. Frame-based Batching
Aggregate updates for 16.67ms (60 FPS) before dispatching callbacks.

### 3. Dirty Rectangle Tracking
Only update modified screen regions, minimizing GPU work.

### 4. Zero-copy Design
Where possible, share memory between tmux and Ghostty rather than copying.

## Testing Strategy

### Unit Tests
- Grid diff algorithm
- Cell format conversion
- Attribute mapping
- Unicode handling

### Integration Tests
- vim/neovim compatibility
- tmux command compliance
- Performance benchmarks
- Stress testing

### Acceptance Criteria
- All tmux commands functional
- 60+ FPS rendering maintained
- < 5ms input latency
- Zero memory leaks

## Risk Mitigation

### Technical Risks
1. **Event loop complexity**: Mitigated by vtable abstraction
2. **Performance regression**: Mitigated by profiling and optimization
3. **Thread safety**: Mitigated by careful synchronization design

### Project Risks
1. **Scope creep**: Mitigated by phased implementation
2. **Breaking changes**: Mitigated by maintaining parallel TTY backend
3. **Integration complexity**: Mitigated by incremental development

## Success Metrics

### Performance
- Grid updates: < 1ms per frame
- Input latency: < 5ms
- Memory usage: < 100MB base
- CPU usage: < 5% idle

### Functionality
- 100% tmux command compatibility
- All keybindings functional
- Config file compatibility
- Session persistence

### Quality
- Test coverage > 80%
- Valgrind clean (no memory leaks)
- No Ghostty feature regression
- Smooth visual experience

## Next Steps

1. **Review Architecture**: Validate design with tmux and Ghostty maintainers
2. **Prototype Backend**: Implement minimal UI backend to validate approach
3. **Performance Testing**: Benchmark callback overhead early
4. **Incremental Integration**: Start with basic rendering, add features gradually

## Viewing the Diagrams

To view the PlantUML diagrams:

1. **Online**: Use [PlantUML Online Server](http://www.plantuml.com/plantuml/uml/)
2. **VS Code**: Install PlantUML extension
3. **Command Line**: 
   ```bash
   plantuml high-level-integration.puml
   plantuml component-interaction.puml
   # etc...
   ```

## Repository Structure

```
/Users/jqwang/98-ghosttyAI/
├── docs/
│   └── architecture/           # This documentation
│       ├── *.md                # Architecture documents
│       └── *.puml              # PlantUML diagrams
├── example/
│   ├── libtmuxcore.h           # C API header
│   ├── ghostty_tmx_bridge.zig  # Zig FFI example
│   └── integration_plan.md     # Original plan
├── tmux/                       # tmux source code
└── ghostty/                    # Ghostty source code
```

## Contact and Collaboration

This architecture is designed to be clear enough for any engineer to implement without ambiguity. Key areas for collaboration:

1. **tmux maintainers**: Validate UI backend approach
2. **Ghostty team**: Confirm FFI bridge design
3. **Performance experts**: Review optimization strategies
4. **Security team**: Audit memory management across FFI

## Conclusion

This architecture provides a clear path to embedding tmux directly into Ghostty, eliminating VT sequence overhead while maintaining full compatibility. The modular design allows for incremental implementation and testing, reducing risk while delivering significant performance improvements.