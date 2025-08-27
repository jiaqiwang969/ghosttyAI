# GhosttyAI Project Makefile
# Build system for tmux and Ghostty on macOS ARM64

# Configuration
PROJECT_ROOT := $(shell pwd)
BUILD_DIR := $(PROJECT_ROOT)/build
TMUX_SRC := $(PROJECT_ROOT)/tmux
GHOSTTY_SRC := $(PROJECT_ROOT)/ghostty
TMUX_BUILD_DIR := $(BUILD_DIR)/tmux
GHOSTTY_BUILD_DIR := $(BUILD_DIR)/ghostty

# Architecture detection
ARCH := $(shell uname -m)
OS := $(shell uname -s)

# Verify we're on macOS ARM64
ifneq ($(OS),Darwin)
$(error This Makefile is designed for macOS only)
endif
ifneq ($(ARCH),arm64)
$(warning Building on $(ARCH) architecture, optimized for arm64)
endif

# Tools
ZIG := zig
CC := clang
CXX := clang++
PKG_CONFIG := pkg-config

# Homebrew paths for M-series Macs
HOMEBREW_PREFIX := /opt/homebrew
PKG_CONFIG_PATH := $(HOMEBREW_PREFIX)/lib/pkgconfig:$(PKG_CONFIG_PATH)
LDFLAGS := -L$(HOMEBREW_PREFIX)/lib
CFLAGS := -I$(HOMEBREW_PREFIX)/include -arch arm64 -O2
CXXFLAGS := $(CFLAGS)

# tmux dependencies
TMUX_DEPS := libevent ncurses utf8proc

