# libtmuxcore Build System Documentation

**DevOps Build Engineer (OPS-001)**  
**Date**: 2025-08-25  
**Version**: 1.0.0

## Overview

This document describes the build system for the Ghostty × tmux integration project, specifically the extraction of tmux functionality into a library (`libtmuxcore`) for direct embedding into Ghostty.

## Project Structure

```
/Users/jqwang/98-ghosttyAI/
├── tmux/                       # Original tmux source
├── ghostty/                    # Ghostty terminal source
└── cache/week1/OPS-001/       # Build system workspace
    ├── build/                  # Build artifacts
    │   ├── Makefile           # Main build configuration
    │   ├── lib/               # Compiled libraries
    │   ├── include/           # Public headers
    │   └── obj/               # Object files
    ├── tests/                  # Test suites
    ├── ci/                     # CI/CD configurations
    └── docs/                   # This documentation
```

## Build System Components

### 1. libtmuxcore Library

The core library extracts essential tmux functionality:

- **Grid Management**: Terminal grid operations
- **Session/Window/Pane**: Multiplexer structures
- **Input Handling**: Keyboard and mouse events
- **Rendering Callbacks**: UI backend hooks

### 2. Build Targets

```bash
# Build everything
make all

# Build static library only
make libtmuxcore.a

# Build dynamic library only
make libtmuxcore.dylib

# Run tests
make test

# Install to system
make install PREFIX=/usr/local

# Clean build artifacts
make clean
```

### 3. Dependencies

Required dependencies (installed via Homebrew):

- `libevent`: Event notification library
- `ncurses`: Terminal UI library
- `utf8proc`: UTF-8 processing
- `cmocka`: Unit testing framework

Install all dependencies:
```bash
brew install libevent ncurses utf8proc cmocka pkg-config
```

## Building libtmuxcore

### Quick Start

```bash
cd cache/week1/OPS-001/build
make clean
make all
make test
```

### Detailed Build Process

1. **Configure Environment**
   ```bash
   export CC=clang
   export CFLAGS="-arch arm64 -O2"
   export LDFLAGS="-L/opt/homebrew/lib"
   ```

2. **Build Library**
   ```bash
   make dirs           # Create build directories
   make headers        # Generate public headers
   make libtmuxcore.a  # Build static library
   make libtmuxcore.dylib # Build dynamic library
   ```

3. **Run Tests**
   ```bash
   make test-setup     # Install test framework
   make test          # Run all tests
   ```

## Testing Framework

### Test Structure

Tests are organized by component:

- `test_grid_operations.c`: Grid functionality tests
- `test_session_management.c`: Session handling tests
- `test_input_handling.c`: Input processing tests
- `test_callbacks.c`: UI backend callback tests

### Writing Tests

Example test using cmocka:

```c
#include <cmocka.h>
#include "libtmuxcore.h"

static void test_grid_creation(void **state) {
    tmc_grid_t* grid = tmc_create_grid(80, 24);
    assert_non_null(grid);
    tmc_free_grid(grid);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_grid_creation),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
```

### Running Individual Tests

```bash
# Compile test
gcc -o test_grid test_grid_operations.c -lcmocka -ltmuxcore

# Run with valgrind
valgrind --leak-check=full ./test_grid
```

## CI/CD Pipeline

### GitHub Actions Workflow

The CI/CD pipeline runs on every push and pull request:

1. **Build Stage**
   - Compile tmux from source
   - Build libtmuxcore library
   - Build Ghostty with Zig

2. **Test Stage**
   - Run unit tests
   - Run integration tests
   - Check code coverage

3. **Quality Stage**
   - Static analysis with cppcheck
   - Code formatting with clang-format
   - Memory leak detection with valgrind

4. **Release Stage**
   - Create release artifacts
   - Generate documentation
   - Deploy to package registry

### Running CI Locally

Use [act](https://github.com/nektos/act) to run GitHub Actions locally:

```bash
# Install act
brew install act

# Run all jobs
act

# Run specific job
act -j build-libtmuxcore
```

## API Documentation

### Core Functions

```c
// Server lifecycle
tmc_server_t* tmc_server_new(void);
void tmc_server_free(tmc_server_t* server);

// UI Backend registration
int tmc_server_set_ui_backend(tmc_server_t* server, 
                              tmc_ui_backend_t* backend);

// Session management
tmc_session_t* tmc_create_session(tmc_server_t* server, 
                                  const char* name);

// Input handling
int tmc_send_keys(tmc_pane_t* pane, const char* keys, 
                 size_t len);
```

### Callback Interface

```c
typedef struct {
    void (*on_grid_update)(void* ctx, tmc_pane_t* pane, 
                          int x, int y, int w, int h);
    void (*on_layout_change)(void* ctx, tmc_window_t* window);
    void (*on_copy_mode)(void* ctx, tmc_pane_t* pane, bool entering);
    void (*on_title_change)(void* ctx, tmc_window_t* window, 
                           const char* title);
    void* ctx;
} tmc_ui_backend_t;
```

## Performance Considerations

### Optimization Flags

- **Release**: `-O2` for balanced optimization
- **Debug**: `-O0 -g` for debugging
- **Performance**: `-O3 -march=native` for maximum speed

### Memory Management

- Use `valgrind` to detect leaks
- Run `make memcheck` for automated checks
- Profile with Instruments on macOS

## Troubleshooting

### Common Issues

1. **Missing Dependencies**
   ```bash
   # Check installed packages
   brew list
   
   # Reinstall dependencies
   brew reinstall libevent ncurses utf8proc
   ```

2. **Build Failures**
   ```bash
   # Clean and rebuild
   make clean
   make check  # Verify environment
   make all
   ```

3. **Test Failures**
   ```bash
   # Run tests with verbose output
   make test VERBOSE=1
   
   # Run single test
   ./test_grid_operations
   ```

## Integration with Ghostty

### Linking libtmuxcore

In Ghostty's `build.zig`:

```zig
const tmuxcore = b.addStaticLibrary("tmuxcore", null);
tmuxcore.addIncludePath("cache/week1/OPS-001/build/include");
tmuxcore.addLibraryPath("cache/week1/OPS-001/build/lib");
tmuxcore.linkSystemLibrary("tmuxcore");
```

### FFI Bridge Example

```zig
const c = @cImport({
    @cInclude("libtmuxcore.h");
});

pub fn initTmuxCore() !*c.tmc_server_t {
    const server = c.tmc_server_new() orelse return error.InitFailed;
    
    var backend = c.tmc_ui_backend_t{
        .on_grid_update = onGridUpdate,
        .on_layout_change = onLayoutChange,
        .ctx = null,
    };
    
    _ = c.tmc_server_set_ui_backend(server, &backend);
    return server;
}
```

## Deliverables

All build system components are located in:
```
/Users/jqwang/98-ghosttyAI/cache/week1/OPS-001/build/
```

### Files Delivered

1. **Makefile**: Complete build configuration
2. **libtmuxcore.h**: Public API header
3. **test_grid_operations.c**: Example test suite
4. **.github/workflows/ci.yml**: CI/CD pipeline
5. **README.md**: This documentation

## Next Steps

1. **Week 2**: Integrate with Ghostty's build system
2. **Week 3**: Add performance benchmarks
3. **Week 4**: Complete test coverage
4. **Week 5**: Production release preparation

## Contact

**DevOps Build Engineer (OPS-001)**  
Session: `ghostty-devops:0`  
Reports to: Independent (consults with Orchestrator)

---

*Build system prepared and ready for integration. All components tested and documented.*