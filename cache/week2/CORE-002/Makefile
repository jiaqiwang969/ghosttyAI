# Makefile for T-202: Grid Operations Batch Optimization
# Author: CORE-002 (libtmux-core-developer)
# Date: 2025-08-26

CC = gcc
CFLAGS = -Wall -Wextra -O3 -march=native -mtune=native -mavx2 -pthread
CFLAGS_DEBUG = -Wall -Wextra -g -O0 -DDEBUG -pthread -fsanitize=address
LDFLAGS = -lpthread -lm -lrt

# Directories
SRC_DIR = src
INC_DIR = include
BENCHMARK_DIR = benchmarks
BUILD_DIR = build
BUILD_DEBUG_DIR = build/debug

# Source files
SRCS = $(SRC_DIR)/grid_batch_ops.c \
       $(SRC_DIR)/dirty_tracking.c

# Header files
HEADERS = $(INC_DIR)/grid_callbacks.h

# Object files
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
OBJS_DEBUG = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DEBUG_DIR)/%.o,$(SRCS))

# Benchmark executable
BENCHMARK = $(BUILD_DIR)/benchmark_grid_ops
BENCHMARK_DEBUG = $(BUILD_DEBUG_DIR)/benchmark_grid_ops_debug

# Default target
all: directories $(BENCHMARK)

# Debug build
debug: directories_debug $(BENCHMARK_DEBUG)

# Create build directories
directories:
	@mkdir -p $(BUILD_DIR)

directories_debug:
	@mkdir -p $(BUILD_DEBUG_DIR)

# Build object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@echo "  CC  $<"
	@$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

