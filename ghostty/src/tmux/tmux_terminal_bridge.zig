// tmux_terminal_bridge.zig - Bridge between tmux callbacks and Ghostty Terminal
// Purpose: Connect Week 5 tmux UI Backend to Ghostty Terminal.zig
// Date: 2025-08-26
// Task: T-502 - Connect Terminal.zig to receive callbacks

const std = @import("std");
const Terminal = @import("../terminal/Terminal.zig");
const Screen = @import("../terminal/Screen.zig");
const Cell = @import("../terminal/page.zig").Cell;
const Style = @import("../terminal/style.zig").Style;
const color_pkg = @import("../terminal/color.zig");

const log = std.log.scoped(.tmux_bridge);

// Import our FFI bridge
const ffi = @import("ffi_bridge.zig");

// Import C functions from libtmuxcore
const c = @cImport({
    @cInclude("tmux/ui_backend/ui_backend_minimal.h");
});

// Global instance for callbacks (single terminal support for now)
var g_terminal: ?*Terminal = null;
var g_handler: ?*ffi.CallbackHandler = null;

// ===========================================================================
// C Callback Implementations that write to Ghostty Terminal
// ===========================================================================

fn onCell(
    ch: u8,
    row: c_int,
    col: c_int,
    attr: c_int,
    fg: c_int,
    bg: c_int,
    user_data: ?*anyopaque,
) callconv(.C) void {
    _ = user_data;
    
    if (g_terminal) |terminal| {
        // Convert position to Ghostty coordinates
        const term_row = @as(usize, @intCast(row));
        const term_col = @as(usize, @intCast(col));
        
        // Bounds check
        if (term_row >= terminal.rows or term_col >= terminal.cols) {
            return;
        }
        
        // Create a cell with the character and attributes
        var cell = Cell{
            .char = .{ .codepoint = @as(u21, ch) },
            .style = Style{
                .flags = Style.Flags.fromInt(@as(u16, @intCast(attr))),
                .fg = color_pkg.RGB.fromInt(@as(u24, @intCast(fg))),
                .bg = color_pkg.RGB.fromInt(@as(u24, @intCast(bg))),
            },
        };
        
        // Get the active screen and write the cell
        const screen = switch (terminal.active_screen) {
            .primary => &terminal.screen,
            .alternate => &terminal.secondary_screen,
        };
        
        // Write to the grid
        screen.cursor.x = term_col;
        screen.cursor.y = term_row;
        screen.writeCell(cell) catch |err| {
            log.err("Failed to write cell at ({},{}): {}", .{ row, col, err });
        };
        
        log.debug("Cell '{}' written at ({},{})", .{ ch, row, col });
    }
}

fn onClearLine(row: c_int, user_data: ?*anyopaque) callconv(.C) void {
    _ = user_data;
    
    if (g_terminal) |terminal| {
        const term_row = @as(usize, @intCast(row));
        
        if (term_row >= terminal.rows) {
            return;
        }
        
        const screen = switch (terminal.active_screen) {
            .primary => &terminal.screen,
            .alternate => &terminal.secondary_screen,
        };
        
        // Clear the entire line
        screen.clearLine(term_row) catch |err| {
            log.err("Failed to clear line {}: {}", .{ row, err });
        };
        
        log.debug("Line {} cleared", .{row});
    }
}

fn onClearScreen(user_data: ?*anyopaque) callconv(.C) void {
    _ = user_data;
    
    if (g_terminal) |terminal| {
        const screen = switch (terminal.active_screen) {
            .primary => &terminal.screen,
            .alternate => &terminal.secondary_screen,
        };
        
        screen.clear() catch |err| {
            log.err("Failed to clear screen: {}", .{err});
        };
        
        log.debug("Screen cleared", .{});
    }
}

fn onScrollUp(count: c_int, user_data: ?*anyopaque) callconv(.C) void {
    _ = user_data;
    
    if (g_terminal) |terminal| {
        const lines = @as(usize, @intCast(count));
        
        const screen = switch (terminal.active_screen) {
            .primary => &terminal.screen,
            .alternate => &terminal.secondary_screen,
        };
        
        screen.scrollUp(lines) catch |err| {
            log.err("Failed to scroll up {} lines: {}", .{ count, err });
        };
        
        log.debug("Scrolled up {} lines", .{count});
    }
}

