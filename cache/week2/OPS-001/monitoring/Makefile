# Makefile for tmux-Ghostty Monitoring System

CC = clang
CFLAGS = -Wall -Wextra -O2 -g -pthread -DENABLE_MONITORING=1
LDFLAGS = -lpthread -lm

# Directories
SRC_DIR = src
INC_DIR = include
TEST_DIR = tests
BUILD_DIR = build
BIN_DIR = bin

# Source files
SRCS = $(SRC_DIR)/metrics_collection.c \
       $(SRC_DIR)/prometheus_exporter.c \
       $(SRC_DIR)/logging_framework.c

# Object files
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Test files
TEST_SRCS = $(TEST_DIR)/test_monitoring_overhead.c
TEST_BINS = $(patsubst $(TEST_DIR)/%.c,$(BIN_DIR)/%,$(TEST_SRCS))

# Library output
LIB_NAME = libtmux_ghostty_monitoring.a
LIB_PATH = $(BIN_DIR)/$(LIB_NAME)

# Default target
all: dirs $(LIB_PATH) tests

# Create directories
dirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

# Build library
$(LIB_PATH): $(OBJS)
	@echo "Creating static library: $@"
	@ar rcs $@ $^
	@echo "✅ Library built successfully"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Build tests
tests: $(TEST_BINS)

$(BIN_DIR)/test_%: $(TEST_DIR)/test_%.c $(LIB_PATH)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(LDFLAGS)
	@echo "✅ Test built: $@"

# Run tests
test: tests
	@echo "Running tests..."
	@$(BIN_DIR)/test_monitoring_overhead
	@echo "✅ All tests passed"

# Test overhead specifically
test-overhead: $(BIN_DIR)/test_monitoring_overhead
	@echo "Testing monitoring overhead..."
	@$<

# Build with debug symbols
debug: CFLAGS += -DDEBUG -g3 -O0
debug: all

# Build without monitoring (for comparison)
no-monitoring: CFLAGS := $(filter-out -DENABLE_MONITORING=1,$(CFLAGS))
no-monitoring: all

# Profile with instruments (macOS)
profile: tests
	@echo "Profiling with Instruments..."
	@xcrun xctrace record --template 'Time Profiler' --launch $(BIN_DIR)/test_monitoring_overhead

# Memory check with valgrind (if available)
memcheck: tests
	@echo "Running memory check..."
	@if command -v valgrind >/dev/null 2>&1; then \
		valgrind --leak-check=full --show-leak-kinds=all $(BIN_DIR)/test_monitoring_overhead; \
	else \
		echo "Valgrind not found, using leaks command..."; \
		leaks --atExit -- $(BIN_DIR)/test_monitoring_overhead; \
	fi

# Run DTrace scripts (requires sudo)
dtrace-event-loop:
	@echo "Starting event loop trace (requires sudo)..."
	@sudo dtrace -s dtrace_scripts/event_loop_trace.d -p $$(pgrep tmux_ghostty || echo 1)

dtrace-ffi:
	@echo "Starting FFI trace (requires sudo)..."
	@sudo dtrace -s dtrace_scripts/ffi_calls.d -p $$(pgrep tmux_ghostty || echo 1)

# Start Prometheus exporter demo
demo-exporter: tests
	@echo "Starting Prometheus exporter demo..."
	@echo "Access metrics at: http://localhost:9090/metrics"
	@$(BIN_DIR)/test_monitoring_overhead &
	@sleep 2
	@curl -s http://localhost:9090/metrics | head -50
	@pkill test_monitoring_overhead

# Install Grafana dashboard
install-dashboard:
	@echo "Installing Grafana dashboard..."
	@if [ -z "$(GRAFANA_URL)" ]; then \
		echo "Set GRAFANA_URL environment variable"; \
		exit 1; \
	fi
	@curl -X POST $(GRAFANA_URL)/api/dashboards/import \
		-H "Content-Type: application/json" \
		-d @grafana_dashboards/tmux_ghostty_overview.json

# Generate documentation
docs:
	@echo "Generating documentation..."
	@doxygen -g - | sed 's/PROJECT_NAME.*=.*/PROJECT_NAME = "tmux-Ghostty Monitoring"/' | \
		sed 's/INPUT.*=.*/INPUT = $(INC_DIR) $(SRC_DIR)/' | \
		sed 's/GENERATE_LATEX.*=.*/GENERATE_LATEX = NO/' | \
		doxygen -

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@rm -f /tmp/test_*.txt /tmp/test_*.json
	@echo "✅ Clean complete"

# Show configuration
info:
	@echo "=== Build Configuration ==="
	@echo "CC: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "Library: $(LIB_PATH)"
	@echo "Tests: $(TEST_BINS)"
	@echo "==========================="

# Continuous monitoring mode
monitor: tests
	@echo "Starting continuous monitoring..."
	@while true; do \
		clear; \
		echo "=== tmux-Ghostty Monitoring ==="; \
		echo "Time: $$(date)"; \
		echo ""; \
		$(BIN_DIR)/test_monitoring_overhead | grep -E "(ops/sec|overhead|PASS|FAIL)"; \
		echo ""; \
		echo "Press Ctrl-C to stop..."; \
		sleep 5; \
	done

.PHONY: all dirs tests test test-overhead debug no-monitoring profile memcheck \
        dtrace-event-loop dtrace-ffi demo-exporter install-dashboard docs \
        clean info monitor

# Help target
help:
	@echo "tmux-Ghostty Monitoring System Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build library and tests (default)"
	@echo "  test             - Run all tests"
	@echo "  test-overhead    - Test monitoring overhead specifically"
	@echo "  debug            - Build with debug symbols"
	@echo "  no-monitoring    - Build without monitoring enabled"
	@echo "  profile          - Profile with Instruments (macOS)"
	@echo "  memcheck         - Check for memory leaks"
	@echo "  dtrace-event-loop - Run event loop DTrace script"
	@echo "  dtrace-ffi       - Run FFI DTrace script"
	@echo "  demo-exporter    - Demo Prometheus exporter"
	@echo "  install-dashboard - Install Grafana dashboard"
	@echo "  docs             - Generate documentation"
	@echo "  clean            - Remove build artifacts"
	@echo "  info             - Show build configuration"
	@echo "  monitor          - Continuous monitoring mode"
	@echo "  help             - Show this help message"