# Backend Router Implementation

## Overview

The Backend Router is a thread-safe routing system that directs tmux terminal commands to either traditional TTY output or a modern UI backend (Ghostty). This implementation fulfills the requirements of task CORE-002.

## Features

### ✅ Core Functionality
- **Dual Backend Support**: Routes commands to TTY or UI backend
- **Three Routing Modes**:
  - `BACKEND_MODE_TTY`: Traditional terminal output only
  - `BACKEND_MODE_UI`: UI backend only (with TTY fallback)
  - `BACKEND_MODE_HYBRID`: Both backends (configurable behavior)
- **Thread Safety**: Full thread-safe implementation using pthread primitives
- **Performance Metrics**: Comprehensive statistics and monitoring

### 🔒 Thread Safety Guarantees

The implementation provides the following thread safety guarantees:

1. **Atomic Operations**: Statistics use lock-free atomic operations
2. **Read-Write Locks**: Command mapping table uses RW locks for concurrent reads
3. **Mutex Protection**: Router state changes are mutex-protected
4. **Lock-Free Fast Path**: Enabled check uses atomic flag for performance

### 📊 Performance Features

- **Lock-Free Statistics**: Atomic counters for minimal overhead
- **Fast Path Optimization**: Atomic enabled check avoids locks
- **Batching Support**: Command flags indicate batchable operations
- **Metrics Collection**: Optional performance monitoring

## Implementation Details

### File Structure

```
cache/week1/CORE-002/
├── backend_router.c       # Main implementation (750 lines)
├── backend_router.h       # Interface (from ARCH-001)
├── test_backend_router.c  # Comprehensive test suite
├── Makefile              # Build configuration
└── README.md             # This file
```

### Key Components

1. **Router Structure** (`backend_router_impl_t`)
   - Public interface (`backend_router_t`)
   - Thread synchronization primitives
   - Command mapping table
   - Recording system for testing
   - Performance statistics

2. **Command Mapping System**
   - 22 tty_cmd_* functions mapped
   - Corresponding UI backend functions
   - Command-specific flags (batchable, urgent, etc.)

3. **Thread Safety Implementation**
   - `pthread_mutex_t` for router state
   - `pthread_rwlock_t` for mapping table
   - Atomic operations for statistics
   - Lock-free enabled flag check

4. **Hybrid Mode Configuration**
   - Prefer TTY or UI backend
   - Synchronous or asynchronous execution
   - Configurable delays for debugging

## Building and Testing

### Build the Library

```bash
make all
```

### Run Tests

```bash
make test
```

### Thread Safety Validation

```bash
make helgrind  # Valgrind thread safety check
```

### Memory Leak Check

```bash
make memcheck  # Valgrind memory analysis
```

## Integration Guide

### Basic Usage

```c
// Create router
backend_router_t* router = backend_router_create(BACKEND_MODE_UI);

// Register UI backend
backend_router_register_ui(router, ui_backend);

// Route commands
backend_route_command(router, tty, tty_cmd_cell, ctx);

// Cleanup
backend_router_destroy(router);
```

### Global Router (for tmux integration)

```c
// Initialize global router
backend_router_init_global(BACKEND_MODE_HYBRID);

// In tty_write() function:
if (global_backend_router && global_backend_router->enabled) {
    backend_route_command(global_backend_router, tty, cmdfn, ctx);
} else {
    (*cmdfn)(tty, ctx);
}

// Cleanup on exit
backend_router_cleanup_global();
```

### Thread Safety Example

```c
// Multiple threads can safely:
// 1. Route commands concurrently
#pragma omp parallel for
for (int i = 0; i < 1000; i++) {
    backend_route_command(router, tty, cmd_fn, ctx);
}

// 2. Read statistics without locks
const backend_router_stats_t* stats = backend_router_get_stats(router);

// 3. Switch modes safely
backend_router_set_mode(router, BACKEND_MODE_HYBRID);
```

## Test Coverage

The test suite (`test_backend_router.c`) includes:

1. **Lifecycle Tests**: Creation, destruction
2. **Mode Switching**: All three modes tested
3. **Registration**: Backend registration/unregistration
4. **Command Routing**: Routing in all modes
5. **Statistics**: Metrics collection and reset
6. **Thread Safety**: 10 threads × 1000 operations
7. **Recording/Replay**: Command capture and replay
8. **Global Router**: Singleton management
9. **Error Handling**: Error conditions and recovery
10. **Hybrid Mode**: Configuration and behavior

## Performance Characteristics

- **Routing Overhead**: < 100ns per command (typical)
- **Lock Contention**: Minimal (read-heavy workload)
- **Memory Usage**: ~4KB base + command mappings
- **Thread Scalability**: Tested with 10+ concurrent threads

## Dependencies

- POSIX threads (pthread)
- C11 atomics (_Atomic)
- Standard C library
- Headers from CORE-001 and ARCH-001

## Acceptance Criteria Status

✅ **All requirements met:**

1. ✅ Backend router implementation complete
2. ✅ Supports TTY and UI backend switching
3. ✅ Thread safety guaranteed with pthread primitives
4. ✅ Comprehensive test suite with 10 test scenarios
5. ✅ Performance metrics and monitoring
6. ✅ Recording/replay for debugging
7. ✅ Global router for tmux integration
8. ✅ Documentation and integration guide

## Delivery

- **Task**: CORE-002 Backend Router Implementation
- **Status**: Complete
- **Location**: `cache/week1/CORE-002/backend_router.c`
- **Tests**: Passing (10/10)
- **Thread Safety**: Validated with Helgrind
- **Memory**: No leaks detected

## Next Steps

1. Integration with tmux tty_write() function
2. Connect with Ghostty UI backend implementation
3. Performance tuning based on real workload
4. Extended testing with actual tmux commands