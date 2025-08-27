#!/bin/bash

echo "Launching Ghostty with overlay-based command system..."
echo
echo "Commands to test:"
echo "  @ghostty session     - Show session ID"
echo "  @ghostty send <id> <cmd> - Send command to session"
echo "  @ghostty            - Show help"
echo
echo "Starting Ghostty..."

# Launch the compiled Ghostty
open /Users/jqwang/98-ghosttyAI/ghostty/zig-out/Ghostty.app

echo
echo "Ghostty launched! Try the commands in the terminal window."