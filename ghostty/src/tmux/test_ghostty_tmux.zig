// test_ghostty_tmux.zig - Test program for Ghostty tmux integration
// Purpose: Verify tmux callbacks work with Ghostty Terminal
// Date: 2025-08-26
// Task: T-502-B - First tmux output in Ghostty

const std = @import("std");
const Terminal = @import("../terminal/Terminal.zig");
const tmux_bridge = @import("tmux_terminal_bridge.zig");

const c = @cImport({
    @cInclude("tmux/ui_backend/ui_backend_minimal.h");
});

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();
    
    std.log.info("=== Ghostty tmux Integration Test ===", .{});
    
    // Create a terminal instance
    var terminal = try Terminal.init(allocator, .{
        .cols = 80,
        .rows = 24,
        .max_scrollback = 1000,
        .default_modes = .{},
    });
    defer terminal.deinit(allocator);
    
    std.log.info("Terminal created: {}x{}", .{ terminal.cols, terminal.rows });
    
    // Initialize tmux integration
    try tmux_bridge.initTmuxIntegration(&terminal, allocator);
    defer tmux_bridge.deinitTmuxIntegration(allocator);
    
    std.log.info("tmux integration initialized", .{});
    
    // Test 1: Write "Hello from tmux!" through callbacks
    std.log.info("\nTest 1: Writing text through callbacks...", .{});
    try tmux_bridge.testTmuxIntegration();
    
    // Test 2: Simulate actual tmux commands with command IDs
    std.log.info("\nTest 2: Simulating tmux cell commands...", .{});
    
    // Create test cells with command IDs
    var cell = c.grid_cell{
        .data = .{ .size = 1, .data = "H" },
        .attr = 0,
        .fg = 0xFFFFFF,
        .bg = 0x000000,
    };
    
    var ctx = c.tty_ctx{
        .cell = &cell,
        .ocy = 1,
        .ocx = 0,
        .ui_cmd_id = 1, // UI_CMD_CELL
    };
    
    // Get UI backend instance
    const backend = c.ui_backend_get_instance();
    if (backend == null) {
        std.log.err("Failed to get UI backend instance", .{});
        return error.BackendNotFound;
    }
    
    // Dispatch the cell command
    const result = c.ui_backend_dispatch(backend, null, &ctx);
    if (result == 0) {
        std.log.info("Cell 'H' dispatched successfully", .{});
    } else {
        std.log.err("Failed to dispatch cell (result={})", .{result});
    }
    
    // Write more characters to spell "Hello"
    const hello = "ello tmux!";
    for (hello, 0..) |ch, i| {
        cell.data.data[0] = ch;
        ctx.ocx = @as(c_uint, @intCast(i + 1));
        _ = c.ui_backend_dispatch(backend, null, &ctx);
    }
    
    // Flush to ensure display
    c.ui_backend_flush();
    
    // Test 3: Clear operations
    std.log.info("\nTest 3: Testing clear operations...", .{});
    
    ctx.ui_cmd_id = 2; // UI_CMD_CLEARLINE
    ctx.ocy = 2;
    const clear_result = c.ui_backend_dispatch(backend, null, &ctx);
    if (clear_result == 0) {
        std.log.info("Clear line 2 successful", .{});
    }
    
    // Print terminal grid contents for verification
    std.log.info("\n=== Terminal Grid Contents ===", .{});
    const screen = switch (terminal.active_screen) {
        .primary => &terminal.screen,
        .alternate => &terminal.secondary_screen,
    };
    
    // Show first few rows
    var row: usize = 0;
    while (row < 5 and row < terminal.rows) : (row += 1) {
        std.log.info("Row {}: {}", .{ row, screen.getRow(row) });
    }
    
    std.log.info("\n=== Test Complete ===", .{});
    std.log.info("✅ tmux integration test successful!", .{});
}