# Makefile for Week 3 Integration Tests
# Task: T-305-R - End-to-End Integration Testing
# Purpose: Build and run comprehensive test suite for libtmuxcore integration
# Author: week3-ghostty-tmux-executor
# Date: 2025-08-26

CC = clang
CFLAGS = -Wall -Wextra -O2 -g -march=native -pthread
LDFLAGS = -ldl -lm -pthread

# Library path
LIBPATH = /Users/jqwang/98-ghosttyAI/tmux
LIBRARY = $(LIBPATH)/libtmuxcore.dylib

# Test executables
TESTS = test_library_loading \
        test_ffi_bridge \
        test_integration_e2e \
        benchmark_performance

# Build all tests
all: $(TESTS)

# Individual test targets
test_library_loading: test_library_loading.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "✅ Built $@"

test_ffi_bridge: test_ffi_bridge.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "✅ Built $@"

test_integration_e2e: test_integration_e2e.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "✅ Built $@"

benchmark_performance: benchmark_performance.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "✅ Built $@"

# Run individual tests
run-library: test_library_loading
	@echo "\n=== Running Library Loading Tests ==="
	@./test_library_loading

run-ffi: test_ffi_bridge
	@echo "\n=== Running FFI Bridge Tests ==="
	@./test_ffi_bridge

run-integration: test_integration_e2e
	@echo "\n=== Running Integration Tests ==="
	@./test_integration_e2e

run-benchmark: benchmark_performance
	@echo "\n=== Running Performance Benchmarks ==="
	@./benchmark_performance

# Run all tests
test: all
	@echo "\n========================================="
	@echo "     WEEK 3 INTEGRATION TEST SUITE"
	@echo "========================================="
	@./test_library_loading
	@echo ""
	@./test_ffi_bridge
	@echo ""
	@./test_integration_e2e
	@echo ""
	@echo "\n=== Final Test: Performance ==="
	@./benchmark_performance
	@echo "\n========================================="
	@echo "     TEST SUITE COMPLETE"
	@echo "========================================="

# Run tests with memory checking (macOS doesn't have valgrind, use leaks)
test-memory: all
	@echo "\n=== Memory Leak Testing ==="
	@echo "Testing library loading..."
	@leaks --atExit -- ./test_library_loading > /dev/null 2>&1 && echo "✅ No leaks" || echo "❌ Leaks detected"
	@echo "Testing FFI bridge..."
	@leaks --atExit -- ./test_ffi_bridge > /dev/null 2>&1 && echo "✅ No leaks" || echo "❌ Leaks detected"
	@echo "Testing integration..."
	@leaks --atExit -- ./test_integration_e2e > /dev/null 2>&1 && echo "✅ No leaks" || echo "❌ Leaks detected"

# Coverage report (requires gcov)
coverage: CFLAGS += -fprofile-arcs -ftest-coverage
coverage: clean all
	@./test_library_loading
	@./test_ffi_bridge
	@./test_integration_e2e
	@gcov *.c
	@echo "\n=== Coverage Summary ==="
	@grep -A 1 "File" *.gcov | grep -E "Lines executed"

# Quick validation
quick-test: test_library_loading test_integration_e2e
	@echo "\n=== Quick Validation ==="
	@./test_library_loading
	@./test_integration_e2e
	@echo "\n✅ Quick validation complete"

# Performance-only
perf: benchmark_performance
	@./benchmark_performance

# Clean build artifacts
clean:
	rm -f $(TESTS)
	rm -f *.o *.gcov *.gcda *.gcno
	rm -rf *.dSYM
	@echo "✅ Cleaned build artifacts"

# Help
help:
	@echo "Week 3 Integration Test Suite"
	@echo "============================="
	@echo "Available targets:"
	@echo "  make all          - Build all tests"
	@echo "  make test         - Run full test suite"
	@echo "  make quick-test   - Run quick validation"
	@echo "  make perf         - Run performance benchmarks only"
	@echo "  make test-memory  - Run with memory leak detection"
	@echo "  make coverage     - Generate coverage report"
	@echo "  make clean        - Clean build artifacts"
	@echo ""
	@echo "Individual test targets:"
	@echo "  make run-library     - Test library loading"
	@echo "  make run-ffi        - Test FFI bridge"  
	@echo "  make run-integration - Test end-to-end integration"
	@echo "  make run-benchmark  - Run performance benchmarks"

.PHONY: all test test-memory coverage quick-test perf clean help \
        run-library run-ffi run-integration run-benchmark