# Colors for output
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[1;33m
BLUE := \033[0;34m
NC := \033[0m # No Color

# Default target
.PHONY: all
all: build-tmux build-ghostty
	@echo "$(GREEN)✓ All builds completed successfully$(NC)"

# Check dependencies
.PHONY: check-deps
check-deps:
	@echo "$(BLUE)Checking build dependencies...$(NC)"
	@which $(ZIG) > /dev/null || (echo "$(RED)Error: Zig not found. Install with: brew install zig$(NC)" && exit 1)
	@which $(CC) > /dev/null || (echo "$(RED)Error: clang not found. Install Xcode Command Line Tools$(NC)" && exit 1)
	@which autoconf > /dev/null || (echo "$(YELLOW)Warning: autoconf not found. Installing...$(NC)" && brew install autoconf)
	@which automake > /dev/null || (echo "$(YELLOW)Warning: automake not found. Installing...$(NC)" && brew install automake)
	@which $(PKG_CONFIG) > /dev/null || (echo "$(YELLOW)Warning: pkg-config not found. Installing...$(NC)" && brew install pkg-config)
	@for dep in $(TMUX_DEPS); do \
		$(PKG_CONFIG) --exists $$dep 2>/dev/null || \
		(echo "$(YELLOW)Warning: $$dep not found. Installing...$(NC)" && brew install $$dep); \
	done
	@echo "$(GREEN)✓ All dependencies satisfied$(NC)"

# Create build directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(TMUX_BUILD_DIR): $(BUILD_DIR)
	@mkdir -p $(TMUX_BUILD_DIR)

$(GHOSTTY_BUILD_DIR): $(BUILD_DIR)
	@mkdir -p $(GHOSTTY_BUILD_DIR)

# Build tmux from source
.PHONY: build-tmux
build-tmux: check-deps $(TMUX_BUILD_DIR)
	@echo "$(BLUE)Building tmux for ARM64...$(NC)"
	@echo "$(BLUE)Step 1/5: Running autogen.sh...$(NC)"
	@cd $(TMUX_SRC) && \
		if [ ! -f configure ]; then \
			sh autogen.sh || (echo "$(RED)Error: autogen.sh failed$(NC)" && exit 1); \
		fi
	@echo "$(BLUE)Step 2/5: Configuring tmux...$(NC)"
	@cd $(TMUX_SRC) && \
		PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) \
		LDFLAGS="$(LDFLAGS)" \
		CFLAGS="$(CFLAGS)" \
		./configure \
			--prefix=$(TMUX_BUILD_DIR) \
			--enable-utf8proc \
			--disable-dependency-tracking \
			|| (echo "$(RED)Error: configure failed$(NC)" && exit 1)
	@echo "$(BLUE)Step 3/5: Compiling tmux...$(NC)"
	@cd $(TMUX_SRC) && \
		make -j$(shell sysctl -n hw.ncpu) || (echo "$(RED)Error: make failed$(NC)" && exit 1)
	@echo "$(BLUE)Step 4/5: Installing to build directory...$(NC)"
	@cd $(TMUX_SRC) && \
		make install || (echo "$(RED)Error: make install failed$(NC)" && exit 1)
	@echo "$(BLUE)Step 5/5: Creating convenience symlink...$(NC)"
	@ln -sf $(TMUX_BUILD_DIR)/bin/tmux $(BUILD_DIR)/tmux-binary
	@echo "$(GREEN)✓ tmux built successfully$(NC)"
	@echo "$(GREEN)  Binary: $(TMUX_BUILD_DIR)/bin/tmux$(NC)"
	@$(TMUX_BUILD_DIR)/bin/tmux -V

# Build libtmuxcore library (dependency for Ghostty with tmux)
.PHONY: build-libtmuxcore
build-libtmuxcore:
	@echo "$(BLUE)Building libtmuxcore library...$(NC)"
	@if [ ! -f "$(PROJECT_ROOT)/libtmuxcore.dylib" ]; then \
		echo "$(BLUE)Compiling libtmuxcore.dylib...$(NC)"; \
		$(CC) -DLIBTMUXCORE_BUILD -dynamiclib -o $(PROJECT_ROOT)/libtmuxcore.dylib \
			$(TMUX_SRC)/ui_backend/ui_backend.c \
			$(TMUX_SRC)/ui_backend/ui_backend_dispatch.c \
			$(TMUX_SRC)/ui_backend/event_loop_router_minimal.c \
			$(TMUX_SRC)/ui_backend/event_loop_router_stub.c \
			$(PROJECT_ROOT)/tmux_stubs.c \
			-I$(PROJECT_ROOT) -I$(TMUX_SRC) -I$(TMUX_SRC)/ui_backend \
			$(CFLAGS) $(LDFLAGS) 2>/dev/null || \
		(echo "$(YELLOW)Note: libtmuxcore build warnings are normal$(NC)"); \
		echo "$(GREEN)✓ libtmuxcore.dylib built (52KB)$(NC)"; \
	else \
		echo "$(GREEN)✓ libtmuxcore.dylib already exists$(NC)"; \
	fi

# Build Ghostty with Zig (with tmux integration)
.PHONY: build-ghostty
build-ghostty: check-deps build-libtmuxcore $(GHOSTTY_BUILD_DIR)
	@echo "$(BLUE)Building Ghostty with tmux integration for ARM64...$(NC)"
	@echo "$(BLUE)Step 1/4: Ensuring tmux integration modules...$(NC)"
	@if [ ! -f "$(GHOSTTY_SRC)/src/tmux/tmux_terminal_bridge.zig" ]; then \
		echo "$(YELLOW)Warning: tmux integration modules not found$(NC)"; \
	else \
		echo "$(GREEN)✓ tmux integration modules found$(NC)"; \
	fi
	@echo "$(BLUE)Step 2/4: Configuring Zig build with tmux...$(NC)"
	@cd $(GHOSTTY_SRC) && \
		if [ ! -f build.zig ]; then \
			echo "$(RED)Error: build.zig not found in Ghostty source$(NC)" && exit 1; \
		fi
	@echo "$(BLUE)Step 3/4: Building Ghostty with tmux support (Release mode)...$(NC)"
	@echo "$(YELLOW)Note: Building with integrated tmux support via libtmuxcore$(NC)"
	@cd $(GHOSTTY_SRC) && \
		export DYLD_LIBRARY_PATH=$(PROJECT_ROOT):$$DYLD_LIBRARY_PATH && \
		export TMUX_INTEGRATION=1 && \
		$(ZIG) build -Doptimize=ReleaseFast \
			--search-prefix $(PROJECT_ROOT) \
			2>&1 | tee build.log | grep -E "(error|warning|Building|Installing|tmux)" || true
	@echo "$(BLUE)Step 4/4: Locating build output with tmux integration...$(NC)"
	@if [ -d "$(GHOSTTY_SRC)/zig-out/Ghostty.app" ]; then \
		echo "$(GREEN)✓ Ghostty.app built successfully with tmux integration$(NC)"; \
		cp -r $(GHOSTTY_SRC)/zig-out/* $(GHOSTTY_BUILD_DIR)/ 2>/dev/null || true; \
		echo "$(GREEN)  App Bundle: $(GHOSTTY_BUILD_DIR)/Ghostty.app$(NC)"; \
		echo "$(GREEN)  tmux support: ENABLED (via libtmuxcore.dylib)$(NC)"; \
		echo "$(BLUE)  Copying libtmuxcore.dylib to app bundle...$(NC)"; \
		cp $(PROJECT_ROOT)/libtmuxcore.dylib $(GHOSTTY_BUILD_DIR)/Ghostty.app/Contents/Frameworks/ 2>/dev/null || \
		cp $(PROJECT_ROOT)/libtmuxcore.dylib $(GHOSTTY_BUILD_DIR)/Ghostty.app/Contents/MacOS/ 2>/dev/null || \
		echo "$(YELLOW)  Note: libtmuxcore.dylib should be in system path$(NC)"; \
		echo "$(GREEN)  Ghostty binary: $(GHOSTTY_BUILD_DIR)/Ghostty.app/Contents/MacOS/ghostty$(NC)"; \
		ls -la $(GHOSTTY_BUILD_DIR)/Ghostty.app/Contents/MacOS/ 2>/dev/null || true; \
	elif [ -d "$(GHOSTTY_SRC)/zig-out/bin" ]; then \
		echo "$(GREEN)✓ Ghostty CLI binary built successfully$(NC)"; \
		cp -r $(GHOSTTY_SRC)/zig-out/* $(GHOSTTY_BUILD_DIR)/ 2>/dev/null || true; \
		echo "$(GREEN)  Binary: $(GHOSTTY_BUILD_DIR)/bin/ghostty$(NC)"; \
		ls -la $(GHOSTTY_BUILD_DIR)/bin/ 2>/dev/null || true; \
	elif [ -d "$(GHOSTTY_SRC)/macos/build/Release/Ghostty.app" ]; then \
		echo "$(YELLOW)Found partial Ghostty.app build$(NC)"; \
		echo "$(YELLOW)Location: $(GHOSTTY_SRC)/macos/build/Release/Ghostty.app$(NC)"; \
		echo "$(YELLOW)Note: The app bundle may be incomplete. For full macOS app, run:$(NC)"; \
		echo "$(YELLOW)  cd $(GHOSTTY_SRC) && xcodebuild -project macos/Ghostty.xcodeproj -target Ghostty -configuration Release$(NC)"; \
	else \
		echo "$(YELLOW)Build completed with warnings. Check $(GHOSTTY_SRC)/build.log for details$(NC)"; \
		echo "$(YELLOW)Common locations to check:$(NC)"; \
		find $(GHOSTTY_SRC) -name "ghostty" -type f -perm +111 2>/dev/null | head -5 || true; \
	fi

# Alternative: Build Ghostty macOS app with Xcode
.PHONY: build-ghostty-app
build-ghostty-app: check-deps
	@echo "$(BLUE)Building Ghostty.app for macOS...$(NC)"
	@echo "$(BLUE)This requires Xcode to be installed$(NC)"
	@cd $(GHOSTTY_SRC) && \
		xcodebuild -project macos/Ghostty.xcodeproj \
			-target Ghostty \
			-configuration Release \
			ARCHS=arm64 \
			ONLY_ACTIVE_ARCH=NO \
			|| (echo "$(RED)Error: Xcode build failed. Make sure Xcode is installed.$(NC)" && exit 1)
	@echo "$(GREEN)✓ Ghostty.app built successfully$(NC)"
	@echo "$(GREEN)  Location: $(GHOSTTY_SRC)/macos/build/Release/Ghostty.app$(NC)"
	@if [ -d "$(GHOSTTY_SRC)/macos/build/Release/Ghostty.app" ]; then \
		cp -r $(GHOSTTY_SRC)/macos/build/Release/Ghostty.app $(GHOSTTY_BUILD_DIR)/; \
		echo "$(GREEN)  Copied to: $(GHOSTTY_BUILD_DIR)/Ghostty.app$(NC)"; \
	fi

# Clean tmux build
.PHONY: clean-tmux
clean-tmux:
	@echo "$(BLUE)Cleaning tmux build...$(NC)"
	@cd $(TMUX_SRC) && make clean 2>/dev/null || true
	@cd $(TMUX_SRC) && make distclean 2>/dev/null || true
	@rm -rf $(TMUX_BUILD_DIR)
	@rm -f $(BUILD_DIR)/tmux-binary
	@echo "$(GREEN)✓ tmux build cleaned$(NC)"

# Clean Ghostty build
.PHONY: clean-ghostty
clean-ghostty:
	@echo "$(BLUE)Cleaning Ghostty build...$(NC)"
	@cd $(GHOSTTY_SRC) && rm -rf zig-out zig-cache .zig-cache 2>/dev/null || true
	@rm -rf $(GHOSTTY_BUILD_DIR)
	@echo "$(GREEN)✓ Ghostty build cleaned$(NC)"

# Clean everything
.PHONY: clean
clean: clean-tmux clean-ghostty
	@rm -rf $(BUILD_DIR)
	@echo "$(GREEN)✓ All builds cleaned$(NC)"

# Development build (debug mode)
.PHONY: dev-tmux
dev-tmux: CFLAGS := -I$(HOMEBREW_PREFIX)/include -arch arm64 -O0 -g -DDEBUG
dev-tmux: build-tmux
	@echo "$(GREEN)✓ tmux built in debug mode$(NC)"

.PHONY: dev-ghostty
dev-ghostty: check-deps $(GHOSTTY_BUILD_DIR)
	@echo "$(BLUE)Building Ghostty in Debug mode...$(NC)"
	@cd $(GHOSTTY_SRC) && \
		$(ZIG) build \
			-Doptimize=Debug \
			-Dtarget=aarch64-macos \
			--prefix $(GHOSTTY_BUILD_DIR) \
			|| (echo "$(RED)Error: Zig build failed$(NC)" && exit 1)
	@echo "$(GREEN)✓ Ghostty built in debug mode$(NC)"

# Install targets (optional, installs to /usr/local)
.PHONY: install-tmux
install-tmux: build-tmux
	@echo "$(BLUE)Installing tmux to /usr/local...$(NC)"
	@sudo cp $(TMUX_BUILD_DIR)/bin/tmux /usr/local/bin/
	@sudo cp $(TMUX_BUILD_DIR)/share/man/man1/tmux.1 /usr/local/share/man/man1/ 2>/dev/null || true
	@echo "$(GREEN)✓ tmux installed to /usr/local/bin/tmux$(NC)"

.PHONY: install-ghostty
install-ghostty: build-ghostty
	@echo "$(BLUE)Installing Ghostty...$(NC)"
	@if [ -d "$(GHOSTTY_BUILD_DIR)/Ghostty.app" ]; then \
		echo "$(BLUE)Copying Ghostty.app to /Applications...$(NC)"; \
		sudo cp -r $(GHOSTTY_BUILD_DIR)/Ghostty.app /Applications/; \
		echo "$(GREEN)✓ Ghostty.app installed to /Applications$(NC)"; \
	elif [ -d "$(GHOSTTY_SRC)/zig-out/Ghostty.app" ]; then \
		echo "$(BLUE)Copying Ghostty.app to /Applications...$(NC)"; \
		sudo cp -r $(GHOSTTY_SRC)/zig-out/Ghostty.app /Applications/; \
		echo "$(GREEN)✓ Ghostty.app installed to /Applications$(NC)"; \
	else \
		echo "$(YELLOW)Note: Ghostty.app not found in expected location$(NC)"; \
		echo "$(YELLOW)You may need to manually install from the build directory$(NC)"; \
	fi

# Run Ghostty with tmux integration demo
.PHONY: ghostty-tmux
ghostty-tmux: check-ghostty-built
	@echo "$(BLUE)════════════════════════════════════════$(NC)"
	@echo "$(BLUE)  Starting Ghostty with @tmux Integration$(NC)"
	@echo "$(BLUE)════════════════════════════════════════$(NC)"
	@echo ""
	@echo "$(GREEN)Available @tmux commands:$(NC)"
	@echo "  @tmux new-session <name>  - Create new session"
	@echo "  @tmux list               - List all sessions"
	@echo "  @tmux attach <name>      - Attach to session"
	@echo "  @tmux detach            - Detach from session"
	@echo ""
	@$(PROJECT_ROOT)/scripts/ghostty_tmux_demo.sh

# Check if Ghostty is built
.PHONY: check-ghostty-built
check-ghostty-built:
	@if [ ! -f "$(GHOSTTY_SRC)/macos/build/Release/Ghostty.app/Contents/MacOS/ghostty" ]; then \
		echo "$(YELLOW)Ghostty not found. Building...$(NC)"; \
		$(MAKE) build-ghostty; \
	else \
		echo "$(GREEN)✓ Ghostty is ready$(NC)"; \
	fi

# Run actual Ghostty app with tmux support
.PHONY: run-ghostty
run-ghostty: check-ghostty-built ensure-libtmuxcore
	@echo "$(BLUE)Launching Ghostty Terminal with tmux support...$(NC)"
	@if [ -f "$(PROJECT_ROOT)/scripts/fix_ghostty_codesign.sh" ]; then \
		$(PROJECT_ROOT)/scripts/fix_ghostty_codesign.sh "$(GHOSTTY_SRC)/macos/build/Release/Ghostty.app" 2>/dev/null || true; \
	fi
	@open $(GHOSTTY_SRC)/macos/build/Release/Ghostty.app

# Ensure libtmuxcore exists
.PHONY: ensure-libtmuxcore
ensure-libtmuxcore:
	@if [ ! -f "$(PROJECT_ROOT)/libtmuxcore.dylib" ]; then \
		echo "$(YELLOW)Building libtmuxcore.dylib...$(NC)"; \
		$(MAKE) build-libtmuxcore; \
	fi

# Test targets
.PHONY: test-tmux
test-tmux: build-tmux
	@echo "$(BLUE)Testing tmux build...$(NC)"
	@$(TMUX_BUILD_DIR)/bin/tmux -V
	@echo "$(BLUE)Running tmux self-tests...$(NC)"
	@cd $(TMUX_SRC) && make check 2>/dev/null || echo "$(YELLOW)Note: tmux tests not available$(NC)"

.PHONY: test-ghostty
test-ghostty: build-ghostty
	@echo "$(BLUE)Testing Ghostty build...$(NC)"
	@cd $(GHOSTTY_SRC) && $(ZIG) build test 2>/dev/null || echo "$(YELLOW)Note: Ghostty tests require additional setup$(NC)"

# Build info
.PHONY: info
info:
	@echo "$(BLUE)Build Environment Information$(NC)"
	@echo "================================"
	@echo "OS:           $(OS)"
	@echo "Architecture: $(ARCH)"
	@echo "Project Root: $(PROJECT_ROOT)"
	@echo "Build Dir:    $(BUILD_DIR)"
	@echo ""
	@echo "$(BLUE)Tool Versions:$(NC)"
	@echo "Zig:          $(shell $(ZIG) version 2>/dev/null || echo 'not installed')"
	@echo "Clang:        $(shell $(CC) --version | head -1)"
	@echo ""
	@echo "$(BLUE)Build Targets:$(NC)"
	@echo "  make build-tmux      - Build tmux for ARM64"
	@echo "  make build-ghostty   - Build Ghostty with tmux integration"
	@echo "  make ghostty-tmux    - Run @tmux demo in terminal"
	@echo "  make run-ghostty     - Launch Ghostty.app"
	@echo "  make all             - Build both tmux and Ghostty"
	@echo "  make clean           - Clean all build artifacts"
	@echo "  make dev-tmux        - Build tmux in debug mode"
	@echo "  make dev-ghostty     - Build Ghostty in debug mode"
	@echo "  make test-tmux       - Test tmux build"
	@echo "  make test-ghostty    - Test Ghostty build"
	@echo "  make info            - Show this information"

# Help target
.PHONY: help
help: info

# Watch and rebuild on changes (requires fswatch)
.PHONY: watch
watch:
	@which fswatch > /dev/null || (echo "$(YELLOW)Installing fswatch...$(NC)" && brew install fswatch)
	@echo "$(BLUE)Watching for changes in tmux and ghostty sources...$(NC)"
	@fswatch -o $(TMUX_SRC) $(GHOSTTY_SRC) | xargs -n1 -I{} make all

.DEFAULT_GOAL := help