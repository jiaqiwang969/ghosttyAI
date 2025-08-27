init:
	@echo You probably want to run "zig build" instead.
.PHONY: init

# Build Ghostty with automatic code signing to fix Sparkle framework issue
build-ghostty:
	@echo "Building Ghostty with optimizations..."
	zig build -Doptimize=ReleaseFast
	@echo "Applying code signature to fix Sparkle framework..."
	codesign --force --deep --sign - zig-out/Ghostty.app
	@echo "✅ Ghostty built and signed successfully!"
.PHONY: build-ghostty

# Development build (faster, with debug symbols)
build-ghostty-debug:
	@echo "Building Ghostty in debug mode..."
	zig build
	@echo "Applying code signature to fix Sparkle framework..."
	codesign --force --deep --sign - zig-out/Ghostty.app
	@echo "✅ Ghostty debug build completed and signed!"
.PHONY: build-ghostty-debug

# Build and run Ghostty
run: build-ghostty
	@echo "Launching Ghostty..."
	zig-out/Ghostty.app/Contents/MacOS/Ghostty
.PHONY: run

# Test terminal communication feature
test-comm: build-ghostty
	@echo "Launching terminal communication test..."
	@if [ -f scripts/test_terminal_communication.sh ]; then \
		bash scripts/test_terminal_communication.sh; \
	else \
		echo "Test script not found. Running Ghostty directly..."; \
		zig-out/Ghostty.app/Contents/MacOS/Ghostty; \
	fi
.PHONY: test-comm

# glad updates the GLAD loader. To use this, place the generated glad.zip
# in this directory next to the Makefile, remove vendor/glad and run this target.
#
# Generator: https://gen.glad.sh/
glad: vendor/glad
.PHONY: glad

vendor/glad: vendor/glad/include/glad/gl.h vendor/glad/include/glad/glad.h

vendor/glad/include/glad/gl.h: glad.zip
	rm -rf vendor/glad
	mkdir -p vendor/glad
	unzip glad.zip -dvendor/glad
	find vendor/glad -type f -exec touch '{}' +

vendor/glad/include/glad/glad.h: vendor/glad/include/glad/gl.h
	@echo "#include <glad/gl.h>" > $@

clean:
	rm -rf \
		zig-out zig-cache \
		macos/build \
		macos/GhosttyKit.xcframework
.PHONY: clean
