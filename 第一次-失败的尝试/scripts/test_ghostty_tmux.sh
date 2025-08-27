#!/bin/bash
# test_ghostty_tmux.sh - Test Ghostty with tmux integration

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║     Testing Ghostty with tmux Integration                   ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

GHOSTTY_APP="/Users/jqwang/98-ghosttyAI/build/ghostty/Ghostty.app"
GHOSTTY_BIN="$GHOSTTY_APP/Contents/MacOS/ghostty"

# Check if Ghostty binary exists
if [ ! -f "$GHOSTTY_BIN" ]; then
    echo "❌ Ghostty binary not found at: $GHOSTTY_BIN"
    echo "   Please run: make build-ghostty"
    exit 1
fi

echo "✅ Found Ghostty binary"

# Check if libtmuxcore is linked
echo ""
echo "📍 Checking tmux integration..."
if otool -L "$GHOSTTY_BIN" 2>/dev/null | grep -q "libtmuxcore"; then
    echo "✅ libtmuxcore is linked"
else
    echo "⚠️  libtmuxcore not directly linked (may be loaded dynamically)"
fi

# Check if libtmuxcore.dylib exists in Frameworks
LIBTMUX="$GHOSTTY_APP/Contents/Frameworks/libtmuxcore.dylib"
if [ -f "$LIBTMUX" ]; then
    echo "✅ libtmuxcore.dylib found in app bundle"
else
    echo "❌ libtmuxcore.dylib not found in app bundle"
fi

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "                  Running Ghostty Terminal Test                  "
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Run in terminal mode to test commands
echo "Starting Ghostty in terminal mode for testing..."
echo ""

# Create test script
cat > /tmp/ghostty_tmux_test.sh << 'EOF'
#!/bin/bash
echo "Testing tmux integration in Ghostty..."
echo ""
echo "1. Testing @tmux command recognition:"
echo "   Command: @tmux version"
@tmux version 2>/dev/null || echo "   [Not working - @tmux commands not recognized]"

echo ""
echo "2. Testing Ctrl-B prefix key:"
echo "   Press Ctrl-B then ? for help (if tmux is active)"
echo ""

echo "3. Available tmux-like commands:"
echo "   Ctrl-B c    - Create new window"
echo "   Ctrl-B \"    - Split horizontally"
echo "   Ctrl-B %    - Split vertically"
echo "   Ctrl-B n    - Next window"
echo "   Ctrl-B p    - Previous window"
echo ""

echo "Testing complete. Press Ctrl-C to exit."
EOF

chmod +x /tmp/ghostty_tmux_test.sh

# Set environment variables for tmux
export GHOSTTY_TMUX_ENABLED=1
export DYLD_LIBRARY_PATH="$GHOSTTY_APP/Contents/Frameworks:$DYLD_LIBRARY_PATH"

# Run Ghostty with the test script
echo "Launching Ghostty with test script..."
"$GHOSTTY_BIN" --command "/tmp/ghostty_tmux_test.sh" 2>&1 &

GHOSTTY_PID=$!
echo "Ghostty launched with PID: $GHOSTTY_PID"

# Wait a moment for it to start
sleep 2

# Check if it's running
if ps -p $GHOSTTY_PID > /dev/null; then
    echo "✅ Ghostty is running"
    echo ""
    echo "Try these commands in the Ghostty window:"
    echo "  1. Type: @tmux new-session -s test"
    echo "  2. Press: Ctrl-B then c (new window)"
    echo "  3. Press: Ctrl-B then % (split vertical)"
    echo ""
    echo "Press Enter to kill Ghostty and continue..."
    read
    kill $GHOSTTY_PID 2>/dev/null
else
    echo "❌ Ghostty exited immediately (may have crashed)"
fi

echo ""
echo "Test complete!"