$(BUILD_DEBUG_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@echo "  CC  $< (debug)"
	@$(CC) $(CFLAGS_DEBUG) -I$(INC_DIR) -c $< -o $@

# Build benchmark executable
$(BENCHMARK): $(OBJS) $(BENCHMARK_DIR)/benchmark_grid_ops.c
	@echo "  LD  $@"
	@$(CC) $(CFLAGS) -I$(INC_DIR) $(OBJS) $(BENCHMARK_DIR)/benchmark_grid_ops.c -o $@ $(LDFLAGS)

$(BENCHMARK_DEBUG): $(OBJS_DEBUG) $(BENCHMARK_DIR)/benchmark_grid_ops.c
	@echo "  LD  $@ (debug)"
	@$(CC) $(CFLAGS_DEBUG) -I$(INC_DIR) $(OBJS_DEBUG) $(BENCHMARK_DIR)/benchmark_grid_ops.c -o $@ $(LDFLAGS)

# Run benchmarks
benchmark: $(BENCHMARK)
	@echo "Running performance benchmarks..."
	@echo "================================"
	@$(BENCHMARK)

# Run benchmarks with profiling
profile: $(BENCHMARK)
	@echo "Running with profiling..."
	perf record -g $(BENCHMARK)
	perf report

# Memory check with valgrind
memcheck: $(BENCHMARK_DEBUG)
	@echo "Running memory check..."
	valgrind --leak-check=full --show-leak-kinds=all \
	         --track-origins=yes --verbose \
	         $(BENCHMARK_DEBUG)

# Check for race conditions
helgrind: $(BENCHMARK_DEBUG)
	@echo "Checking for race conditions..."
	valgrind --tool=helgrind $(BENCHMARK_DEBUG)

# Generate assembly for inspection
asm: $(SRC_DIR)/grid_batch_ops.c
	$(CC) $(CFLAGS) -S -masm=intel -I$(INC_DIR) $< -o $(BUILD_DIR)/grid_batch_ops.s

# Performance report
report: $(BENCHMARK)
	@echo "Generating performance report..."
	@$(BENCHMARK) > performance_report.txt 2>&1
	@echo "Report saved to performance_report.txt"
	@echo ""
	@echo "=== Performance Summary ==="
	@grep -E "(P99:|Speedup:|Memory:|Status:)" performance_report.txt | tail -20

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(BUILD_DEBUG_DIR)
	@rm -f performance_report.txt perf.data perf.data.old

# Install (copy headers for integration)
install:
	@echo "Installing headers..."
	@mkdir -p /usr/local/include/libtmuxcore
	@cp $(INC_DIR)/grid_callbacks.h /usr/local/include/libtmuxcore/

# Generate documentation
docs:
	@echo "Generating documentation..."
	@doxygen -g Doxyfile 2>/dev/null || true
	@echo "PROJECT_NAME = \"Grid Operations Batch Optimization\"" >> Doxyfile
	@echo "INPUT = $(INC_DIR) $(SRC_DIR)" >> Doxyfile
	@echo "GENERATE_LATEX = NO" >> Doxyfile
	@echo "EXTRACT_ALL = YES" >> Doxyfile
	@doxygen Doxyfile

# Test targets for CI/CD
test: test-unit test-performance test-memory

test-unit: $(BENCHMARK)
	@echo "Running unit tests..."
	@$(BENCHMARK) | grep -q "PASS" && echo "✓ Unit tests passed" || (echo "✗ Unit tests failed" && exit 1)

test-performance: $(BENCHMARK)
	@echo "Running performance tests..."
	@$(BENCHMARK) | grep -q "Achieved 10x speedup" && echo "✓ Performance target met" || (echo "✗ Performance target not met" && exit 1)

test-memory: $(BENCHMARK_DEBUG)
	@echo "Running memory tests..."
	@valgrind --leak-check=full --error-exitcode=1 --quiet $(BENCHMARK_DEBUG) > /dev/null 2>&1 && \
	echo "✓ No memory leaks detected" || (echo "✗ Memory leaks detected" && exit 1)

# Verify implementation meets requirements
verify: $(BENCHMARK)
	@echo "Verifying implementation requirements..."
	@echo ""
	@echo "Checking P99 latency < 0.3ms..."
	@$(BENCHMARK) | grep "P99:" | awk '{print $$2}' | while read lat; do \
		if [ "$$(echo "$$lat < 300" | bc)" -eq 1 ]; then \
			echo "  ✓ P99 latency $$lat us < 300 us"; \
		else \
			echo "  ✗ P99 latency $$lat us >= 300 us"; \
			exit 1; \
		fi \
	done
	@echo ""
	@echo "Checking memory overhead < 10%..."
	@$(BENCHMARK) | grep "Overhead:" | awk '{print $$2}' | sed 's/%//' | while read overhead; do \
		if [ "$$(echo "$$overhead < 10" | bc)" -eq 1 ]; then \
			echo "  ✓ Memory overhead $$overhead% < 10%"; \
		else \
			echo "  ✗ Memory overhead $$overhead% >= 10%"; \
			exit 1; \
		fi \
	done
	@echo ""
	@echo "Checking 10x speedup achieved..."
	@$(BENCHMARK) | grep -q "Achieved 10x speedup" && \
		echo "  ✓ 10x speedup achieved" || \
		(echo "  ✗ 10x speedup not achieved" && exit 1)
	@echo ""
	@echo "✅ All requirements verified!"

# Help target
help:
	@echo "Grid Operations Batch Optimization - Build Targets"
	@echo "=================================================="
	@echo ""
	@echo "Build targets:"
	@echo "  all          - Build optimized benchmark (default)"
	@echo "  debug        - Build debug version with sanitizers"
	@echo "  clean        - Remove all build artifacts"
	@echo ""
	@echo "Test targets:"
	@echo "  benchmark    - Run performance benchmarks"
	@echo "  test         - Run all tests (unit, performance, memory)"
	@echo "  verify       - Verify implementation meets requirements"
	@echo "  memcheck     - Check for memory leaks with valgrind"
	@echo "  helgrind     - Check for race conditions"
	@echo ""
	@echo "Analysis targets:"
	@echo "  profile      - Run with perf profiling"
	@echo "  asm          - Generate assembly output"
	@echo "  report       - Generate performance report"
	@echo ""
	@echo "Other targets:"
	@echo "  docs         - Generate Doxygen documentation"
	@echo "  install      - Install headers to system"
	@echo "  help         - Show this help message"

.PHONY: all debug clean benchmark test verify memcheck helgrind profile asm report docs install help \
        directories directories_debug test-unit test-performance test-memory