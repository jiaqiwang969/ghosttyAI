# PlantUML to SVG Converter
# =========================

# Configuration
PLANTUML ?= plantuml
PLANTUML_OPTS = -charset UTF-8 -tsvg -quiet

# Colors for terminal output
RED = \033[0;31m
GREEN = \033[0;32m
YELLOW = \033[1;33m
BLUE = \033[0;34m
NC = \033[0m # No Color

# Find all PlantUML files
PUML_FILES = $(wildcard *.puml)
SVG_FILES = $(PUML_FILES:.puml=.svg)

# Default target
.DEFAULT_GOAL := all

# Check PlantUML installation
check:
	@if ! command -v $(PLANTUML) >/dev/null 2>&1; then \
		echo "$(RED)❌ PlantUML not found$(NC)"; \
		echo "$(YELLOW)Install with: brew install plantuml$(NC)"; \
		exit 1; \
	fi
	@echo "$(GREEN)✓ PlantUML ready$(NC)"

# Convert all diagrams
all: check $(SVG_FILES)
	@echo "$(GREEN)✅ All $(words $(SVG_FILES)) diagrams converted!$(NC)"

# Pattern rule for .puml to .svg conversion
%.svg: %.puml
	@echo "$(BLUE)⚙️  Converting $< → $@$(NC)"
	@$(PLANTUML) $(PLANTUML_OPTS) $< || (echo "$(RED)❌ Failed: $<$(NC)" && exit 1)
	@echo "$(GREEN)  ✓ Done$(NC)"

# Quick refresh (force rebuild)
refresh:
	@echo "$(YELLOW)🔄 Refreshing all diagrams...$(NC)"
	@touch *.puml 2>/dev/null || true
	@$(MAKE) --no-print-directory all

# Clean all generated SVGs
clean:
	@echo "$(YELLOW)🧹 Cleaning SVG files...$(NC)"
	@rm -f *.svg
	@echo "$(GREEN)✓ Cleaned$(NC)"

# Force rebuild (clean + build)
force: clean all

# Watch for changes (requires fswatch)
watch:
	@if ! command -v fswatch >/dev/null 2>&1; then \
		echo "$(RED)❌ fswatch not found$(NC)"; \
		echo "$(YELLOW)Install with: brew install fswatch$(NC)"; \
		exit 1; \
	fi
	@echo "$(YELLOW)👁  Watching for changes... (Ctrl+C to stop)$(NC)"
	@fswatch -o *.puml | while read; do \
		clear; \
		$(MAKE) --no-print-directory all; \
	done

# Open all SVGs in browser
open: all
	@echo "$(YELLOW)🌐 Opening SVGs...$(NC)"
	@for svg in $(SVG_FILES); do \
		open $$svg 2>/dev/null || echo "$(RED)Cannot open $$svg$(NC)"; \
	done

# Show file statistics
stats:
	@echo "$(BLUE)📊 Diagram Statistics$(NC)"
	@echo "────────────────────"
	@echo "PlantUML files: $(words $(PUML_FILES))"
	@echo "SVG files:      $(words $(wildcard *.svg))"
	@echo ""
	@if [ -n "$(PUML_FILES)" ]; then \
		echo "$(BLUE)Source files:$(NC)"; \
		for file in $(PUML_FILES); do \
			lines=$$(wc -l < $$file | tr -d ' '); \
			printf "  • %-30s %4d lines\n" $$file $$lines; \
		done; \
	fi

# List all files
list:
	@echo "$(BLUE)📁 Files:$(NC)"
	@echo "PlantUML: $(PUML_FILES)"
	@echo "SVG:      $(SVG_FILES)"

# Install PlantUML (macOS)
install:
	@echo "$(YELLOW)📦 Installing PlantUML...$(NC)"
	@brew install plantuml || echo "$(RED)Installation failed$(NC)"

# Help message
help:
	@echo "$(BLUE)PlantUML → SVG Makefile$(NC)"
	@echo "═══════════════════════"
	@echo ""
	@echo "$(GREEN)Usage:$(NC)"
	@echo "  make              - Convert all .puml to .svg"
	@echo "  make refresh      - Force refresh all SVGs"
	@echo "  make clean        - Remove all SVG files"
	@echo "  make force        - Clean and rebuild"
	@echo ""
	@echo "$(GREEN)Tools:$(NC)"
	@echo "  make watch        - Auto-rebuild on changes"
	@echo "  make open         - Open SVGs in browser"
	@echo "  make stats        - Show statistics"
	@echo "  make list         - List all files"
	@echo "  make install      - Install PlantUML"
	@echo "  make help         - Show this help"
	@echo ""
	@echo "$(GREEN)Single file:$(NC)"
	@echo "  make <name>.svg   - Convert specific file"

# Declare phony targets
.PHONY: all check refresh clean force watch open stats list install help

# Silent execution by default
.SILENT: