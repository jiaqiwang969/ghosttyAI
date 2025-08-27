// termio_tmux_patch.zig - Patch to add tmux integration to Termio
// Purpose: Add tmux support initialization to Termio
// Date: 2025-08-26
// This shows where to add the tmux integration calls

// Add this import at the top of Termio.zig (around line 30):
const tmux_bridge = @import("../tmux/tmux_terminal_bridge.zig");

// Add this field to the Termio struct (around line 70):
/// Whether tmux integration is enabled
tmux_enabled: bool = false,

// Add this to the init function after creating the terminal (around line 310):
// After:
//     .terminal = term,
// Add:
    // Initialize tmux integration if enabled
    if (opts.config.enable_tmux) {
        tmux_bridge.initTmuxIntegration(&term, alloc) catch |err| {
            log.warn("Failed to initialize tmux integration: {}", .{err});
        };
        result.tmux_enabled = true;
    }

// Add this to the deinit function (wherever Terminal cleanup happens):
    // Clean up tmux integration
    if (self.tmux_enabled) {
        tmux_bridge.deinitTmuxIntegration(self.alloc);
    }

// Add a method to handle tmux commands:
pub fn handleTmuxCommand(self: *Termio, command: []const u8) !void {
    if (!self.tmux_enabled) {
        return error.TmuxNotEnabled;
    }
    
    try tmux_bridge.handleTmuxCommand(command);
}

// Example of how to trigger tmux test from Terminal input:
// In the input processing, check for "@tmux" prefix:
pub fn processInput(self: *Termio, data: []const u8) !void {
    // Check if this is a tmux command
    if (std.mem.startsWith(u8, data, "@tmux ")) {
        const cmd = data[6..];
        try self.handleTmuxCommand(cmd);
        return;
    }
    
    // Normal input processing...
}