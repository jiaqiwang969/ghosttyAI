# Makefile for Ghostty × tmux Integration Testing
# QA-002 Integration Test Suite
# Date: 2025-08-25

CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -pthread -std=c11
COVERAGE_FLAGS = -fprofile-arcs -ftest-coverage
INCLUDES = -I../include
LIBS = -lpthread

# Directories
SRC_DIR = ../src
TEST_DIR = .
INCLUDE_DIR = ../include
BUILD_DIR = build
COVERAGE_DIR = ../coverage

# Source files
SOURCES = $(SRC_DIR)/tty_write_hooks.c \
          $(SRC_DIR)/backend_router.c \
          $(SRC_DIR)/backend_ghostty_minimal.c

# Test files
TEST_SOURCES = $(TEST_DIR)/integrated_test_minimal.c

# Object files
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
TEST_OBJECTS = $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%.o,$(TEST_SOURCES))

# Targets
TEST_EXEC = integrated_test
COVERAGE_EXEC = integrated_test_coverage

# Default target
all: $(BUILD_DIR) $(TEST_EXEC)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build test executable
$(TEST_EXEC): $(OBJECTS) $(TEST_OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

# Build with coverage
coverage: CFLAGS += $(COVERAGE_FLAGS)
coverage: $(BUILD_DIR) $(COVERAGE_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(COVERAGE_FLAGS) -o $(COVERAGE_EXEC) $(SOURCES) $(TEST_SOURCES) $(LIBS)

# Run tests
test: $(TEST_EXEC)
	./$(TEST_EXEC)

# Run tests with coverage
test-coverage: coverage
	./$(COVERAGE_EXEC)
	gcov -b $(SOURCES) $(TEST_SOURCES)
	lcov --capture --directory . --output-file $(COVERAGE_DIR)/coverage.info
	genhtml $(COVERAGE_DIR)/coverage.info --output-directory $(COVERAGE_DIR)/html

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile test files
$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Create coverage directory
$(COVERAGE_DIR):
	mkdir -p $(COVERAGE_DIR)

# Performance benchmark
benchmark: $(TEST_EXEC)
	@echo "Running performance benchmark..."
	time ./$(TEST_EXEC) | grep -A 5 "Performance Benchmark"

# Memory check with valgrind
memcheck: $(TEST_EXEC)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TEST_EXEC)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TEST_EXEC) $(COVERAGE_EXEC)
	rm -f *.gcno *.gcda *.gcov
	rm -rf $(COVERAGE_DIR)

# Generate documentation
docs:
	@echo "Generating documentation..."
	@echo "Components integrated:"
	@echo "  - ARCH-001: UI Backend Interface"
	@echo "  - CORE-001: TTY Write Hooks (22 functions)"
	@echo "  - CORE-002: Backend Router"
	@echo "  - INTG-001: Ghostty Backend"
	@echo "  - QA-001: Test Framework"
	@echo "  - QA-002: Integration Testing"

# Show help
help:
	@echo "Available targets:"
	@echo "  make all          - Build test executable"
	@echo "  make test         - Run integration tests"
	@echo "  make coverage     - Build with coverage support"
	@echo "  make test-coverage- Run tests and generate coverage report"
	@echo "  make benchmark    - Run performance benchmarks"
	@echo "  make memcheck     - Check for memory leaks"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make docs         - Show component documentation"
	@echo "  make help         - Show this help message"

.PHONY: all test coverage test-coverage benchmark memcheck clean docs help