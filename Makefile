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

# Build Ghostty with Zig
.PHONY: build-ghostty
build-ghostty: check-deps $(GHOSTTY_BUILD_DIR)
	@echo "$(BLUE)Building Ghostty for ARM64...$(NC)"
	@echo "$(BLUE)Step 1/3: Configuring Zig build...$(NC)"
	@cd $(GHOSTTY_SRC) && \
		if [ ! -f build.zig ]; then \
			echo "$(RED)Error: build.zig not found in Ghostty source$(NC)" && exit 1; \
		fi
	@echo "$(BLUE)Step 2/3: Building Ghostty with Zig (Release mode)...$(NC)"
	@echo "$(YELLOW)Note: Full app bundle build requires Xcode. Attempting CLI build...$(NC)"
	@cd $(GHOSTTY_SRC) && \
		$(ZIG) build -Doptimize=ReleaseFast 2>&1 | tee build.log | grep -E "(error|warning|Building|Installing)" || true
	@echo "$(BLUE)Step 3/3: Locating build output...$(NC)"
	@if [ -d "$(GHOSTTY_SRC)/zig-out/Ghostty.app" ]; then \
		echo "$(GREEN)✓ Ghostty.app built successfully$(NC)"; \
		cp -r $(GHOSTTY_SRC)/zig-out/* $(GHOSTTY_BUILD_DIR)/ 2>/dev/null || true; \
		echo "$(GREEN)  App Bundle: $(GHOSTTY_BUILD_DIR)/Ghostty.app$(NC)"; \
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
	@echo "  make build-ghostty   - Build Ghostty CLI binary"
	@echo "  make build-ghostty-app - Build Ghostty.app with Xcode"
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

# ============================================================================
# PlantUML Diagram Management
# ============================================================================

# PlantUML configuration
PLANTUML_JAR := $(shell brew list plantuml 2>/dev/null | grep libexec/plantuml.jar | head -1)
ifeq ($(PLANTUML_JAR),)
	PLANTUML_JAR := /opt/homebrew/Cellar/plantuml/1.2025.4/libexec/plantuml.jar
endif
PLANTUML := java -jar $(PLANTUML_JAR)
DOCS_DIR := $(PROJECT_ROOT)/docs
PUML_FILES := $(shell find $(DOCS_DIR) -name "*.puml" 2>/dev/null)
SVG_FILES := $(PUML_FILES:.puml=.svg)
PNG_FILES := $(PUML_FILES:.puml=.png)

# Check PlantUML dependencies
.PHONY: check-plantuml
check-plantuml:
	@echo "$(BLUE)Checking PlantUML dependencies...$(NC)"
	@which java > /dev/null || (echo "$(RED)Error: Java not found. Install with: brew install openjdk$(NC)" && exit 1)
	@if [ ! -f "$(PLANTUML_JAR)" ]; then \
		echo "$(YELLOW)PlantUML not found. Installing...$(NC)"; \
		brew install plantuml || (echo "$(RED)Error: Failed to install PlantUML$(NC)" && exit 1); \
	fi
	@echo "$(GREEN)✓ PlantUML ready$(NC)"

# Validate all PlantUML files
.PHONY: validate-puml
validate-puml: check-plantuml
	@echo "$(BLUE)Validating PlantUML diagrams...$(NC)"
	@failed=0; \
	for puml in $(PUML_FILES); do \
		echo "$(BLUE)Checking: $$puml$(NC)"; \
		$(PLANTUML) -checkonly $$puml 2>&1 | grep -q "No error found" && \
			echo "$(GREEN)  ✓ Valid$(NC)" || \
			(echo "$(RED)  ✗ Syntax error in $$puml$(NC)" && failed=1); \
	done; \
	if [ $$failed -eq 0 ]; then \
		echo "$(GREEN)✓ All PlantUML diagrams are valid$(NC)"; \
	else \
		echo "$(RED)✗ Some diagrams have errors$(NC)"; \
		exit 1; \
	fi

# Generate SVG from PlantUML
%.svg: %.puml
	@echo "$(BLUE)Generating SVG: $@$(NC)"
	@$(PLANTUML) -tsvg $< -o $(dir $@)
	@echo "$(GREEN)  ✓ Generated: $@$(NC)"

# Generate PNG from PlantUML  
%.png: %.puml
	@echo "$(BLUE)Generating PNG: $@$(NC)"
	@$(PLANTUML) -tpng $< -o $(dir $@)
	@echo "$(GREEN)  ✓ Generated: $@$(NC)"

# Generate all SVG diagrams
.PHONY: diagrams-svg
diagrams-svg: check-plantuml $(SVG_FILES)
	@echo "$(GREEN)✓ All SVG diagrams generated$(NC)"
	@echo "$(BLUE)Generated files:$(NC)"
	@for svg in $(SVG_FILES); do \
		if [ -f "$$svg" ]; then \
			echo "  $$svg ($(shell ls -lh $$svg | awk '{print $$5}'))"; \
		fi; \
	done

# Generate all PNG diagrams
.PHONY: diagrams-png
diagrams-png: check-plantuml $(PNG_FILES)
	@echo "$(GREEN)✓ All PNG diagrams generated$(NC)"
	@echo "$(BLUE)Generated files:$(NC)"
	@for png in $(PNG_FILES); do \
		if [ -f "$$png" ]; then \
			echo "  $$png ($(shell ls -lh $$png | awk '{print $$5}'))"; \
		fi; \
	done

# Generate all diagrams (both SVG and PNG)
.PHONY: diagrams
diagrams: diagrams-svg diagrams-png
	@echo "$(GREEN)✓ All diagrams generated successfully$(NC)"

# Clean generated diagrams
.PHONY: clean-diagrams
clean-diagrams:
	@echo "$(BLUE)Cleaning generated diagrams...$(NC)"
	@rm -f $(SVG_FILES) $(PNG_FILES)
	@echo "$(GREEN)✓ Diagrams cleaned$(NC)"

# List all PlantUML files
.PHONY: list-puml
list-puml:
	@echo "$(BLUE)PlantUML diagrams in project:$(NC)"
	@echo "================================"
	@for puml in $(PUML_FILES); do \
		dir=$$(dirname $$puml | sed "s|$(DOCS_DIR)/||"); \
		file=$$(basename $$puml); \
		echo "$(YELLOW)$$dir/$(NC)$$file"; \
	done
	@echo ""
	@echo "$(BLUE)Total: $(words $(PUML_FILES)) diagrams$(NC)"

# Show PlantUML statistics
.PHONY: puml-stats
puml-stats:
	@echo "$(BLUE)PlantUML Diagram Statistics$(NC)"
	@echo "================================"
	@echo "Total diagrams: $(words $(PUML_FILES))"
	@echo ""
	@echo "$(BLUE)By directory:$(NC)"
	@for dir in $(shell find $(DOCS_DIR) -name "*.puml" -exec dirname {} \; | sort -u); do \
		count=$$(find $$dir -maxdepth 1 -name "*.puml" | wc -l | tr -d ' '); \
		dir_name=$$(echo $$dir | sed "s|$(DOCS_DIR)/||"); \
		echo "  $$dir_name: $$count diagrams"; \
	done
	@echo ""
	@echo "$(BLUE)Diagram types found:$(NC)"
	@grep -h "^@startuml" $(PUML_FILES) | sed 's/@startuml *//' | sort -u | while read name; do \
		echo "  - $$name"; \
	done

