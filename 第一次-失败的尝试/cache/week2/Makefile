# Ghostty × tmux Integration Makefile
# Week 2 Complete Build System

# Compiler settings
CC = clang
ZIG = zig

# Detect architecture and set appropriate flags
ARCH := $(shell uname -m)
ifeq ($(ARCH),arm64)
    # Apple Silicon (M1/M2/M3/M4) - use NEON instead of AVX2
    CFLAGS = -Wall -Wextra -O3 -march=native -fPIC -DUSE_NEON
else
    # Intel x86_64 - use AVX2
    CFLAGS = -Wall -Wextra -O3 -march=native -mavx2 -fPIC -DUSE_AVX2
endif

ZIGFLAGS = -O ReleaseFast
LDFLAGS = -lpthread -framework ApplicationServices -framework CoreFoundation

# Directories
SRC_DIR = src
INC_DIR = include
TEST_DIR = tests
BUILD_DIR = build
INTEGRATION_DIR = integration

# Source files (temporarily excluding copy_mode_backend.c and zig)
C_SOURCES = \
    $(SRC_DIR)/event_loop_router.c \
    $(SRC_DIR)/grid_operations.c \
    $(SRC_DIR)/layout_manager.c

# ZIG_SOURCES = \
#     $(INTEGRATION_DIR)/ffi_bridge.zig

TEST_SOURCES = \
    $(TEST_DIR)/integration_test.c

# Object files
C_OBJECTS = $(C_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
# ZIG_OBJECTS = $(ZIG_SOURCES:$(INTEGRATION_DIR)/%.zig=$(BUILD_DIR)/%.o)
TEST_OBJECTS = $(TEST_SOURCES:$(TEST_DIR)/%.c=$(BUILD_DIR)/test_%.o)

# Test targets
SIMPLE_TEST = $(BUILD_DIR)/simple_test
TEST_BINARY = $(BUILD_DIR)/integration_test
STATIC_LIB = $(BUILD_DIR)/libtmuxcore.a

# Default target
all: dirs $(LIBRARY) $(STATIC_LIB) $(SIMPLE_TEST)

# Create build directories
dirs:
	@mkdir -p $(BUILD_DIR)

# Build dynamic library
$(LIBRARY): $(C_OBJECTS)
	@echo "🔗 Linking dynamic library..."
	$(CC) -shared -o $@ $^ $(LDFLAGS)
	@echo "✅ Built $(LIBRARY)"

# Build static library
$(STATIC_LIB): $(C_OBJECTS)
	@echo "📚 Creating static library..."
	ar rcs $@ $^
	@echo "✅ Built $(STATIC_LIB)"

# Compile C sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "🔨 Compiling $<..."
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Compile Zig sources
$(BUILD_DIR)/%.o: $(INTEGRATION_DIR)/%.zig
	@echo "⚡ Compiling Zig $<..."
	$(ZIG) build-obj $(ZIGFLAGS) -femit-bin=$@ $<

# Build simple test
$(SIMPLE_TEST): $(TEST_DIR)/simple_test.c $(STATIC_LIB)
	@echo "🧪 Building simple test..."
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $< $(STATIC_LIB) $(LDFLAGS)
	@echo "✅ Built $(SIMPLE_TEST)"

# Build test binary
$(TEST_BINARY): $(TEST_OBJECTS) $(LIBRARY)
	@echo "🧪 Building test suite..."
	$(CC) -o $@ $(TEST_OBJECTS) -L$(BUILD_DIR) -ltmuxcore $(LDFLAGS)
	@echo "✅ Built $(TEST_BINARY)"

# Compile test sources
$(BUILD_DIR)/test_%.o: $(TEST_DIR)/%.c
	@echo "🔨 Compiling test $<..."
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Run tests
test: $(TEST_BINARY)
	@echo "🧪 Running integration tests..."
	@export DYLD_LIBRARY_PATH=$(BUILD_DIR):$$DYLD_LIBRARY_PATH && $(TEST_BINARY)

# Performance benchmark
benchmark: $(TEST_BINARY)
	@echo "📊 Running performance benchmarks..."
	@export DYLD_LIBRARY_PATH=$(BUILD_DIR):$$DYLD_LIBRARY_PATH && \
	$(TEST_BINARY) --benchmark

# Memory check with Valgrind (macOS: use leaks instead)
memcheck: $(TEST_BINARY)
	@echo "🔍 Checking for memory leaks..."
	@export DYLD_LIBRARY_PATH=$(BUILD_DIR):$$DYLD_LIBRARY_PATH && \
	leaks --atExit -- $(TEST_BINARY)

# Thread safety check
thread-check: $(TEST_BINARY)
	@echo "🔍 Checking thread safety..."
	@export DYLD_LIBRARY_PATH=$(BUILD_DIR):$$DYLD_LIBRARY_PATH && \
	TSAN_OPTIONS=halt_on_error=1 $(TEST_BINARY)

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)

