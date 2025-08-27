// terminal_tmux_integration.zig - Tmux integration for Terminal
// Purpose: Add tmux support to Ghostty Terminal
// Date: 2025-08-26
// Task: T-304-R - Integrate with Terminal module

const std = @import("std");
const TmuxCore = @import("../tmux/core.zig").TmuxCore;

/// Extension structure for Terminal to add tmux support
pub const TmuxIntegration = struct {
    /// Optional tmux core instance
    tmux_core: ?TmuxCore,
    
    /// Whether tmux mode is enabled
    tmux_enabled: bool,
    
    /// Initialize tmux integration
    pub fn init(allocator: std.mem.Allocator, enable_tmux: bool) !TmuxIntegration {
        if (enable_tmux) {
            const tmux = TmuxCore.init(allocator) catch |err| {
                std.log.warn("Failed to initialize tmux core: {}", .{err});
                return TmuxIntegration{
                    .tmux_core = null,
                    .tmux_enabled = false,
                };
            };
            
            // Set backend mode to ghostty for integration
            try tmux.setBackendMode("ghostty");
            
            return TmuxIntegration{
                .tmux_core = tmux,
                .tmux_enabled = true,
            };
        }
        
        return TmuxIntegration{
            .tmux_core = null,
            .tmux_enabled = false,
        };
    }
    
    /// Cleanup tmux integration
    pub fn deinit(self: *TmuxIntegration) void {
        if (self.tmux_core) |*tmux| {
            tmux.deinit();
            self.tmux_core = null;
        }
        self.tmux_enabled = false;
    }
    
    /// Handle tmux command
    pub fn handleTmuxCommand(self: *TmuxIntegration, cmd: []const u8) !void {
        if (self.tmux_core) |*tmux| {
            try tmux.executeCommand(cmd);
        } else {
            return error.TmuxNotEnabled;
        }
    }
    
    /// Create new tmux session
    pub fn createSession(self: *TmuxIntegration, name: []const u8) !void {
        if (self.tmux_core) |*tmux| {
            try tmux.createSession(name);
        } else {
            return error.TmuxNotEnabled;
        }
    }
    
    /// Check if tmux is enabled
    pub fn isEnabled(self: *const TmuxIntegration) bool {
        return self.tmux_enabled and self.tmux_core != null;
    }
};

/// Helper to parse tmux commands from terminal input
pub fn parseTmuxCommand(input: []const u8) ?[]const u8 {
    // Check for tmux command prefix (e.g., "@tmux " or similar)
    const prefix = "@tmux ";
    if (std.mem.startsWith(u8, input, prefix)) {
        return input[prefix.len..];
    }
    return null;
}