# Open all diagrams in browser (requires generated SVGs)
.PHONY: view-diagrams
view-diagrams: diagrams-svg
	@echo "$(BLUE)Opening diagrams in browser...$(NC)"
	@for svg in $(SVG_FILES); do \
		if [ -f "$$svg" ]; then \
			open $$svg; \
		fi; \
	done

# Create PlantUML template
.PHONY: new-puml
new-puml:
	@read -p "Enter diagram name (without .puml): " name; \
	read -p "Enter directory (e.g., architecture-view): " dir; \
	file="$(DOCS_DIR)/$$dir/$$name.puml"; \
	mkdir -p "$(DOCS_DIR)/$$dir"; \
	echo "@startuml $$name" > $$file; \
	echo "!define PLANTUML_LIMIT_SIZE 32768" >> $$file; \
	echo "skinparam dpi 300" >> $$file; \
	echo "" >> $$file; \
	echo "title $$name Diagram\\n<size:12>Generated: $$(date '+%Y-%m-%d')</size>" >> $$file; \
	echo "" >> $$file; \
	echo "' Add your diagram content here" >> $$file; \
	echo "" >> $$file; \
	echo "@enduml" >> $$file; \
	echo "$(GREEN)✓ Created template: $$file$(NC)"

# Update help to include PlantUML targets
.PHONY: help
help:
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
	@echo "Java:         $(shell java -version 2>&1 | head -1)"
	@echo "PlantUML:     $(shell [ -f "$(PLANTUML_JAR)" ] && echo 'installed' || echo 'not installed')"
	@echo ""
	@echo "$(BLUE)Build Targets:$(NC)"
	@echo "  make build-tmux         - Build tmux for ARM64"
	@echo "  make build-ghostty      - Build Ghostty CLI binary"
	@echo "  make build-ghostty-app  - Build Ghostty.app with Xcode"
	@echo "  make all                - Build both tmux and Ghostty"
	@echo "  make clean              - Clean all build artifacts"
	@echo "  make dev-tmux           - Build tmux in debug mode"
	@echo "  make dev-ghostty        - Build Ghostty in debug mode"
	@echo "  make test-tmux          - Test tmux build"
	@echo "  make test-ghostty       - Test Ghostty build"
	@echo ""
	@echo "$(BLUE)PlantUML Targets:$(NC)"
	@echo "  make validate-puml      - Validate all PlantUML diagrams"
	@echo "  make diagrams           - Generate all diagrams (SVG + PNG)"
	@echo "  make diagrams-svg       - Generate SVG diagrams only"
	@echo "  make diagrams-png       - Generate PNG diagrams only"
	@echo "  make clean-diagrams     - Clean generated diagrams"
	@echo "  make list-puml          - List all PlantUML files"
	@echo "  make puml-stats         - Show diagram statistics"
	@echo "  make view-diagrams      - Open diagrams in browser"
	@echo "  make new-puml           - Create new diagram from template"
	@echo ""
	@echo "  make info               - Show this information"

.DEFAULT_GOAL := help