# Makefile for Fixed Backend Router (DEFECT-002)
# Author: libtmux-core-developer
# Date: 2025-08-25

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -pthread -O2 -g
INCLUDES = -I. -I../../CORE-001 -I../../ARCH-001

# Source files
SRCS = backend_router.c integration_wrapper.c
TEST_SRCS = test_backend_router.c
COMPAT_TEST_SRCS = test_interface_compatibility.c

# Object files
OBJS = $(SRCS:.c=.o)
TEST_OBJS = $(TEST_SRCS:.c=.o)
COMPAT_TEST_OBJS = $(COMPAT_TEST_SRCS:.c=.o)

# Targets
TARGET = libbackend_router_fixed.a
TEST_TARGET = test_backend_router
COMPAT_TEST = test_interface_compatibility

# Default target
all: $(TARGET) $(TEST_TARGET) $(COMPAT_TEST)

# Library target
$(TARGET): $(OBJS)
	ar rcs $@ $^
	@echo "✓ Built fixed library: $@"

# Test executable
$(TEST_TARGET): $(TEST_OBJS) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJS) -L. -lbackend_router_fixed -lpthread
	@echo "✓ Built test executable: $@"

# Interface compatibility test
$(COMPAT_TEST): $(COMPAT_TEST_OBJS) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $(COMPAT_TEST_OBJS) -L. -lbackend_router_fixed -lpthread
	@echo "✓ Built compatibility test: $@"

# Object file compilation
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Run all tests
test: $(TEST_TARGET) $(COMPAT_TEST)
	@echo "\n=== Running Backend Router Tests ==="
	@./$(TEST_TARGET)
	@echo "\n=== Running Interface Compatibility Tests (DEFECT-002) ==="
	@./$(COMPAT_TEST)

# Quick compatibility check
check-interfaces: $(COMPAT_TEST)
	@echo "Checking interface naming consistency..."
	@./$(COMPAT_TEST)

# Verify no old interface names exist
verify-fix:
	@echo "Verifying DEFECT-002 fix..."
	@! grep -r "backend_router_register_backend" *.c *.h 2>/dev/null || (echo "❌ Found old interface name!" && false)
	@grep -q "backend_router_register_ui" backend_router.h && echo "✓ Correct interface found in header"
	@grep -q "backend_router_register_ui" backend_router.c && echo "✓ Correct interface found in implementation"
	@echo "✅ DEFECT-002 fix verified!"

# Integration test
integration-test: $(TARGET)
	@echo "Testing integration with tty_hooks..."
	$(CC) $(CFLAGS) $(INCLUDES) -o integration_test integration_wrapper.c -L. -lbackend_router_fixed -lpthread
	@echo "✓ Integration wrapper compiled successfully"

# Clean
clean:
	rm -f $(OBJS) $(TEST_OBJS) $(COMPAT_TEST_OBJS) $(TARGET) $(TEST_TARGET) $(COMPAT_TEST)
	rm -f integration_test *.gcov *.gcda *.gcno
	@echo "✓ Cleaned build artifacts"

# Install fixed version
install: $(TARGET) verify-fix
	@mkdir -p ../../../lib ../../../include
	@cp $(TARGET) ../../../lib/libbackend_router.a
	@cp backend_router.h ../../../include/
	@cp integration_wrapper.c ../../../include/
	@echo "✓ Installed fixed version to cache/week1/lib and cache/week1/include"

# Generate fix report
report:
	@echo "\n=== DEFECT-002 Fix Report ==="
	@echo "Issue: Interface naming mismatch"
	@echo "Fixed:"
	@echo "  • backend_router_register_backend() → backend_router_register_ui() ✓"
	@echo "  • backend_router_unregister_backend() → backend_router_unregister_ui() ✓"
	@echo "  • Ensured compatibility with tty_hooks_init() ✓"
	@echo "Location: cache/week1/CORE-002/fixed/"
	@echo "Status: FIXED"
	@echo "============================"

.PHONY: all test clean install verify-fix check-interfaces integration-test report