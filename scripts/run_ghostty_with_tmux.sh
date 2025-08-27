#!/bin/bash
# run_ghostty_with_tmux.sh - Launch Ghostty with tmux integration
# Purpose: Run Ghostty with proper environment for tmux support
# Date: 2025-08-26

set -e

# Set paths
PROJECT_ROOT="/Users/jqwang/98-ghosttyAI"
GHOSTTY_APP="$PROJECT_ROOT/ghostty/macos/build/Release/Ghostty.app"
GHOSTTY_BIN="$GHOSTTY_APP/Contents/MacOS/ghostty"
LIBTMUXCORE="$PROJECT_ROOT/libtmuxcore.dylib"

# Check if Ghostty exists
if [ ! -f "$GHOSTTY_BIN" ]; then
    echo "Error: Ghostty not found at $GHOSTTY_BIN"
    echo "Please run: make build-ghostty"
    exit 1
fi

# Check if libtmuxcore exists
if [ ! -f "$LIBTMUXCORE" ]; then
    echo "Building libtmuxcore.dylib..."
    cd "$PROJECT_ROOT"
    make build-libtmuxcore
fi

# Copy libtmuxcore to Ghostty app if not already there
if [ ! -f "$GHOSTTY_APP/Contents/MacOS/libtmuxcore.dylib" ]; then
    echo "Copying libtmuxcore.dylib to Ghostty app..."
    cp "$LIBTMUXCORE" "$GHOSTTY_APP/Contents/MacOS/"
fi

# Set environment for tmux integration
export DYLD_LIBRARY_PATH="$PROJECT_ROOT:$GHOSTTY_APP/Contents/MacOS:$DYLD_LIBRARY_PATH"
export TMUX_INTEGRATION=1
export GHOSTTY_TMUX_ENABLED=1

# Fix code signing if needed
if ! "$GHOSTTY_BIN" --version >/dev/null 2>&1; then
    echo "Fixing code signing..."
    "$PROJECT_ROOT/fix_ghostty_codesign.sh" "$GHOSTTY_APP"
fi

echo "======================================="
echo "  Launching Ghostty with tmux Support"
echo "======================================="
echo ""
echo "tmux integration: ENABLED"
echo "libtmuxcore: $LIBTMUXCORE (52KB)"
echo ""
echo "Available tmux commands in Ghostty:"
echo "  @tmux new-session <name>    - Create new session"
echo "  @tmux list-sessions         - List all sessions"
echo "  @tmux attach <name>         - Attach to session"
echo "  @tmux detach               - Detach from session"
echo ""
echo "Starting Ghostty..."
echo "======================================="

# Launch Ghostty
exec "$GHOSTTY_BIN" "$@"