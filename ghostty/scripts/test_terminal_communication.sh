#!/bin/bash
# Test script for Ghostty terminal-to-terminal communication
# This demonstrates the @send command functionality

set -e

echo "======================================"
echo "Ghostty Terminal Communication Demo"
echo "======================================"
echo ""
echo "This demo will:"
echo "1. Launch Ghostty application"
echo "2. Open two terminal windows"
echo "3. Send commands between terminals"
echo ""

# Path to the compiled Ghostty app
GHOSTTY_APP="/Users/jqwang/98-ghosttyAI/ghostty/zig-out/Ghostty.app/Contents/MacOS/Ghostty"

if [ ! -f "$GHOSTTY_APP" ]; then
    echo "Error: Ghostty not found at $GHOSTTY_APP"
    echo "Please run: zig build -Doptimize=ReleaseFast"
    exit 1
fi

echo "Starting Ghostty..."
"$GHOSTTY_APP" &
GHOSTTY_PID=$!

# Give Ghostty time to start
sleep 3

# Instructions for manual testing
cat << 'EOF'

==== MANUAL TESTING INSTRUCTIONS ====

Once Ghostty opens:

1. Open at least 2 terminal windows/tabs in Ghostty
   - Use Cmd+T or File -> New Tab

2. In each terminal, type (or paste):
   @session
   
   This will display the session ID for that terminal.
   You'll see something like: "Session ID: surface-1054d6000"

3. In Terminal 2, send a command to Terminal 1:
   @send surface-<ID> echo "Hello from Terminal 2!"
   
   Replace <ID> with the actual session ID from Terminal 1

4. You should see "Hello from Terminal 2!" executed in Terminal 1

5. Test various commands:
   @send surface-<ID> ls
   @send surface-<ID> pwd
   @send surface-<ID> date

==== ADVANCED TESTING ====

Try these scenarios:
- Send commands from Terminal 1 to Terminal 2 (reverse direction)
- Open 3+ terminals and send commands between them
- Send multi-word commands: @send surface-<ID> echo "Multi word test"

==== TROUBLESHOOTING ====

If @send commands are not working:
1. Check Ghostty logs for "Intercepted @send command" messages
2. Verify session IDs are correct
3. Ensure both terminals are in the same Ghostty instance

To see debug logs, run Ghostty with:
RUST_LOG=debug "$GHOSTTY_APP"

Press Ctrl+C to stop the demo.

EOF

# Wait for user to test
wait $GHOSTTY_PID