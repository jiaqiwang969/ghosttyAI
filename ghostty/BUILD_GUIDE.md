# Ghostty Build Guide

## Quick Start

```bash
# Build optimized version with code signing
make build-ghostty

# Build and run immediately
make run

# Test terminal communication features
make test-comm
```

## Available Make Targets

### `make build-ghostty`
- **Purpose**: Build Ghostty with optimizations and automatic code signing
- **What it does**:
  1. Compiles Ghostty with `-Doptimize=ReleaseFast` for best performance
  2. Automatically applies ad-hoc code signature to fix Sparkle framework issues
  3. Ready for distribution and testing
- **When to use**: Production builds, testing, or distribution

### `make build-ghostty-debug`
- **Purpose**: Build Ghostty in debug mode with symbols
- **What it does**:
  1. Compiles Ghostty without optimizations (faster build)
  2. Includes debug symbols for debugging
  3. Applies code signature to fix Sparkle framework
- **When to use**: Development and debugging

### `make run`
- **Purpose**: Build and immediately launch Ghostty
- **What it does**:
  1. Runs `make build-ghostty`
  2. Launches the built application
- **When to use**: Quick testing after code changes

### `make test-comm`
- **Purpose**: Test terminal-to-terminal communication features
- **What it does**:
  1. Builds Ghostty with optimizations
  2. Runs the test script with demo instructions
- **When to use**: Testing @send and @session commands

### `make clean`
- **Purpose**: Remove all build artifacts
- **What it does**: Deletes zig-out, zig-cache, and macOS build directories
- **When to use**: Fresh start or resolving build issues

## Manual Build Options

If you need custom build configurations:

```bash
# Standard build without code signing (will have Sparkle issues)
zig build

# Build with specific optimization level
zig build -Doptimize=ReleaseSafe    # Safe optimizations
zig build -Doptimize=ReleaseSmall   # Size optimizations
zig build -Doptimize=ReleaseFast    # Maximum performance

# Apply code signing manually after build
codesign --force --deep --sign - zig-out/Ghostty.app
```

## Troubleshooting

### Sparkle Framework Code Signing Error
**Error**: `code signature in Sparkle.framework not valid for use in process`

**Solution**: The make targets automatically fix this. If building manually, run:
```bash
codesign --force --deep --sign - zig-out/Ghostty.app
```

### Build Failures
1. Clean build artifacts: `make clean`
2. Ensure Zig 0.14.1 is installed
3. Check for sufficient disk space
4. Rebuild: `make build-ghostty`

## Terminal Communication Features

After building, test the new terminal communication capabilities:

1. Launch Ghostty: `make run`
2. Open multiple terminal tabs (Cmd+T)
3. In each terminal, type `@session` to see its ID
4. Send commands between terminals:
   ```bash
   @send surface-<ID> echo "Hello from another terminal!"
   ```

See `scripts/test_terminal_communication.sh` for detailed instructions.