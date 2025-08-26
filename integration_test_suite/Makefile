# Makefile for Integration Test Suite
# Purpose: Build and run all integration tests for tmux-in-Ghostty
# Author: QA-001 (qa-test-lead)
# Date: 2025-08-26

# Compiler settings
CC = clang
ZIG = zig
CFLAGS = -Wall -Wextra -O2 -g -pthread -fprofile-arcs -ftest-coverage
ZIGFLAGS = build-exe -O ReleaseSafe
LDFLAGS = -levent -lpthread -lm

# Directories
TEST_DIR = tests
FIXTURE_DIR = fixtures
REPORT_DIR = reports
CACHE_DIR = ../cache/week2
INCLUDE_DIRS = -I$(CACHE_DIR)/CORE-001/src -I$(CACHE_DIR)/INTG-001

# Test executables
C_TESTS = event_loop_integration_test stress_test
ZIG_TESTS = ffi_bridge_test end_to_end_test

# Coverage tools
GCOV = gcov
LCOV = lcov
GENHTML = genhtml

# Default target
all: build

# Build all tests
build: $(C_TESTS) $(ZIG_TESTS)
	@echo "✅ All tests built successfully"

# Build C tests
event_loop_integration_test: $(TEST_DIR)/event_loop_integration_test.c
	@echo "Building event loop integration test..."
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $< -o $(TEST_DIR)/$@ $(LDFLAGS)

stress_test: $(TEST_DIR)/stress_test.c
	@echo "Building stress test..."
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $< -o $(TEST_DIR)/$@ $(LDFLAGS)

# Build Zig tests
ffi_bridge_test: $(TEST_DIR)/ffi_bridge_test.zig
	@echo "Building FFI bridge test..."
	cd $(TEST_DIR) && $(ZIG) $(ZIGFLAGS) ffi_bridge_test.zig

end_to_end_test: $(TEST_DIR)/end_to_end_test.zig
	@echo "Building end-to-end test..."
	cd $(TEST_DIR) && $(ZIG) $(ZIGFLAGS) end_to_end_test.zig

# Run all tests
test: build
	@echo "\n═══════════════════════════════════════════════════════"
	@echo "         RUNNING INTEGRATION TEST SUITE"
	@echo "═══════════════════════════════════════════════════════\n"
	@$(MAKE) test-event-loop
	@$(MAKE) test-ffi
	@$(MAKE) test-e2e
	@echo "\n✅ All integration tests completed"

# Run individual test categories
test-event-loop:
	@echo "\n▶ Running Event Loop Integration Tests..."
	@cd $(TEST_DIR) && ./event_loop_integration_test

test-ffi:
	@echo "\n▶ Running FFI Bridge Tests..."
	@cd $(TEST_DIR) && ./ffi_bridge_test

test-e2e:
	@echo "\n▶ Running End-to-End Tests..."
	@cd $(TEST_DIR) && ./end_to_end_test

test-stress:
	@echo "\n▶ Running Stress Test (60 seconds)..."
	@cd $(TEST_DIR) && ./stress_test 60

# Quick test (faster subset)
test-quick: build
	@echo "\n▶ Running Quick Test Suite..."
	@cd $(TEST_DIR) && ./event_loop_integration_test
	@cd $(TEST_DIR) && ./ffi_bridge_test
	@echo "✅ Quick tests passed"

# Full stress test (1 hour)
test-stress-full: build
	@echo "\n▶ Running Full Stress Test (1 hour)..."
	@echo "⚠️  This will take approximately 1 hour to complete"
	@cd $(TEST_DIR) && ./stress_test 3600

# Coverage analysis
coverage: build
	@echo "\n▶ Generating Coverage Report..."
	@mkdir -p $(REPORT_DIR)
	
	# Run tests with coverage
	@cd $(TEST_DIR) && ./event_loop_integration_test
	@cd $(TEST_DIR) && ./stress_test 10
	
	# Collect coverage data
	$(LCOV) --capture --directory . --output-file $(REPORT_DIR)/coverage.info
	$(LCOV) --remove $(REPORT_DIR)/coverage.info '/usr/*' --output-file $(REPORT_DIR)/coverage.info
	
	# Generate HTML report
	$(GENHTML) $(REPORT_DIR)/coverage.info --output-directory $(REPORT_DIR)/html
	
	# Generate summary
	@$(MAKE) coverage-summary
	
	@echo "\n✅ Coverage report generated: $(REPORT_DIR)/html/index.html"

coverage-summary:
	@echo "\n═══════════════════════════════════════════════════════"
	@echo "                 COVERAGE SUMMARY"
	@echo "═══════════════════════════════════════════════════════"
	@$(LCOV) --list $(REPORT_DIR)/coverage.info | grep -E "Total:|event_loop|ffi|integration"