fn onScrollDown(count: c_int, user_data: ?*anyopaque) callconv(.C) void {
    _ = user_data;
    
    if (g_terminal) |terminal| {
        const lines = @as(usize, @intCast(count));
        
        const screen = switch (terminal.active_screen) {
            .primary => &terminal.screen,
            .alternate => &terminal.secondary_screen,
        };
        
        screen.scrollDown(lines) catch |err| {
            log.err("Failed to scroll down {} lines: {}", .{ count, err });
        };
        
        log.debug("Scrolled down {} lines", .{count});
    }
}

fn onFlush(user_data: ?*anyopaque) callconv(.C) void {
    _ = user_data;
    
    if (g_terminal) |terminal| {
        // Trigger a redraw/refresh
        terminal.flags.dirty = true;
        log.debug("Flush requested - marked terminal dirty", .{});
    }
}

// ===========================================================================
// Public API
// ===========================================================================

/// Initialize tmux integration for the given terminal
pub fn initTmuxIntegration(terminal: *Terminal, allocator: std.mem.Allocator) !void {
    // Store terminal reference
    g_terminal = terminal;
    
    // Initialize callback handler for debugging
    g_handler = try allocator.create(ffi.CallbackHandler);
    g_handler.* = try ffi.CallbackHandler.init(
        @as(usize, terminal.rows),
        @as(usize, terminal.cols),
        allocator
    );
    
    // Create callback table
    const callbacks = ffi.UICallbacks{
        .on_cell = onCell,
        .on_clear_line = onClearLine,
        .on_clear_screen = onClearScreen,
        .on_insert_line = null,  // TODO: Implement
        .on_delete_line = null,  // TODO: Implement
        .on_clear_eol = null,    // TODO: Implement
        .on_reverse_index = null, // TODO: Implement
        .on_line_feed = null,     // TODO: Implement
        .on_scroll_up = onScrollUp,
        .on_scroll_down = onScrollDown,
        .on_flush = onFlush,
    };
    
    // Initialize UI Backend
    const env_set = std.c.setenv("TMUX_UI_BACKEND", "ghostty", 1);
    if (env_set != 0) {
        log.warn("Failed to set TMUX_UI_BACKEND environment variable", .{});
    }
    
    // Initialize the backend
    ffi.ui_backend_init();
    
    // Check if enabled
    if (ffi.ui_backend_enabled() == 0) {
        return error.UIBackendNotEnabled;
    }
    
    // Register callbacks
    ffi.ui_backend_set_callbacks(&callbacks, g_handler);
    
    // Verify callbacks are registered
    if (ffi.ui_backend_has_callbacks() == 0) {
        return error.CallbackRegistrationFailed;
    }
    
    const status = ffi.ui_backend_get_status();
    log.info("tmux UI Backend initialized: {s}", .{status});
}

/// Deinitialize tmux integration
pub fn deinitTmuxIntegration(allocator: std.mem.Allocator) void {
    if (g_handler) |handler| {
        handler.deinit(allocator);
        allocator.destroy(handler);
        g_handler = null;
    }
    
    g_terminal = null;
    
    // Clean up UI Backend
    // Note: We may need to add ui_backend_cleanup() to the C API
    
    log.info("tmux integration deinitialized", .{});
}

/// Handle a tmux command through the UI Backend
pub fn handleTmuxCommand(command: []const u8) !void {
    _ = command; // TODO: Implement command execution
    
    // For now, just test that callbacks work
    if (g_terminal == null) {
        return error.TerminalNotInitialized;
    }
    
    // This would normally parse and execute tmux commands
    // For testing, we can trigger some test callbacks
    
    log.info("tmux command received: {s}", .{command});
}

/// Test function to verify integration
pub fn testTmuxIntegration() !void {
    log.info("Testing tmux integration...", .{});
    
    // This would normally come from actual tmux commands
    // For now, we can manually trigger some callbacks to test
    
    // Test writing "Hello from tmux!"
    const test_string = "Hello from tmux!";
    for (test_string, 0..) |ch, i| {
        onCell(@as(u8, ch), 0, @as(c_int, @intCast(i)), 0, 0xFFFFFF, 0x000000, null);
    }
    
    onFlush(null);
    
    log.info("Test complete - check terminal for output", .{});
}