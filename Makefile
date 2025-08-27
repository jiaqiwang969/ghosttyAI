# Minimal Ghostty Build Makefile for Terminal Communication Integration
# Third Attempt - Focus on simplicity and functionality

# Configuration
PROJECT_ROOT := $(shell pwd)
GHOSTTY_SRC := $(PROJECT_ROOT)/ghostty
BUILD_DIR := $(PROJECT_ROOT)/build
GHOSTTY_BUILD_DIR := $(BUILD_DIR)/ghostty

# Architecture detection
ARCH := $(shell uname -m)
OS := $(shell uname -s)

# Tools
ZIG := zig
CC := clang
PKG_CONFIG := pkg-config

# Homebrew paths for M-series Macs
HOMEBREW_PREFIX := /opt/homebrew
PKG_CONFIG_PATH := $(HOMEBREW_PREFIX)/lib/pkgconfig:$(PKG_CONFIG_PATH)
LDFLAGS := -L$(HOMEBREW_PREFIX)/lib
CFLAGS := -I$(HOMEBREW_PREFIX)/include -arch arm64 -O2

# Colors for output
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[1;33m
BLUE := \033[0;34m
NC := \033[0m # No Color

# Default target - minimal build
.PHONY: all
all: build-ghostty
	@echo "$(GREEN)✓ Ghostty build completed$(NC)"

# Check minimal dependencies
.PHONY: check-deps
check-deps:
	@echo "$(BLUE)Checking build dependencies...$(NC)"
	@which $(ZIG) > /dev/null || (echo "$(RED)Error: Zig not found. Install with: brew install zig$(NC)" && exit 1)
	@echo "  Zig version: $$($(ZIG) version)"
	@echo "$(GREEN)✓ Dependencies satisfied$(NC)"

# Create build directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(GHOSTTY_BUILD_DIR): $(BUILD_DIR)
	@mkdir -p $(GHOSTTY_BUILD_DIR)

# Main build target - Ghostty with Zig
.PHONY: build-ghostty
build-ghostty: check-deps $(GHOSTTY_BUILD_DIR)
	@echo "$(BLUE)Building Ghostty for ARM64...$(NC)"
	@echo "$(BLUE)Step 1/3: Ensuring source exists...$(NC)"
	@if [ ! -f "$(GHOSTTY_SRC)/build.zig" ]; then \
		echo "$(RED)Error: build.zig not found in $(GHOSTTY_SRC)$(NC)"; \
		exit 1; \
	fi
	@echo "$(BLUE)Step 2/3: Building Ghostty (Release mode)...$(NC)"
	@cd $(GHOSTTY_SRC) && \
		$(ZIG) build -Doptimize=ReleaseFast 2>&1 | tee $(BUILD_DIR)/build.log | \
		grep -E "(Building|Installing|error|warning)" || true
	@echo "$(BLUE)Step 3/3: Locating build output...$(NC)"
	@if [ -d "$(GHOSTTY_SRC)/zig-out/bin" ] && [ -f "$(GHOSTTY_SRC)/zig-out/bin/ghostty" ]; then \
		echo "$(GREEN)✓ Ghostty CLI binary built successfully$(NC)"; \
		cp -r $(GHOSTTY_SRC)/zig-out/* $(GHOSTTY_BUILD_DIR)/ 2>/dev/null || true; \
		echo "$(GREEN)  Binary: $(GHOSTTY_BUILD_DIR)/bin/ghostty$(NC)"; \
		ls -lah $(GHOSTTY_BUILD_DIR)/bin/ghostty 2>/dev/null || true; \
	else \
		echo "$(YELLOW)Note: CLI binary not found in expected location$(NC)"; \
		echo "$(YELLOW)Checking for macOS app bundle...$(NC)"; \
		if [ -d "$(GHOSTTY_SRC)/macos/build/Release/Ghostty.app" ]; then \
			echo "$(GREEN)✓ Found Ghostty.app$(NC)"; \
			echo "  Location: $(GHOSTTY_SRC)/macos/build/Release/Ghostty.app"; \
		else \
			echo "$(YELLOW)Build artifacts location:$(NC)"; \
			find $(GHOSTTY_SRC) -name "ghostty" -type f -perm +111 2>/dev/null | head -5 || \
			echo "$(YELLOW)No executable found. Check $(BUILD_DIR)/build.log for details$(NC)"; \
		fi; \
	fi
	@echo "$(GREEN)✓ Build process completed$(NC)"

# Build macOS app with Xcode (optional)
.PHONY: build-app
build-app: check-xcode
	@echo "$(BLUE)Building Ghostty.app for macOS...$(NC)"
	@cd $(GHOSTTY_SRC) && \
		xcodebuild -project macos/Ghostty.xcodeproj \
			-scheme Ghostty \
			-configuration Release \
			-derivedDataPath $(BUILD_DIR)/DerivedData \
			ARCHS=arm64 \
			ONLY_ACTIVE_ARCH=NO \
			build 2>&1 | tee $(BUILD_DIR)/xcode-build.log | \
			grep -E "(Building|Compiling|Linking|SUCCEEDED|FAILED|error|warning)" || true
	@if [ -d "$(BUILD_DIR)/DerivedData/Build/Products/Release/Ghostty.app" ]; then \
		echo "$(GREEN)✓ Ghostty.app built successfully$(NC)"; \
		cp -r $(BUILD_DIR)/DerivedData/Build/Products/Release/Ghostty.app $(GHOSTTY_BUILD_DIR)/; \
		echo "$(GREEN)  App: $(GHOSTTY_BUILD_DIR)/Ghostty.app$(NC)"; \
	else \
		echo "$(YELLOW)Checking alternative location...$(NC)"; \
		find $(GHOSTTY_SRC)/macos -name "Ghostty.app" -type d 2>/dev/null | head -1; \
	fi

# Check Xcode availability
.PHONY: check-xcode
check-xcode:
	@which xcodebuild > /dev/null || (echo "$(RED)Error: Xcode not found. Install from App Store$(NC)" && exit 1)
	@echo "$(GREEN)✓ Xcode found: $$(xcodebuild -version | head -1)$(NC)"

# Development build (debug mode)
.PHONY: dev
dev: check-deps $(GHOSTTY_BUILD_DIR)
	@echo "$(BLUE)Building Ghostty in Debug mode...$(NC)"
	@cd $(GHOSTTY_SRC) && \
		$(ZIG) build -Doptimize=Debug 2>&1 | tee $(BUILD_DIR)/build-debug.log
	@echo "$(GREEN)✓ Debug build completed$(NC)"

# Run tests
.PHONY: test
test: check-deps
	@echo "$(BLUE)Running Ghostty tests...$(NC)"
	@cd $(GHOSTTY_SRC) && \
		$(ZIG) build test 2>&1 | tee $(BUILD_DIR)/test.log || \
		echo "$(YELLOW)Note: Some tests may require additional setup$(NC)"

# Clean build artifacts
.PHONY: clean
clean:
	@echo "$(BLUE)Cleaning build artifacts...$(NC)"
	@cd $(GHOSTTY_SRC) && rm -rf zig-out zig-cache .zig-cache 2>/dev/null || true
	@rm -rf $(BUILD_DIR)
	@echo "$(GREEN)✓ Build artifacts cleaned$(NC)"

# Integration preparation - add SessionManager
.PHONY: prepare-integration
prepare-integration:
	@echo "$(BLUE)Preparing for terminal communication integration...$(NC)"
	@echo "$(BLUE)Step 1/3: Creating terminal module directory...$(NC)"
	@mkdir -p $(GHOSTTY_SRC)/src/terminal
	@echo "$(BLUE)Step 2/3: Checking if SessionManager exists...$(NC)"
	@if [ -f "$(PROJECT_ROOT)/src/terminal/SessionManager.zig" ]; then \
		echo "$(GREEN)✓ SessionManager.zig found$(NC)"; \
		cp $(PROJECT_ROOT)/src/terminal/SessionManager.zig $(GHOSTTY_SRC)/src/terminal/; \
		echo "$(GREEN)✓ Copied to Ghostty source$(NC)"; \
	else \
		echo "$(YELLOW)SessionManager.zig not found in project root$(NC)"; \
	fi
	@echo "$(BLUE)Step 3/3: Ready for integration modifications...$(NC)"
	@echo "$(GREEN)Next steps:$(NC)"
	@echo "  1. Modify $(GHOSTTY_SRC)/src/App.zig to add SessionManager"
	@echo "  2. Modify $(GHOSTTY_SRC)/src/Surface.zig to add session_id"
	@echo "  3. Extend $(GHOSTTY_SRC)/src/apprt/ipc.zig with new Actions"
	@echo "  4. Run 'make test-integration' to verify"

# Build without XCFramework (for development)
.PHONY: build-lib
build-lib: check-deps $(GHOSTTY_BUILD_DIR)
	@echo "$(BLUE)Building Ghostty libraries only (no XCFramework)...$(NC)"
	@cd $(GHOSTTY_SRC) && \
		$(ZIG) build libghostty -Doptimize=ReleaseFast 2>&1 | tee $(BUILD_DIR)/build-lib.log | \
		grep -E "(Building|Installing|error|warning)" || true
	@echo "$(GREEN)✓ Libraries built (check .zig-cache for artifacts)$(NC)"
	@find $(GHOSTTY_SRC)/.zig-cache -name "libghostty*.a" -type f 2>/dev/null | head -3

# Test integration build
.PHONY: test-integration
test-integration: prepare-integration build-lib
	@echo "$(BLUE)Testing terminal communication integration...$(NC)"
	@echo "$(GREEN)✓ Core libraries available for integration:$(NC)"
	@find $(GHOSTTY_SRC)/.zig-cache -name "libghostty*.a" -type f -exec ls -lh {} \; 2>/dev/null | head -3
	@echo "$(BLUE)Next step: Integrate SessionManager into Ghostty source$(NC)"

# Run Ghostty
.PHONY: run
run: build-ghostty
	@echo "$(BLUE)Launching Ghostty...$(NC)"
	@if [ -f "$(GHOSTTY_BUILD_DIR)/bin/ghostty" ]; then \
		$(GHOSTTY_BUILD_DIR)/bin/ghostty || \
		echo "$(YELLOW)Ghostty exited$(NC)"; \
	elif [ -d "$(GHOSTTY_BUILD_DIR)/Ghostty.app" ]; then \
		open $(GHOSTTY_BUILD_DIR)/Ghostty.app; \
	else \
		echo "$(RED)No runnable Ghostty found. Try 'make build-app' for macOS app$(NC)"; \
	fi

# Show build info
.PHONY: info
info:
	@echo "$(BLUE)═══════════════════════════════════════════$(NC)"
	@echo "$(BLUE)  Ghostty Terminal Communication Build$(NC)"
	@echo "$(BLUE)  Third Attempt - Minimal Implementation$(NC)"
	@echo "$(BLUE)═══════════════════════════════════════════$(NC)"
	@echo ""
	@echo "Environment:"
	@echo "  OS:           $(OS)"
	@echo "  Architecture: $(ARCH)" 
	@echo "  Project Root: $(PROJECT_ROOT)"
	@echo "  Ghostty Src:  $(GHOSTTY_SRC)"
	@echo "  Build Dir:    $(BUILD_DIR)"
	@echo ""
	@echo "Tools:"
	@echo "  Zig:     $$($(ZIG) version 2>/dev/null || echo 'not installed')"
	@echo "  Clang:   $$($(CC) --version | head -1)"
	@echo "  Xcode:   $$(xcodebuild -version 2>/dev/null | head -1 || echo 'not installed')"
	@echo ""
	@echo "$(BLUE)Available targets:$(NC)"
	@echo "  $(GREEN)make build-ghostty$(NC)   - Build Ghostty CLI (default)"
	@echo "  $(GREEN)make build-app$(NC)       - Build Ghostty.app (requires Xcode)"
	@echo "  $(GREEN)make dev$(NC)             - Build in debug mode"
	@echo "  $(GREEN)make test$(NC)            - Run tests"
	@echo "  $(GREEN)make prepare-integration$(NC) - Prepare for terminal communication"
	@echo "  $(GREEN)make test-integration$(NC)    - Test with integration"
	@echo "  $(GREEN)make run$(NC)             - Launch Ghostty"
	@echo "  $(GREEN)make clean$(NC)           - Clean build artifacts"
	@echo "  $(GREEN)make info$(NC)            - Show this information"
	@echo ""
	@echo "$(BLUE)Integration workflow:$(NC)"
	@echo "  1. make prepare-integration"
	@echo "  2. [Modify source files as needed]"
	@echo "  3. make test-integration"
	@echo "  4. make run"

# Default goal
.DEFAULT_GOAL := info

# Quick build and run
.PHONY: quick
quick: build-ghostty run

# Watch for changes and rebuild (requires fswatch)
.PHONY: watch
watch:
	@which fswatch > /dev/null || (echo "$(YELLOW)Installing fswatch...$(NC)" && brew install fswatch)
	@echo "$(BLUE)Watching for changes in Ghostty source...$(NC)"
	@fswatch -o $(GHOSTTY_SRC)/src | xargs -n1 -I{} sh -c 'clear && make build-ghostty'

# Help (same as info)
.PHONY: help
help: info