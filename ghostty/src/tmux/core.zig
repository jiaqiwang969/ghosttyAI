// core.zig - Ghostty tmux integration core module
// Purpose: Integrate libtmuxcore with Ghostty terminal
// Date: 2025-08-26
// Task: T-303-R - Create Ghostty tmux integration module

const std = @import("std");
const builtin = @import("builtin");

// C imports for libtmuxcore
const c = @cImport({
    @cInclude("libtmuxcore.h");
});

// Import Ghostty components
const Terminal = @import("../terminal/Terminal.zig");
const callbacks = @import("callbacks.zig");
const c_types = @import("c_types.zig");

const log = std.log.scoped(.tmux_core);

/// Error types for tmux integration
pub const TmuxError = error{
    InitFailed,
    InvalidHandle,
    CommandFailed,
    CallbackRegistrationFailed,
    BackendUnavailable,
};

/// Tmux core integration for Ghostty
pub const TmuxCore = struct {
    handle: ?*c.tmc_handle_t,
    allocator: std.mem.Allocator,
    terminal: ?*Terminal,
    callbacks_registered: bool,
    
    /// Initialize tmux core
    pub fn init(allocator: std.mem.Allocator) !TmuxCore {
        log.info("Initializing tmux core integration", .{});
        
        // Initialize libtmuxcore
        const handle = c.tmc_init();
        if (handle == null) {
            log.err("Failed to initialize libtmuxcore", .{});
            return TmuxError.InitFailed;
        }
        
        log.info("libtmuxcore initialized successfully", .{});
        
        return TmuxCore{
            .handle = handle,
            .allocator = allocator,
            .terminal = null,
            .callbacks_registered = false,
        };
    }
    
    /// Cleanup tmux core
    pub fn deinit(self: *TmuxCore) void {
        if (self.handle) |handle| {
            c.tmc_cleanup(handle);
            self.handle = null;
        }
        log.info("tmux core cleaned up", .{});
    }
    
    /// Attach to a terminal instance
    pub fn attachTerminal(self: *TmuxCore, terminal: *Terminal) !void {
        self.terminal = terminal;
        
        // Register callbacks with libtmuxcore
        if (!self.callbacks_registered) {
            try self.registerCallbacks();
            self.callbacks_registered = true;
        }
        
        log.info("Attached to terminal", .{});
    }
    
    /// Execute a tmux command
    pub fn executeCommand(self: *TmuxCore, cmd: []const u8) !void {
        if (self.handle == null) {
            return TmuxError.InvalidHandle;
        }
        
        // Convert Zig string to C string
        const c_cmd = try self.allocator.dupeZ(u8, cmd);
        defer self.allocator.free(c_cmd);
        
        const result = c.tmc_execute_command(self.handle, c_cmd);
        if (result != 0) {
            log.warn("Command failed: {s}", .{cmd});
            return TmuxError.CommandFailed;
        }
        
        log.debug("Executed command: {s}", .{cmd});
    }
    
    /// Create a new tmux session
    pub fn createSession(self: *TmuxCore, name: []const u8) !void {
        if (self.handle == null) {
            return TmuxError.InvalidHandle;
        }
        
        const c_name = try self.allocator.dupeZ(u8, name);
        defer self.allocator.free(c_name);
        
        const result = c.tmc_create_session(self.handle, c_name);
        if (result != 0) {
            return TmuxError.CommandFailed;
        }
        
        log.info("Created session: {s}", .{name});
    }
    
    /// Destroy a tmux session
    pub fn destroySession(self: *TmuxCore, name: []const u8) !void {
        if (self.handle == null) {
            return TmuxError.InvalidHandle;
        }
        
        const c_name = try self.allocator.dupeZ(u8, name);
        defer self.allocator.free(c_name);
        
        const result = c.tmc_destroy_session(self.handle, c_name);
        if (result != 0) {
            return TmuxError.CommandFailed;
        }
        
        log.info("Destroyed session: {s}", .{name});
    }
    
    /// Set backend mode (ghostty, libevent, or hybrid)
    pub fn setBackendMode(self: *TmuxCore, mode: []const u8) !void {
        if (self.handle == null) {
            return TmuxError.InvalidHandle;
        }
        
        const c_mode = try self.allocator.dupeZ(u8, mode);
        defer self.allocator.free(c_mode);
        
        const result = c.tmc_set_backend_mode(self.handle, c_mode);
        if (result != 0) {
            return TmuxError.BackendUnavailable;
        }
        
        log.info("Set backend mode to: {s}", .{mode});
    }
    
    /// Register callbacks with libtmuxcore
    fn registerCallbacks(self: *TmuxCore) !void {
        if (self.handle == null) {
            return TmuxError.InvalidHandle;
        }
        
        // Create callback structure
        var cb = c.tmc_ui_callbacks_t{
            .on_redraw = onRedrawCallback,
            .on_cell_update = onCellUpdateCallback,
            .on_cursor_move = onCursorMoveCallback,
            .on_resize = onResizeCallback,
            .user_data = @ptrCast(self),
        };
        
        const result = c.tmc_register_callbacks(self.handle, &cb);
        if (result != 0) {
            return TmuxError.CallbackRegistrationFailed;
        }
        
        log.info("Registered UI callbacks", .{});
    }
    
    // Callback functions that will be called by libtmuxcore
    fn onRedrawCallback(user_data: ?*anyopaque) callconv(.C) void {
        const self: *TmuxCore = @ptrCast(@alignCast(user_data orelse return));
        
        if (self.terminal) |terminal| {
            // Trigger terminal redraw
            log.debug("Redraw callback triggered", .{});
            // TODO: Call terminal redraw method
            _ = terminal;
        }
    }
    
    fn onCellUpdateCallback(row: c_int, col: c_int, text: [*c]const u8, user_data: ?*anyopaque) callconv(.C) void {
        const self: *TmuxCore = @ptrCast(@alignCast(user_data orelse return));
        
        if (self.terminal) |terminal| {
            // Update terminal cell
            const text_slice = std.mem.span(text);
            log.debug("Cell update at ({}, {}): {s}", .{ row, col, text_slice });
            // TODO: Call terminal cell update method
            _ = terminal;
        }
    }
    
    fn onCursorMoveCallback(row: c_int, col: c_int, user_data: ?*anyopaque) callconv(.C) void {
        const self: *TmuxCore = @ptrCast(@alignCast(user_data orelse return));
        
        if (self.terminal) |terminal| {
            // Move cursor
            log.debug("Cursor move to ({}, {})", .{ row, col });
            // TODO: Call terminal cursor move method
            _ = terminal;
        }
    }
    
    fn onResizeCallback(rows: c_int, cols: c_int, user_data: ?*anyopaque) callconv(.C) void {
        const self: *TmuxCore = @ptrCast(@alignCast(user_data orelse return));
        
        if (self.terminal) |terminal| {
            // Handle resize
            log.debug("Resize to {}x{}", .{ cols, rows });
            // TODO: Call terminal resize method
            _ = terminal;
        }
    }
};

/// Check if tmux integration is available
pub fn isAvailable() bool {
    // Check if libtmuxcore is available at runtime
    const handle = c.tmc_init();
    if (handle == null) {
        return false;
    }
    c.tmc_cleanup(handle);
    return true;
}

/// Get libtmuxcore version
pub fn getVersion() struct { major: u32, minor: u32, patch: u32 } {
    var major: c_int = 0;
    var minor: c_int = 0;
    var patch: c_int = 0;
    c.tmc_get_version(&major, &minor, &patch);
    return .{
        .major = @intCast(@as(u32, major)),
        .minor = @intCast(@as(u32, minor)),
        .patch = @intCast(@as(u32, patch)),
    };
}

// Tests
test "TmuxCore initialization" {
    const allocator = std.testing.allocator;
    
    // Skip test if libtmuxcore is not available
    if (!isAvailable()) {
        return;
    }
    
    var tmux = try TmuxCore.init(allocator);
    defer tmux.deinit();
    
    try std.testing.expect(tmux.handle != null);
}

test "TmuxCore version" {
    const version = getVersion();
    try std.testing.expect(version.major >= 1);
}