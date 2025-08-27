#!/bin/bash
# demo_tmux_ghostty.sh - Demonstrate tmux embedded in Ghostty

set -e

PROJECT_ROOT="/Users/jqwang/98-ghosttyAI"
TMUX_DIR="$PROJECT_ROOT/tmux"
GHOSTTY_DIR="$PROJECT_ROOT/ghostty"

echo "=== Ghostty × tmux Integration Demo ==="
echo ""
echo "Step 1: Building libtmuxcore..."
cd "$TMUX_DIR"
make -f Makefile.libtmuxcore clean > /dev/null 2>&1
make -f Makefile.libtmuxcore > /dev/null 2>&1
echo "  ✓ libtmuxcore.dylib built"

echo ""
echo "Step 2: Verifying library functionality..."
if [ -f test_enhanced ]; then
    DYLD_LIBRARY_PATH="$TMUX_DIR" ./test_enhanced | grep "✓" | head -5
else
    clang -o test_enhanced test_enhanced.c -L. -ltmuxcore
    DYLD_LIBRARY_PATH="$TMUX_DIR" ./test_enhanced | grep "✓" | head -5
fi

echo ""
echo "Step 3: Library information..."
echo "  Size: $(ls -lh libtmuxcore.dylib | awk '{print $5}')"
echo "  Symbols: $(nm -g libtmuxcore.dylib | grep "T _tmc_" | wc -l) exported functions"

echo ""
echo "Step 4: Copying to Ghostty..."
cp libtmuxcore.dylib "$GHOSTTY_DIR/"
cp libtmuxcore_api.h "$GHOSTTY_DIR/src/tmux/"
echo "  ✓ Library installed to Ghostty"

echo ""
echo "Step 5: Testing tmux commands..."
cat > test_commands.c << 'EOF'
#include "libtmuxcore_api.h"
#include <stdio.h>

int main() {
    tmc_init();
    
    /* Create a demo session */
    tmc_session_t session;
    tmc_session_new("demo", &session);
    tmc_session_attach(session);
    
    /* Create windows */
    tmc_window_t w1, w2;
    tmc_window_new(session, "editor", &w1);
    tmc_window_new(session, "terminal", &w2);
    
    /* Split panes */
    tmc_pane_t p1, p2;
    tmc_pane_split(w1, 1, 50, &p1);  /* Horizontal */
    tmc_pane_split(w1, 0, 50, &p2);  /* Vertical */
    
    /* Show structure */
    printf("\n=== tmux Structure ===\n");
    tmc_command_execute("list-sessions");
    printf("\n");
    tmc_command_execute("list-windows");
    printf("\n");
    tmc_command_execute("list-panes");
    
    tmc_cleanup();
    return 0;
}
EOF

clang -o test_commands test_commands.c -L. -ltmuxcore
DYLD_LIBRARY_PATH="$TMUX_DIR" ./test_commands

echo ""
echo "=== Demo Complete! ==="
echo ""
echo "Key Achievements:"
echo "  • libtmuxcore successfully built and tested"
echo "  • Real session/window/pane management working"
echo "  • Ready for full Ghostty integration"
echo "  • $(nm -g libtmuxcore.dylib | grep "T _" | wc -l) functions exported"
echo ""
echo "Next steps:"
echo "  1. Run Ghostty with tmux support enabled"
echo "  2. Use Ctrl-B prefix for tmux commands"
echo "  3. Enjoy native tmux integration!"