# Install library
install: $(LIBRARY) $(STATIC_LIB)
	@echo "📦 Installing libraries..."
	install -d /usr/local/lib
	install -m 755 $(LIBRARY) /usr/local/lib/
	install -m 644 $(STATIC_LIB) /usr/local/lib/
	install -d /usr/local/include/tmuxcore
	install -m 644 $(INC_DIR)/*.h /usr/local/include/tmuxcore/
	@echo "✅ Installation complete"

# Uninstall library
uninstall:
	@echo "🗑️  Uninstalling libraries..."
	rm -f /usr/local/lib/libtmuxcore.dylib
	rm -f /usr/local/lib/libtmuxcore.a
	rm -rf /usr/local/include/tmuxcore
	@echo "✅ Uninstallation complete"

# Generate documentation
docs:
	@echo "📚 Generating documentation..."
	doxygen Doxyfile

# Performance profiling
profile: $(TEST_BINARY)
	@echo "📈 Running performance profiling..."
	@export DYLD_LIBRARY_PATH=$(BUILD_DIR):$$DYLD_LIBRARY_PATH && \
	xcrun xctrace record --template 'Time Profiler' --launch $(TEST_BINARY) --output profile.trace

# Debug build
debug: CFLAGS += -g -O0 -DDEBUG -fsanitize=address
debug: LDFLAGS += -fsanitize=address
debug: clean all

# Release build with LTO
release: CFLAGS += -flto -DNDEBUG
release: LDFLAGS += -flto
release: clean all

# Static analysis
analyze:
	@echo "🔍 Running static analysis..."
	clang-tidy $(C_SOURCES) -- $(CFLAGS) -I$(INC_DIR)

# Format code
format:
	@echo "🎨 Formatting code..."
	clang-format -i $(C_SOURCES) $(INC_DIR)/*.h $(TEST_SOURCES)
	zig fmt $(ZIG_SOURCES)

# Show configuration
info:
	@echo "Ghostty × tmux Integration Build Configuration"
	@echo "==============================================="
	@echo "CC:        $(CC)"
	@echo "ZIG:       $(ZIG)"
	@echo "CFLAGS:    $(CFLAGS)"
	@echo "ZIGFLAGS:  $(ZIGFLAGS)"
	@echo "LDFLAGS:   $(LDFLAGS)"
	@echo "Library:   $(LIBRARY)"
	@echo "Static:    $(STATIC_LIB)"
	@echo "Tests:     $(TEST_BINARY)"

# CI/CD targets
ci: clean all test memcheck

# Help target
help:
	@echo "Ghostty × tmux Integration - Make Targets"
	@echo "=========================================="
	@echo "  all         - Build everything"
	@echo "  test        - Run integration tests"
	@echo "  benchmark   - Run performance benchmarks"
	@echo "  memcheck    - Check for memory leaks"
	@echo "  clean       - Remove build artifacts"
	@echo "  install     - Install libraries system-wide"
	@echo "  debug       - Build with debug symbols and sanitizers"
	@echo "  release     - Build optimized release version"
	@echo "  format      - Format source code"
	@echo "  analyze     - Run static analysis"
	@echo "  docs        - Generate documentation"
	@echo "  help        - Show this help message"

.PHONY: all dirs test benchmark memcheck clean install uninstall docs profile debug release analyze format info ci help thread-check