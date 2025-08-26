# Integration Test Suite for tmux-in-Ghostty

## Overview

Comprehensive integration test suite for validating the tmux-in-Ghostty integration, covering event loop abstraction, FFI bridge, and end-to-end functionality.

## Test Coverage

### 1. Event Loop Integration (`event_loop_integration_test.c`)
- Basic event loop operations
- Backend switching (libevent ↔ Ghostty ↔ hybrid)
- Thread safety under concurrent load
- Performance regression testing (<1% overhead)
- Memory leak detection
- Signal handling
- Timer events

### 2. FFI Bridge Tests (`ffi_bridge_test.zig`)
- C to Zig type conversions
- Callback function pointer compatibility
- Memory ownership transitions
- Zero-copy operations
- Complex struct marshaling
- Error handling across FFI boundary
- Thread safety of FFI calls
- Performance benchmarks

### 3. End-to-End Tests (`end_to_end_test.zig`)
- Complete tmux session lifecycle
- Event processing from tmux to Ghostty
- Window and pane management
- PTY to UI data flow
- Performance under typical load
- Error recovery and resilience
- Memory management

### 4. Stress Test (`stress_test.c`)
- 1000 concurrent sessions
- 10000 operations per second
- CPU usage monitoring (<50%)
- Memory leak detection over time
- 1-hour stability test

## Quick Start

```bash
# Run all tests
./run_tests.sh

# Run quick tests only (no stress test)
./run_tests.sh --quick

# Run specific test category
make test-event-loop
make test-ffi
make test-e2e
make test-stress
```

## Requirements

### Required
- Clang or GCC with C11 support
- Zig compiler (0.11.0 or later)
- libevent (for event loop backend)
- pthread support

### Optional (for full reporting)
- lcov (coverage reports)
- valgrind (memory leak detection)
- pprof (performance profiling)

## Installation

```bash
# Install dependencies on macOS
brew install lcov valgrind

# Or use the Makefile
make install-deps
```

## Building Tests

```bash
# Build all tests
make build

# Clean and rebuild
make clean build
```

## Running Tests

### Full Test Suite
```bash
# Run complete test suite with reports
./run_tests.sh

# Or use Make
make test
```

### Individual Tests
```bash
# Event loop tests
./tests/event_loop_integration_test

# FFI bridge tests
./tests/ffi_bridge_test

# End-to-end tests
./tests/end_to_end_test

# Stress test (60 seconds)
./tests/stress_test 60
```

### CI/CD Integration
```bash
# Run CI test suite (quick tests + coverage)
make ci-test
```

## Coverage Analysis

```bash
# Generate coverage report
make coverage

# View HTML report
open reports/coverage_html/index.html
```

## Memory Checking

```bash
# Run valgrind memory check
make memcheck

# Check report
cat reports/valgrind.log
```

## Performance Profiling

```bash
# Generate performance profile
make profile

# View results
cat reports/profile.txt
```

## Test Scenarios

20+ predefined test scenarios covering:
- Basic session operations
- Complex window/pane layouts
- Heavy text output
- Event loop switching
- FFI callback chains
- Memory pressure
- Terminal resizing
- Unicode/emoji support
- Network latency simulation
- And more...

See `fixtures/test_scenarios.json` for complete list.

## Quality Gates

All tests must pass these criteria:

### Performance
- Event loop overhead: <1%
- FFI latency: <100ns
- Throughput: >200k ops/s
- P99 latency: <0.5ms

### Stability
- Memory leaks: 0
- CPU usage: <50% under stress
- 1-hour stress test: PASS
- Thread safety: verified

### Coverage
- Overall: >75%
- Critical paths: 100%
- FFI boundaries: 100%

## Test Results

Reports are generated in `reports/` directory:
- `test_report.html` - Main test report
- `coverage_html/` - Coverage visualization
- `valgrind.log` - Memory check results
- `benchmark.txt` - Performance metrics
- `test_run_*.log` - Detailed test logs

## Troubleshooting

### Build Failures
```bash
# Check compiler and dependencies
clang --version
zig version
pkg-config --libs libevent
```

### Test Failures
- Check `reports/test_run_*.log` for detailed output
- Run individual test with verbose output
- Use `make memcheck` to detect memory issues

### Coverage Issues
```bash
# Ensure gcov is available
which gcov lcov genhtml

# Rebuild with coverage flags
make clean
CFLAGS="-fprofile-arcs -ftest-coverage" make build
```

## Development

### Adding New Tests

1. Create test file in `tests/`
2. Update Makefile with new target
3. Add test scenario to `fixtures/test_scenarios.json`
4. Run full test suite to verify

### Test Naming Convention
- C tests: `*_test.c`
- Zig tests: `*_test.zig`
- Fixtures: `test_*.json`

## Contact

Task: T-401
Owner: QA-001 (qa-test-lead)
Priority: P0 CRITICAL

## License

Part of the tmux-in-Ghostty integration project.