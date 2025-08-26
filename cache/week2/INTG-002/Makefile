# Makefile for Copy Mode Processing Implementation
# Task: T-204 - Copy mode with Ghostty clipboard integration
# Author: INTG-002 (integration-dev)
# Date: 2025-08-26

CC = clang
CFLAGS = -Wall -Wextra -Werror -std=c11 -O2 -g
LDFLAGS = -framework Foundation -framework CoreFoundation -lobjc

# Directories
SRC_DIR = src
TEST_DIR = tests
INCLUDE_DIR = include
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

# Source files
SRCS = $(SRC_DIR)/copy_mode_backend.c \
       $(SRC_DIR)/clipboard_integration.c \
       $(SRC_DIR)/selection_renderer.c

# Object files
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Test files
TEST_SRCS = $(TEST_DIR)/test_copy_mode.c
TEST_OBJS = $(patsubst $(TEST_DIR)/%.c,$(OBJ_DIR)/%.o,$(TEST_SRCS))

# Targets
LIBRARY = $(BUILD_DIR)/libcopymode.a
TEST_BINARY = $(BUILD_DIR)/test_copy_mode

# Default target
all: $(LIBRARY) $(TEST_BINARY)

# Create directories
$(BUILD_DIR) $(OBJ_DIR):
	@mkdir -p $@

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Compile test files
$(OBJ_DIR)/%.o: $(TEST_DIR)/%.c | $(OBJ_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Build static library
$(LIBRARY): $(OBJS) | $(BUILD_DIR)
	@echo "AR  $@"
	@ar rcs $@ $(OBJS)

# Build test binary
$(TEST_BINARY): $(TEST_OBJS) $(LIBRARY)
	@echo "LD  $@"
	@$(CC) $(CFLAGS) $(TEST_OBJS) $(LIBRARY) $(LDFLAGS) -o $@

# Run tests
test: $(TEST_BINARY)
	@echo "Running tests..."
	@$(TEST_BINARY)

# Run tests with valgrind for memory leak detection
memcheck: $(TEST_BINARY)
	@echo "Running memory leak check..."
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
		--suppressions=macos.supp $(TEST_BINARY)

# Performance profiling
profile: $(TEST_BINARY)
	@echo "Running performance profiling..."
	@$(TEST_BINARY) --profile > $(BUILD_DIR)/profile.txt
	@echo "Profile saved to $(BUILD_DIR)/profile.txt"

# Generate coverage report
coverage: CFLAGS += --coverage
coverage: clean $(TEST_BINARY)
	@echo "Generating coverage report..."
	@$(TEST_BINARY)
	@lcov --capture --directory $(OBJ_DIR) --output-file $(BUILD_DIR)/coverage.info
	@genhtml $(BUILD_DIR)/coverage.info --output-directory $(BUILD_DIR)/coverage_html
	@echo "Coverage report generated at $(BUILD_DIR)/coverage_html/index.html"

# Clean build artifacts
clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR)

# Install headers and library
install: $(LIBRARY)
	@echo "Installing..."
	@install -d /usr/local/include/copymode
	@install -m 644 $(INCLUDE_DIR)/*.h /usr/local/include/copymode/
	@install -m 644 $(LIBRARY) /usr/local/lib/

# Format source code
format:
	@echo "Formatting code..."
	@clang-format -i $(SRCS) $(TEST_SRCS) $(INCLUDE_DIR)/*.h

# Static analysis
analyze:
	@echo "Running static analysis..."
	@clang --analyze $(CFLAGS) -I$(INCLUDE_DIR) $(SRCS)

# Check for common issues
check: format analyze
	@echo "Running cppcheck..."
	@cppcheck --enable=all --suppress=missingIncludeSystem \
		-I$(INCLUDE_DIR) $(SRC_DIR) $(TEST_DIR)

# Generate documentation
docs:
	@echo "Generating documentation..."
	@doxygen Doxyfile

# Performance benchmark
benchmark: $(TEST_BINARY)
	@echo "Running performance benchmarks..."
	@$(TEST_BINARY) --benchmark

# Integration with tmux
tmux-integration: $(LIBRARY)
	@echo "Building tmux integration..."
	@$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -I../../tmux \
		integration/tmux_copy_mode_hooks.c $(LIBRARY) \
		-o $(BUILD_DIR)/tmux_copy_test

# Help target
help:
	@echo "Available targets:"
	@echo "  all        - Build library and tests (default)"
	@echo "  test       - Run test suite"
	@echo "  memcheck   - Run valgrind memory leak check"
	@echo "  profile    - Run performance profiling"
	@echo "  coverage   - Generate code coverage report"
	@echo "  clean      - Remove build artifacts"
	@echo "  install    - Install library and headers"
	@echo "  format     - Format source code"
	@echo "  analyze    - Run static analysis"
	@echo "  check      - Run all checks (format, analyze, cppcheck)"
	@echo "  docs       - Generate documentation"
	@echo "  benchmark  - Run performance benchmarks"
	@echo "  help       - Show this help message"

.PHONY: all test memcheck profile coverage clean install format analyze check docs benchmark help tmux-integration