# Makefile for Testing and Coverage
# Author: INTG-001 (Zig-Ghostty Integration Specialist)
# Target: ≥50% Code Coverage

CC = clang
ZIG = zig
CFLAGS = -Wall -Wextra -g -O0 -pthread -I../..
COVERAGE_FLAGS = --coverage -fprofile-arcs -ftest-coverage
LDFLAGS = -lpthread -lm

# Directories
BUILD_DIR = build
COVERAGE_DIR = coverage

# Source files
TEST_SOURCES = test_callbacks.c test_error_handling.c test_concurrency.c
BACKEND_SOURCE = ../backend_ghostty.c
FFI_SOURCE = ../ghostty_ffi_bridge.zig

# Test executables
TEST_CALLBACKS = $(BUILD_DIR)/test_callbacks
TEST_ERROR = $(BUILD_DIR)/test_error_handling
TEST_CONCURRENCY = $(BUILD_DIR)/test_concurrency
TEST_FFI = $(BUILD_DIR)/test_ffi

# Default target
all: test coverage

# Create directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(COVERAGE_DIR):
	@mkdir -p $(COVERAGE_DIR)

# Build test executables with coverage
$(TEST_CALLBACKS): test_callbacks.c test_stubs.c | $(BUILD_DIR)
	@echo "Building callback tests with coverage..."
	@$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $@ test_callbacks.c test_stubs.c $(LDFLAGS)

$(TEST_ERROR): test_error_handling.c test_stubs.c | $(BUILD_DIR)
	@echo "Building error handling tests with coverage..."
	@$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $@ test_error_handling.c test_stubs.c $(LDFLAGS)

$(TEST_CONCURRENCY): test_concurrency.c test_stubs.c | $(BUILD_DIR)
	@echo "Building concurrency tests with coverage..."
	@$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $@ test_concurrency.c test_stubs.c $(LDFLAGS)

$(TEST_FFI): test_ffi_bridge.zig | $(BUILD_DIR)
	@echo "Building FFI tests..."
	@$(ZIG) test --test-filter "" $< -femit-bin=$@

# Run all tests
test: $(TEST_CALLBACKS) $(TEST_ERROR) $(TEST_CONCURRENCY) $(TEST_FFI) | $(COVERAGE_DIR)
	@echo "=== Running Test Suite ==="
	@echo ""
	@echo "1. Testing all 22 callbacks..."
	@$(TEST_CALLBACKS)
	@echo ""
	@echo "2. Testing error handling..."
	@$(TEST_ERROR)
	@echo ""
	@echo "3. Testing concurrency..."
	@$(TEST_CONCURRENCY)
	@echo ""
	@echo "4. Testing FFI bridge..."
	@$(TEST_FFI)
	@echo ""
	@echo "=== All Tests Complete ==="

# Clean build artifacts
clean:
	@rm -rf $(BUILD_DIR) $(COVERAGE_DIR)
	@rm -f *.gcda *.gcno *.gcov

# Quick test without coverage
quick: $(BUILD_DIR)
	@$(CC) $(CFLAGS) -o $(BUILD_DIR)/test_quick test_callbacks.c $(LDFLAGS)
	@$(BUILD_DIR)/test_quick

.PHONY: all test coverage check-coverage clean quick