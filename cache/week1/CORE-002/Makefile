# Makefile for Backend Router
# Author: CORE-002 (libtmux-core-developer)
# Date: 2025-08-25

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -pthread -O2 -g
INCLUDES = -I. -I../CORE-001 -I../ARCH-001

# Source files
SRCS = backend_router.c
TEST_SRCS = test_backend_router.c

# Object files
OBJS = $(SRCS:.c=.o)
TEST_OBJS = $(TEST_SRCS:.c=.o)

# Targets
TARGET = libbackend_router.a
TEST_TARGET = test_backend_router

# Default target
all: $(TARGET) $(TEST_TARGET)

# Library target
$(TARGET): $(OBJS)
	ar rcs $@ $^
	@echo "✓ Built static library: $@"

# Test executable
$(TEST_TARGET): $(TEST_OBJS) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJS) -L. -lbackend_router -lpthread
	@echo "✓ Built test executable: $@"

# Object file compilation
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Run tests
test: $(TEST_TARGET)
	@echo "Running backend router tests..."
	@./$(TEST_TARGET)

# Check thread safety with helgrind
helgrind: $(TEST_TARGET)
	valgrind --tool=helgrind --history-level=full ./$(TEST_TARGET)

# Memory leak check
memcheck: $(TEST_TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TEST_TARGET)

# Performance profiling
profile: $(TEST_TARGET)
	perf record -g ./$(TEST_TARGET)
	perf report

# Generate documentation
docs:
	@echo "Generating documentation..."
	doxygen -g Doxyfile 2>/dev/null || true
	doxygen Doxyfile

# Clean
clean:
	rm -f $(OBJS) $(TEST_OBJS) $(TARGET) $(TEST_TARGET)
	rm -f *.gcov *.gcda *.gcno
	rm -rf html latex
	@echo "✓ Cleaned build artifacts"

# Install (for integration)
install: $(TARGET)
	@mkdir -p ../../lib ../../include
	@cp $(TARGET) ../../lib/
	@cp backend_router.h ../../include/
	@echo "✓ Installed to cache/week1/lib and cache/week1/include"

# Dependencies
backend_router.o: backend_router.c backend_router.h ../ARCH-001/ui_backend.h ../CORE-001/tty_write_hooks.h
test_backend_router.o: test_backend_router.c backend_router.h

.PHONY: all test clean install docs helgrind memcheck profile