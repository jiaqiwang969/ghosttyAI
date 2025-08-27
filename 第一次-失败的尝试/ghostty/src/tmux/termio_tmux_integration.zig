// termio_tmux_integration.zig - Incremental tmux integration for Termio
// Purpose: Add optional tmux support to existing Termio without breaking existing build
// Date: 2025-08-26
// Task: T-503 - Incremental integration

const std = @import("std");
const tmux = @import("../tmux/tmux_terminal_bridge.zig");
const SessionManager = @import("../tmux/session_manager.zig").SessionManager;

// This struct extends Termio with tmux capabilities
pub const TmuxExtension = struct {
    enabled: bool,
    session_manager: ?*SessionManager,
    default_session_name: []const u8,
    
    pub fn init(allocator: std.mem.Allocator, config: TmuxConfig) !TmuxExtension {
        if (!config.enable_tmux) {
            return TmuxExtension{
                .enabled = false,
                .session_manager = null,
                .default_session_name = "",
            };
        }
        
        var session_mgr = try allocator.create(SessionManager);
        session_mgr.* = try SessionManager.init(allocator);
        
        return TmuxExtension{
            .enabled = true,
            .session_manager = session_mgr,
            .default_session_name = config.default_session_name,
        };
    }
    
    pub fn deinit(self: *TmuxExtension, allocator: std.mem.Allocator) void {
        if (self.session_manager) |mgr| {
            mgr.deinit();
            allocator.destroy(mgr);
        }
    }
    
    pub fn attachToTerminal(self: *TmuxExtension, terminal: anytype) !void {
        if (!self.enabled or self.session_manager == null) return;
        
        const mgr = self.session_manager.?;
        
        // Create or attach to default session
        _ = mgr.createSession(self.default_session_name, terminal) catch |err| switch (err) {
            error.SessionAlreadyExists => {
                try mgr.attachSession(self.default_session_name);
            },
            else => return err,
        };
        
        // Initialize tmux bridge for this terminal
        try tmux.initTmuxIntegration(terminal, terminal.alloc);
    }
    
    pub fn handleCommand(self: *TmuxExtension, command: []const u8) !void {
        if (!self.enabled) return error.TmuxNotEnabled;
        
        // Parse tmux-style commands
        if (std.mem.startsWith(u8, command, "new-session")) {
            // Handle new session creation
            const name = command[12..]; // Skip "new-session "
            if (self.session_manager) |mgr| {
                _ = try mgr.createSession(name, null);
            }
        } else if (std.mem.startsWith(u8, command, "list-sessions")) {
            // Handle session listing
            if (self.session_manager) |mgr| {
                const sessions = mgr.listSessions();
                for (sessions) |session| {
                    std.log.info("Session: {s} (windows: {}, attached: {})", .{
                        session.name,
                        session.window_count,
                        session.is_attached,
                    });
                }
            }
        }
    }
};

pub const TmuxConfig = struct {
    enable_tmux: bool = false,
    default_session_name: []const u8 = "main",
    persist_sessions: bool = true,
    auto_attach: bool = true,
};

// Helper function to add to Termio.init incrementally
pub fn enhanceTermio(termio: anytype, allocator: std.mem.Allocator, config: TmuxConfig) !void {
    var tmux_ext = try TmuxExtension.init(allocator, config);
    defer tmux_ext.deinit(allocator);
    
    if (tmux_ext.enabled) {
        try tmux_ext.attachToTerminal(&termio.terminal);
        std.log.info("tmux integration enabled for Termio", .{});
    }
}

// Simple test that won't break existing build
test "TmuxExtension disabled by default" {
    const testing = std.testing;
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();
    
    const config = TmuxConfig{
        .enable_tmux = false,
    };
    
    var ext = try TmuxExtension.init(allocator, config);
    defer ext.deinit(allocator);
    
    try testing.expect(!ext.enabled);
    try testing.expect(ext.session_manager == null);
}