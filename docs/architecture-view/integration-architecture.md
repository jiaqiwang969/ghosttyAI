# Ghostty × tmux Integration Architecture

## Executive Summary

This document defines the complete architecture for embedding tmux as libtmuxcore into Ghostty, replacing traditional VT/TTY output with structured grid updates and event callbacks. The integration maintains tmux's core semantics while leveraging Ghostty's GPU-accelerated rendering pipeline.

## 1. Current Architecture Analysis

### 1.1 tmux Architecture

tmux follows a client-server architecture with the following key components:

#### Core Components
- **Server (server.c)**: Central process managing sessions, windows, and panes
- **Session/Window/Pane**: Hierarchical organization of terminal instances
- **Grid System (grid.c)**: Cell-based screen buffer with history
- **Screen Writer (screen-write.c)**: Terminal state management and command processing
- **TTY Layer (tty.c)**: Terminal output via escape sequences
- **Event Loop**: libevent-based async I/O handling

#### Data Flow
1. Input → Input Parser → Commands → Screen Writer
2. Screen Writer → tty_write() → tty_cmd_* functions
3. tty_cmd_* → Terminal escape sequences → Physical terminal

### 1.2 Ghostty Architecture

Ghostty is a modern terminal emulator built in Zig with:

#### Core Components
- **Terminal (src/terminal/)**: VT parser and state machine
- **Renderer**: GPU-accelerated rendering (Metal/OpenGL/WebGL)
- **Surface**: Platform-specific window management
- **Font System**: Advanced text shaping and rendering

#### Key Strengths
- Zero-copy rendering pipeline
- GPU-accelerated compositing
- Native platform integration
- Modern codebase in Zig

## 2. Integration Design

### 2.1 Architecture Overview

The integration replaces tmux's TTY output layer with a structured callback system that directly updates Ghostty's terminal grid, eliminating VT sequence parsing overhead.

### 2.2 Key Design Decisions

1. **UI Backend Abstraction**: Replace tty_write() with pluggable backends
2. **Direct Grid Updates**: Bypass VT sequences for structured data transfer
3. **Event Loop Integration**: Delegate to host's event loop via vtable
4. **Minimal Invasiveness**: Preserve tmux core logic and command system

### 2.3 Component Boundaries

- **libtmuxcore**: tmux server, sessions, windows, panes, grid management
- **Ghostty**: Rendering, input handling, window management, GPU pipeline
- **Bridge Layer**: C ABI, callbacks, event loop integration

## 3. Detailed Component Design

### 3.1 libtmuxcore Components

#### 3.1.1 UI Backend System
```c
// ui_backend.h
typedef struct {
    void (*on_grid_update)(const tmc_grid_update* update);
    void (*on_layout_change)(const tmc_layout_update* layout);
    void (*on_cursor_move)(uint32_t pane_id, uint32_t x, uint32_t y);
    void (*on_copy_mode)(const tmc_copy_mode_state* state);
} ui_backend_vtable;
```

#### 3.1.2 Modified tty_write Chain
- Intercept at tty_write() in screen-write.c
- Route tty_cmd_* functions to UI backend
- Aggregate cell updates into spans for efficiency

#### 3.1.3 Event Loop Abstraction
```c
typedef struct {
    int (*add_fd)(int fd, int events, callback, user_data);
    int (*add_timer)(uint64_t ms, callback, user_data);
    void (*post)(void (*fn)(void*), void* data);
} tmc_loop_vtable;
```

### 3.2 Ghostty Integration Layer

#### 3.2.1 FFI Bridge (Zig)
```zig
// ghostty_tmux_bridge.zig
pub const TmuxBridge = struct {
    server: *tmc.Server,
    clients: std.ArrayList(*tmc.Client),
    pane_map: std.AutoHashMap(u32, *Terminal),
    
    pub fn onGridUpdate(update: *const tmc.GridUpdate) void {
        // Map tmux grid updates to Ghostty terminal buffer
    }
};
```

#### 3.2.2 Grid Synchronization
- Map tmux cells to Ghostty's Terminal.Cell format
- Handle Unicode, attributes, colors
- Manage dirty rectangles for GPU rendering

### 3.3 Data Flow Architecture

1. **Input Path**: Ghostty → tmc_send_keys() → tmux input processing
2. **Output Path**: tmux screen_write → UI backend → Ghostty grid → GPU
3. **Layout Path**: tmux layout changes → callback → Ghostty split view

## 4. Implementation Roadmap

### Phase 1: Foundation (Week 1)
- [ ] Create ui_backend.h interface
- [ ] Implement backend routing in tty.c
- [ ] Basic grid span aggregation
- [ ] Minimal Ghostty FFI bridge

### Phase 2: Core Integration (Week 2)
- [ ] Event loop vtable implementation
- [ ] Complete grid update callbacks
- [ ] Layout tree synchronization
- [ ] Input routing (keys, mouse)

### Phase 3: Advanced Features (Week 3)
- [ ] Copy mode integration
- [ ] Session management
- [ ] Command execution API
- [ ] Performance optimization

### Phase 4: Polish (Week 4)
- [ ] Configuration synchronization
- [ ] Error handling and recovery
- [ ] Test suite
- [ ] Documentation

## 5. Performance Considerations

### 5.1 Optimization Strategies
- Batch grid updates per frame (60-120Hz)
- Coalesce adjacent cell updates into spans
- Zero-copy where possible
- Minimize callback overhead

### 5.2 Memory Management
- Shared ownership of grid data
- Ring buffer for history
- Lazy allocation for sparse grids

## 6. Testing Strategy

### 6.1 Unit Tests
- Grid diff algorithm
- Unicode handling
- Attribute merging
- Layout calculations

### 6.2 Integration Tests
- vim/neovim compatibility
- tmux command compliance
- Performance benchmarks
- Stress testing

## 7. Risk Mitigation

### 7.1 Technical Risks
- **Risk**: libevent integration complexity
  - **Mitigation**: Abstract via loop vtable
  
- **Risk**: Performance regression
  - **Mitigation**: Benchmark-driven development

- **Risk**: tmux compatibility
  - **Mitigation**: Maintain parallel TTY backend

### 7.2 Project Risks
- **Risk**: Scope creep
  - **Mitigation**: Strict phase boundaries
  
- **Risk**: Breaking changes
  - **Mitigation**: Versioned ABI from day one

## 8. Success Criteria

- [ ] All tmux commands work as expected
- [ ] Performance equal or better than standalone tmux
- [ ] Zero visible rendering artifacts
- [ ] Maintains 60+ FPS during heavy output
- [ ] Passes tmux test suite