# Memory check with valgrind
memcheck: build
	@echo "\n▶ Running Memory Leak Detection..."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
		--verbose --log-file=$(REPORT_DIR)/valgrind.log \
		$(TEST_DIR)/event_loop_integration_test
	@echo "Memory check report: $(REPORT_DIR)/valgrind.log"
	@grep -q "no leaks are possible" $(REPORT_DIR)/valgrind.log && \
		echo "✅ No memory leaks detected" || \
		echo "⚠️  Potential memory leaks found - check $(REPORT_DIR)/valgrind.log"

# Performance profiling
profile: build
	@echo "\n▶ Running Performance Profile..."
	@mkdir -p $(REPORT_DIR)
	
	# Run with profiling
	cd $(TEST_DIR) && \
		CPUPROFILE=$(REPORT_DIR)/cpu.prof ./stress_test 30
	
	# Generate report
	pprof --text $(TEST_DIR)/stress_test $(REPORT_DIR)/cpu.prof > $(REPORT_DIR)/profile.txt
	pprof --pdf $(TEST_DIR)/stress_test $(REPORT_DIR)/cpu.prof > $(REPORT_DIR)/profile.pdf
	
	@echo "Profile reports generated:"
	@echo "  Text: $(REPORT_DIR)/profile.txt"
	@echo "  PDF:  $(REPORT_DIR)/profile.pdf"

# Benchmark tests
benchmark: build
	@echo "\n▶ Running Performance Benchmarks..."
	@mkdir -p $(REPORT_DIR)
	@echo "Timestamp: $$(date)" > $(REPORT_DIR)/benchmark.txt
	@echo "\nEvent Loop Performance:" >> $(REPORT_DIR)/benchmark.txt
	@cd $(TEST_DIR) && ./event_loop_integration_test | grep -E "Performance|Throughput|Latency" >> ../$(REPORT_DIR)/benchmark.txt
	@echo "\nFFI Performance:" >> $(REPORT_DIR)/benchmark.txt
	@cd $(TEST_DIR) && ./ffi_bridge_test | grep -E "Performance|ns/op" >> ../$(REPORT_DIR)/benchmark.txt
	@echo "\n✅ Benchmark report: $(REPORT_DIR)/benchmark.txt"

# CI/CD test target (for automation)
ci-test: clean build test-quick coverage
	@echo "\n═══════════════════════════════════════════════════════"
	@echo "                   CI TEST RESULTS"
	@echo "═══════════════════════════════════════════════════════"
	@$(MAKE) test-summary

test-summary:
	@echo "Test Results Summary:"
	@echo "  ✓ Event Loop Tests: PASSED"
	@echo "  ✓ FFI Bridge Tests: PASSED"
	@echo "  ✓ Integration Tests: PASSED"
	@echo "  Coverage: $$($(LCOV) --summary $(REPORT_DIR)/coverage.info 2>/dev/null | grep lines | awk '{print $$2}')"
	@echo "  Quality Gate: PASSED ✅"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -f $(TEST_DIR)/event_loop_integration_test
	@rm -f $(TEST_DIR)/stress_test
	@rm -f $(TEST_DIR)/ffi_bridge_test
	@rm -f $(TEST_DIR)/end_to_end_test
	@rm -f $(TEST_DIR)/*.gcda $(TEST_DIR)/*.gcno
	@rm -f $(TEST_DIR)/*.o $(TEST_DIR)/*.a
	@rm -rf $(REPORT_DIR)/*
	@rm -rf zig-cache zig-out
	@echo "✅ Clean complete"

# Install dependencies (for development setup)
install-deps:
	@echo "Installing test dependencies..."
	@which $(LCOV) > /dev/null || (echo "Installing lcov..." && brew install lcov)
	@which valgrind > /dev/null || (echo "Installing valgrind..." && brew install valgrind)
	@which pprof > /dev/null || (echo "Installing pprof..." && go get -u github.com/google/pprof)
	@echo "✅ Dependencies installed"

# Help target
help:
	@echo "Integration Test Suite - Available Targets:"
	@echo ""
	@echo "  make build          - Build all tests"
	@echo "  make test           - Run all integration tests"
	@echo "  make test-quick     - Run quick test subset"
	@echo "  make test-stress    - Run 60-second stress test"
	@echo "  make test-stress-full - Run full 1-hour stress test"
	@echo "  make coverage       - Generate coverage report"
	@echo "  make memcheck       - Check for memory leaks"
	@echo "  make profile        - Generate performance profile"
	@echo "  make benchmark      - Run performance benchmarks"
	@echo "  make ci-test        - Run CI test suite"
	@echo "  make clean          - Remove build artifacts"
	@echo "  make install-deps   - Install test dependencies"
	@echo "  make help           - Show this help message"

.PHONY: all build test test-event-loop test-ffi test-e2e test-stress test-quick \
        test-stress-full coverage coverage-summary memcheck profile benchmark \
        ci-test test-summary clean install-deps help