#!/bin/bash
# apply_tmux_deep_integration.sh - Apply deep tmux integration to Ghostty
# Purpose: Modify Termio.zig to enable @tmux commands
# Date: 2025-08-26

set -e

echo "======================================="
echo "  🚀 Applying Deep tmux Integration"
echo "======================================="
echo ""

TERMIO_FILE="/Users/jqwang/98-ghosttyAI/ghostty/src/termio/Termio.zig"
BACKUP_FILE="/Users/jqwang/98-ghosttyAI/ghostty/src/termio/Termio.zig.backup_tmux"

# Step 1: Backup original
if [ ! -f "$BACKUP_FILE" ]; then
    echo "1️⃣ Backing up Termio.zig..."
    cp "$TERMIO_FILE" "$BACKUP_FILE"
    echo "✅ Backup created"
else
    echo "✅ Backup already exists"
fi

# Step 2: Check if already patched
if grep -q "tmux_integration" "$TERMIO_FILE" 2>/dev/null; then
    echo "✅ tmux integration already applied"
    exit 0
fi

# Step 3: Apply the patch
echo ""
echo "2️⃣ Adding tmux integration to Termio.zig..."

# Add import after line 29
sed -i '' '29a\
\
// tmux integration (added for deep integration)\
const tmux = @import("../tmux/tmux_integration.zig");\
' "$TERMIO_FILE"

# Add field to struct after line 75
sed -i '' '75a\
\
/// tmux integration handle (optional)\
tmux_handle: ?*tmux.TmuxHandle = null,\
' "$TERMIO_FILE"

# Find the init function and add tmux initialization
# This is more complex, so we'll use a temporary file
cat > /tmp/tmux_init_patch.txt << 'EOF'

    // Initialize tmux integration (deep integration)
    {
        log.info("Initializing deep tmux integration...", .{});
        self.tmux_handle = try alloc.create(tmux.TmuxHandle);
        self.tmux_handle.* = try tmux.TmuxHandle.init(alloc, &self.terminal);
        try self.tmux_handle.setupCallbacks();
        log.info("tmux deep integration enabled - @tmux commands available", .{});
    }
EOF

# Find a good place to insert (after terminal creation)
# Look for "terminal =" and add after it
LINE_NUM=$(grep -n "self.terminal =" "$TERMIO_FILE" | head -1 | cut -d: -f1)
if [ -n "$LINE_NUM" ]; then
    # Insert after 10 lines from terminal creation
    INSERT_LINE=$((LINE_NUM + 10))
    sed -i '' "${INSERT_LINE}r /tmp/tmux_init_patch.txt" "$TERMIO_FILE"
    echo "✅ Added tmux initialization"
else
    echo "⚠️  Could not find terminal initialization, manual edit needed"
fi

# Step 4: Add command handler
echo ""
echo "3️⃣ Adding @tmux command handler..."

cat > /tmp/tmux_handler.txt << 'EOF'

/// Handle tmux commands (deep integration)
pub fn handleTmuxCommand(self: *Termio, input: []const u8) bool {
    // Check for @tmux prefix
    if (std.mem.startsWith(u8, input, "@tmux ")) {
        const cmd = input[6..];
        
        if (self.tmux_handle) |handle| {
            handle.executeCommand(cmd) catch |err| {
                log.err("tmux command failed: {}", .{err});
                return false;
            };
            return true;
        }
    }
    return false;
}
EOF

# Add the handler function
echo "" >> "$TERMIO_FILE"
cat /tmp/tmux_handler.txt >> "$TERMIO_FILE"

echo "✅ Added command handler"

# Step 5: Add cleanup to deinit
echo ""
echo "4️⃣ Adding cleanup code..."

# Find deinit function
if grep -q "pub fn deinit" "$TERMIO_FILE"; then
    # Add cleanup code
    LINE_NUM=$(grep -n "pub fn deinit" "$TERMIO_FILE" | head -1 | cut -d: -f1)
    INSERT_LINE=$((LINE_NUM + 3))
    
    cat > /tmp/tmux_cleanup.txt << 'EOF'
    // Cleanup tmux integration
    if (self.tmux_handle) |handle| {
        handle.deinit();
        self.alloc.destroy(handle);
    }
EOF
    
    sed -i '' "${INSERT_LINE}r /tmp/tmux_cleanup.txt" "$TERMIO_FILE"
    echo "✅ Added cleanup code"
fi

echo ""
echo "======================================="
echo "  ✅ Deep Integration Applied!"
echo "======================================="
echo ""
echo "Now rebuild Ghostty with:"
echo "  make build-ghostty"
echo ""
echo "Then you can use @tmux commands:"
echo "  @tmux new-session demo"
echo "  @tmux list-sessions"
echo "  @tmux attach demo"
echo ""
echo "To revert changes:"
echo "  cp $BACKUP_FILE $TERMIO_FILE"
echo "======================================="