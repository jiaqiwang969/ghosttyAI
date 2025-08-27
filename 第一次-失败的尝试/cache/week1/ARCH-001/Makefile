# Makefile - Build and Test P0 Fixes
# Author: ARCH-001 (System Architect)
# Purpose: Compile and test all P0 defect fixes

CC = gcc
CFLAGS = -Wall -Wextra -I. -g
LDFLAGS = 

# Source files
SOURCES = interface_adapter.c ui_backend_impl.c
HEADERS = tty_ctx_unified.h interface_compatibility.h ui_backend_callbacks_fixed.h interface_adapter.h

# Test programs
TEST_PROG = test_p0_fixes
EXAMPLE_GHOSTTY = examples/backend_ghostty_example
EXAMPLE_HOOKS = examples/tty_write_hooks_example

# Object files
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean test examples validate

all: $(TEST_PROG) examples

# Main test program
$(TEST_PROG): test_p0_fixes.c $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Examples
examples: $(EXAMPLE_GHOSTTY) $(EXAMPLE_HOOKS)

$(EXAMPLE_GHOSTTY): examples/backend_ghostty_example.c $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJECTS) $(LDFLAGS)

$(EXAMPLE_HOOKS): examples/tty_write_hooks_example.c $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJECTS) $(LDFLAGS)

# Object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Run tests
test: $(TEST_PROG)
	@echo "Running P0 defect fix tests..."
	@./$(TEST_PROG)

# Run validation script
validate:
	@echo "Running validation script..."
	@./validate_p0_fixes.sh

# Run examples
run-examples: examples
	@echo "\n=== Running Ghostty Backend Example ==="
	@./$(EXAMPLE_GHOSTTY)
	@echo "\n=== Running TTY Hooks Example ==="
	@./$(EXAMPLE_HOOKS)

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TEST_PROG) $(EXAMPLE_GHOSTTY) $(EXAMPLE_HOOKS)
	rm -f *.o examples/*.o

# Help
help:
	@echo "P0 Defect Fix Build System"
	@echo "=========================="
	@echo "Targets:"
	@echo "  make all       - Build test program and examples"
	@echo "  make test      - Run test suite"
	@echo "  make examples  - Build example implementations"
	@echo "  make validate  - Run validation script"
	@echo "  make clean     - Remove build artifacts"
	@echo ""
	@echo "Quick validation:"
	@echo "  make clean test validate"