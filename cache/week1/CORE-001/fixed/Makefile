# Makefile for tty_write_hooks
# Purpose: Build and test TTY write hook implementations
# Author: CORE-001 (c-tmux-specialist)
# Date: 2025-08-25

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -O2 -g
CFLAGS_TEST = $(CFLAGS) -DTEST_BUILD
INCLUDES = -I. -I../ARCH-001 -I../../tmux

# Source files
SRCS = tty_write_hooks.c
OBJS = $(SRCS:.c=.o)

# Test files
TEST_SRCS = tests/test_tty_write_hooks.c
TEST_OBJS = $(TEST_SRCS:.c=.o)
TEST_EXEC = test_tty_write_hooks

# Targets
.PHONY: all clean test run-tests

all: $(OBJS)

# Build object files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Build test executable
$(TEST_EXEC): $(TEST_OBJS)
	$(CC) $(CFLAGS_TEST) $(INCLUDES) -o $@ $^

tests/%.o: tests/%.c
	$(CC) $(CFLAGS_TEST) $(INCLUDES) -c $< -o $@

# Run tests
test: $(TEST_EXEC)
	./$(TEST_EXEC)

run-tests: test

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TEST_OBJS) $(TEST_EXEC)
	rm -f *.gcov *.gcda *.gcno
	rm -f tests/*.gcov tests/*.gcda tests/*.gcno

# Code coverage
coverage: CFLAGS += --coverage
coverage: clean all test
	gcov $(SRCS)
	@echo "Coverage report generated. View *.gcov files for details."

# Static analysis
analyze:
	@echo "Running static analysis..."
	cppcheck --enable=all --suppress=missingIncludeSystem $(SRCS) $(TEST_SRCS)

# Format check
format-check:
	@echo "Checking code formatting..."
	clang-format --dry-run --Werror $(SRCS) $(TEST_SRCS) *.h

# Documentation
docs:
	@echo "Generating documentation..."
	doxygen -g Doxyfile 2>/dev/null || true
	doxygen Doxyfile

# Installation (for integration)
install: $(OBJS)
	@echo "Ready for integration into libtmuxcore"
	@echo "Object files: $(OBJS)"
	@echo "Header file: tty_write_hooks.h"

# Help
help:
	@echo "TTY Write Hooks Build System"
	@echo "============================="
	@echo "Targets:"
	@echo "  all         - Build object files"
	@echo "  test        - Build and run tests"
	@echo "  clean       - Remove build artifacts"
	@echo "  coverage    - Generate code coverage report"
	@echo "  analyze     - Run static analysis"
	@echo "  format-check - Check code formatting"
	@echo "  docs        - Generate documentation"
	@echo "  install     - Prepare for integration"
	@echo "  help        - Show this help message"