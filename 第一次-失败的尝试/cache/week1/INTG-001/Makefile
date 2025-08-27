# Makefile for Ghostty Backend Integration
# Author: INTG-001 (Zig-Ghostty Integration Specialist)
# Date: 2025-08-25

# Compiler settings
CC = clang
ZIG = zig
CFLAGS = -Wall -Wextra -O2 -g -pthread -I..
ZIGFLAGS = --release=safe

# Source files
C_SOURCES = backend_ghostty.c test_ghostty_backend.c
ZIG_SOURCES = ghostty_ffi_bridge.zig
HEADERS = ../ARCH-001/ui_backend.h ../ARCH-001/backend_router.h ../CORE-001/tty_write_hooks.h

# Output files
BACKEND_LIB = libghostty_backend.a
FFI_LIB = libghostty_ffi.a
TEST_EXEC = test_ghostty_backend

# Build directories
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

# Object files
C_OBJS = $(C_SOURCES:%.c=$(OBJ_DIR)/%.o)
ZIG_OBJ = $(OBJ_DIR)/ghostty_ffi_bridge.o

# Default target
all: $(BUILD_DIR) $(BACKEND_LIB) $(FFI_LIB) $(TEST_EXEC)

# Create build directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(OBJ_DIR)

# Compile C backend
$(OBJ_DIR)/backend_ghostty.o: backend_ghostty.c $(HEADERS)
	@echo "Compiling backend_ghostty.c..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compile test suite
$(OBJ_DIR)/test_ghostty_backend.o: test_ghostty_backend.c $(HEADERS)
	@echo "Compiling test_ghostty_backend.c..."
	$(CC) $(CFLAGS) -c $< -o $@

# Build Zig FFI bridge
$(ZIG_OBJ): ghostty_ffi_bridge.zig
	@echo "Building Zig FFI bridge..."
	$(ZIG) build-obj $(ZIGFLAGS) $< -femit-bin=$@

# Create static libraries
$(BACKEND_LIB): $(OBJ_DIR)/backend_ghostty.o
	@echo "Creating backend library..."
	ar rcs $(BUILD_DIR)/$@ $^

$(FFI_LIB): $(ZIG_OBJ)
	@echo "Creating FFI library..."
	ar rcs $(BUILD_DIR)/$@ $^

# Link test executable
$(TEST_EXEC): $(OBJ_DIR)/test_ghostty_backend.o $(BACKEND_LIB) $(FFI_LIB)
	@echo "Linking test executable..."
	$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -lghostty_backend -lghostty_ffi -o $(BUILD_DIR)/$@

# Run tests
test: $(TEST_EXEC)
	@echo "Running tests..."
	@cd $(BUILD_DIR) && ./$(TEST_EXEC)

# Run with valgrind
valgrind: $(TEST_EXEC)
	@echo "Running with valgrind..."
	@cd $(BUILD_DIR) && valgrind --leak-check=full --show-leak-kinds=all ./$(TEST_EXEC)

# Performance benchmark
benchmark: $(TEST_EXEC)
	@echo "Running performance benchmark..."
	@cd $(BUILD_DIR) && time ./$(TEST_EXEC) --benchmark

# Generate documentation
docs:
	@echo "Generating documentation..."
	@doxygen Doxyfile 2>/dev/null || echo "Doxygen not found, skipping docs"

# Clean build artifacts
clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR)

# Install headers and libraries
install: all
	@echo "Installing..."
	@mkdir -p /usr/local/include/ghostty
	@cp backend_ghostty.h /usr/local/include/ghostty/ 2>/dev/null || true
	@cp $(BUILD_DIR)/$(BACKEND_LIB) /usr/local/lib/
	@cp $(BUILD_DIR)/$(FFI_LIB) /usr/local/lib/

# Uninstall
uninstall:
	@echo "Uninstalling..."
	@rm -f /usr/local/include/ghostty/backend_ghostty.h
	@rm -f /usr/local/lib/$(BACKEND_LIB)
	@rm -f /usr/local/lib/$(FFI_LIB)

# Show statistics
stats:
	@echo "=== Code Statistics ==="
	@echo "C Lines:"
	@wc -l *.c | tail -1
	@echo "Zig Lines:"
	@wc -l *.zig | tail -1
	@echo "Total Functions:"
	@grep -c "^[a-zA-Z].*(" *.c *.zig | tail -1

# Help
help:
	@echo "Ghostty Backend Integration Build System"
	@echo "========================================"
	@echo "Targets:"
	@echo "  all        - Build everything (default)"
	@echo "  test       - Run test suite"
	@echo "  valgrind   - Run tests with memory checking"
	@echo "  benchmark  - Run performance benchmarks"
	@echo "  docs       - Generate documentation"
	@echo "  clean      - Remove build artifacts"
	@echo "  install    - Install libraries and headers"
	@echo "  uninstall  - Remove installed files"
	@echo "  stats      - Show code statistics"
	@echo "  help       - Show this help message"

.PHONY: all test valgrind benchmark docs clean install uninstall stats help