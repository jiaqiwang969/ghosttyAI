// ffi_bridge.zig - C-Zig FFI Bridge for tmux UI Backend
// Purpose: Connect C callbacks from libtmuxcore to Zig terminal implementation
// Date: 2025-08-26
// Task: W4-INC-003 - Week 4 FFI Bridge Implementation

const std = @import("std");
const c = @cImport({
    @cInclude("stdlib.h");
});

// ===========================================================================
// C Callback Function Type Definitions
// ===========================================================================

pub const CellCallback = *const fn (
    ch: u8,
    row: c_int,
    col: c_int,
    attr: c_int,
    fg: c_int,
    bg: c_int,
    user_data: ?*anyopaque
) callconv(.C) void;

pub const ClearLineCallback = *const fn (
    row: c_int,
    user_data: ?*anyopaque
) callconv(.C) void;

pub const ClearScreenCallback = *const fn (
    user_data: ?*anyopaque
) callconv(.C) void;

pub const InsertLineCallback = *const fn (
    row: c_int,
    user_data: ?*anyopaque
) callconv(.C) void;

pub const DeleteLineCallback = *const fn (
    row: c_int,
    user_data: ?*anyopaque
) callconv(.C) void;

pub const ClearEOLCallback = *const fn (
    row: c_int,
    col: c_int,
    user_data: ?*anyopaque
) callconv(.C) void;

pub const ReverseIndexCallback = *const fn (
    user_data: ?*anyopaque
) callconv(.C) void;

pub const LineFeedCallback = *const fn (
    user_data: ?*anyopaque
) callconv(.C) void;

pub const ScrollUpCallback = *const fn (
    count: c_int,
    user_data: ?*anyopaque
) callconv(.C) void;

pub const ScrollDownCallback = *const fn (
    count: c_int,
    user_data: ?*anyopaque
) callconv(.C) void;

pub const FlushCallback = *const fn (
    user_data: ?*anyopaque
) callconv(.C) void;

// ===========================================================================
// Callback Function Table Structure (matches C's ui_callbacks_t)
// ===========================================================================

pub const UICallbacks = extern struct {
    on_cell: ?CellCallback,
    on_clear_line: ?ClearLineCallback,
    on_clear_screen: ?ClearScreenCallback,
    on_insert_line: ?InsertLineCallback,
    on_delete_line: ?DeleteLineCallback,
    on_clear_eol: ?ClearEOLCallback,
    on_reverse_index: ?ReverseIndexCallback,
    on_line_feed: ?LineFeedCallback,
    on_scroll_up: ?ScrollUpCallback,
    on_scroll_down: ?ScrollDownCallback,
    on_flush: ?FlushCallback,
};

// ===========================================================================
// External C Function Declarations from libtmuxcore
// ===========================================================================

extern fn ui_backend_set_callbacks(callbacks: *const UICallbacks, user_data: ?*anyopaque) void;
extern fn ui_backend_init() void;
extern fn ui_backend_enabled() c_int;
extern fn ui_backend_has_callbacks() c_int;
extern fn ui_backend_flush() void;
extern fn ui_backend_get_status() [*c]const u8;

// ===========================================================================
// Cell Structure for Grid Storage
// ===========================================================================

pub const Cell = struct {
    char: u8,
    attr: u16,
    fg: u32,
    bg: u32,
};

// ===========================================================================
// Zig Callback Handler Implementation
// ===========================================================================

pub const CallbackHandler = struct {
    grid: [][]Cell,
    rows: usize,
    cols: usize,
    cursor_row: usize,
    cursor_col: usize,
    allocator: std.mem.Allocator,
    
    // Initialize the callback handler with a grid
    pub fn init(rows: usize, cols: usize, allocator: std.mem.Allocator) !CallbackHandler {
        var grid = try allocator.alloc([]Cell, rows);
        errdefer allocator.free(grid);
        
        for (grid, 0..) |*row, i| {
            row.* = try allocator.alloc(Cell, cols);
            errdefer {
                // Free previously allocated rows on error
                for (grid[0..i]) |r| {
                    allocator.free(r);
                }
            }
            
            // Initialize cells to spaces
            for (row.*) |*cell| {
                cell.* = Cell{
                    .char = ' ',
                    .attr = 0,
                    .fg = 0xFFFFFF,  // Default white foreground
                    .bg = 0x000000,  // Default black background
                };
            }
        }
        
        return CallbackHandler{
            .grid = grid,
            .rows = rows,
            .cols = cols,
            .cursor_row = 0,
            .cursor_col = 0,
            .allocator = allocator,
        };
    }
    
    // Cleanup
    pub fn deinit(self: *CallbackHandler) void {
        for (self.grid) |row| {
            self.allocator.free(row);
        }
        self.allocator.free(self.grid);
    }
    
    // ===========================================================================
    // C Callback Implementations
    // ===========================================================================
    
    // Handle cell character placement
    pub fn onCell(
        ch: u8,
        row: c_int,
        col: c_int,
        attr: c_int,
        fg: c_int,
        bg: c_int,
        user_data: ?*anyopaque
    ) callconv(.C) void {
        if (user_data) |ptr| {
            const self = @as(*CallbackHandler, @ptrCast(@alignCast(ptr)));
            
            const r = @as(usize, @intCast(row));
            const c = @as(usize, @intCast(col));
            
            if (r < self.rows and c < self.cols) {
                self.grid[r][c] = Cell{
                    .char = ch,
                    .attr = @as(u16, @intCast(attr & 0xFFFF)),
                    .fg = @as(u32, @intCast(fg)),
                    .bg = @as(u32, @intCast(bg)),
                };
                
                self.cursor_row = r;
                self.cursor_col = c + 1;
                
                std.debug.print("[ZIG] Cell '{c}' at ({},{}) received - attr={x} fg={x} bg={x}\n", 
                    .{ ch, r, c, attr, fg, bg });
            }
        }
    }
    
    // Handle line clearing
    pub fn onClearLine(row: c_int, user_data: ?*anyopaque) callconv(.C) void {
        if (user_data) |ptr| {
            const self = @as(*CallbackHandler, @ptrCast(@alignCast(ptr)));
            
            const r = @as(usize, @intCast(row));
            if (r < self.rows) {
                for (self.grid[r]) |*cell| {
                    cell.char = ' ';
                    cell.attr = 0;
                }
                std.debug.print("[ZIG] Line {} cleared\n", .{r});
            }
        }
    }
    
    // Handle screen clearing
    pub fn onClearScreen(user_data: ?*anyopaque) callconv(.C) void {
        if (user_data) |ptr| {
            const self = @as(*CallbackHandler, @ptrCast(@alignCast(ptr)));
            
            for (self.grid) |row| {
                for (row) |*cell| {
                    cell.char = ' ';
                    cell.attr = 0;
                }
            }
            self.cursor_row = 0;
            self.cursor_col = 0;
            std.debug.print("[ZIG] Screen cleared\n", .{});
        }
    }
    
    // Handle line insertion
    pub fn onInsertLine(row: c_int, user_data: ?*anyopaque) callconv(.C) void {
        if (user_data) |ptr| {
            const self = @as(*CallbackHandler, @ptrCast(@alignCast(ptr)));
            
            const r = @as(usize, @intCast(row));
            if (r < self.rows) {
                // Shift lines down
                var i = self.rows - 1;
                while (i > r) : (i -= 1) {
                    std.mem.copy(Cell, self.grid[i], self.grid[i - 1]);
                }
                
                // Clear the inserted line
                for (self.grid[r]) |*cell| {
                    cell.char = ' ';
                    cell.attr = 0;
                }
                
                std.debug.print("[ZIG] Line inserted at row {}\n", .{r});
            }
        }
    }
    
    // Handle line deletion
    pub fn onDeleteLine(row: c_int, user_data: ?*anyopaque) callconv(.C) void {
        if (user_data) |ptr| {
            const self = @as(*CallbackHandler, @ptrCast(@alignCast(ptr)));
            
            const r = @as(usize, @intCast(row));
            if (r < self.rows) {
                // Shift lines up
                var i = r;
                while (i < self.rows - 1) : (i += 1) {
                    std.mem.copy(Cell, self.grid[i], self.grid[i + 1]);
                }
                
                // Clear the last line
                for (self.grid[self.rows - 1]) |*cell| {
                    cell.char = ' ';
                    cell.attr = 0;
                }
                
                std.debug.print("[ZIG] Line deleted at row {}\n", .{r});
            }
        }
    }
    
    // Handle clear to end of line
    pub fn onClearEOL(row: c_int, col: c_int, user_data: ?*anyopaque) callconv(.C) void {
        if (user_data) |ptr| {
            const self = @as(*CallbackHandler, @ptrCast(@alignCast(ptr)));
            
            const r = @as(usize, @intCast(row));
            const c = @as(usize, @intCast(col));
            
            if (r < self.rows) {
                var i = c;
                while (i < self.cols) : (i += 1) {
                    self.grid[r][i].char = ' ';
                    self.grid[r][i].attr = 0;
                }
                std.debug.print("[ZIG] Cleared from ({},{}) to EOL\n", .{ r, c });
            }
        }
    }
    
    // Handle reverse index (scroll up)
    pub fn onReverseIndex(user_data: ?*anyopaque) callconv(.C) void {
        if (user_data) |ptr| {
            const self = @as(*CallbackHandler, @ptrCast(@alignCast(ptr)));
            
            // Move all lines down by one
            var i = self.rows - 1;
            while (i > 0) : (i -= 1) {
                std.mem.copy(Cell, self.grid[i], self.grid[i - 1]);
            }
            
            // Clear the first line
            for (self.grid[0]) |*cell| {
                cell.char = ' ';
                cell.attr = 0;
            }
            
            std.debug.print("[ZIG] Reverse index performed\n", .{});
        }
    }
    
    // Handle line feed (move cursor down)
    pub fn onLineFeed(user_data: ?*anyopaque) callconv(.C) void {
        if (user_data) |ptr| {
            const self = @as(*CallbackHandler, @ptrCast(@alignCast(ptr)));
            
            if (self.cursor_row < self.rows - 1) {
                self.cursor_row += 1;
            } else {
                // Scroll up if at bottom
                onScrollUp(1, user_data);
            }
            self.cursor_col = 0;
            
            std.debug.print("[ZIG] Line feed - cursor now at ({},{})\n", 
                .{ self.cursor_row, self.cursor_col });
        }
    }
    
    // Handle scroll up
    pub fn onScrollUp(count: c_int, user_data: ?*anyopaque) callconv(.C) void {
        if (user_data) |ptr| {
            const self = @as(*CallbackHandler, @ptrCast(@alignCast(ptr)));
            const n = @as(usize, @intCast(count));
            
            // Move lines up by count
            var i: usize = 0;
            while (i < self.rows - n) : (i += 1) {
                std.mem.copy(Cell, self.grid[i], self.grid[i + n]);
            }
            
            // Clear the bottom lines
            i = self.rows - n;
            while (i < self.rows) : (i += 1) {
                for (self.grid[i]) |*cell| {
                    cell.char = ' ';
                    cell.attr = 0;
                }
            }
            
            std.debug.print("[ZIG] Scrolled up {} lines\n", .{n});
        }
    }
    
    // Handle scroll down
    pub fn onScrollDown(count: c_int, user_data: ?*anyopaque) callconv(.C) void {
        if (user_data) |ptr| {
            const self = @as(*CallbackHandler, @ptrCast(@alignCast(ptr)));
            const n = @as(usize, @intCast(count));
            
            // Move lines down by count
            var i = self.rows - 1;
            while (i >= n) : (i -= 1) {
                std.mem.copy(Cell, self.grid[i], self.grid[i - n]);
                if (i == n) break;  // Prevent underflow
            }
            
            // Clear the top lines
            i = 0;
            while (i < n and i < self.rows) : (i += 1) {
                for (self.grid[i]) |*cell| {
                    cell.char = ' ';
                    cell.attr = 0;
                }
            }
            
            std.debug.print("[ZIG] Scrolled down {} lines\n", .{n});
        }
    }
    
    // Handle flush request
    pub fn onFlush(user_data: ?*anyopaque) callconv(.C) void {
        if (user_data) |ptr| {
            const self = @as(*CallbackHandler, @ptrCast(@alignCast(ptr)));
            std.debug.print("[ZIG] Flush requested\n", .{});
            self.displayGrid();
        }
    }
    
    // ===========================================================================
    // Helper Functions
    // ===========================================================================
    
    // Display the current grid contents
    pub fn displayGrid(self: *const CallbackHandler) void {
        std.debug.print("\n=== Grid Display ({} x {}) ===\n", .{ self.rows, self.cols });
        
        for (self.grid, 0..) |row, r| {
            std.debug.print("{:2}: ", .{r});
            for (row) |cell| {
                if (cell.char >= 0x20 and cell.char <= 0x7E) {
                    std.debug.print("{c}", .{cell.char});
                } else {
                    std.debug.print(".", .{});
                }
            }
            std.debug.print("\n", .{});
        }
        
        std.debug.print("=== Cursor at ({},{}) ===\n", .{ self.cursor_row, self.cursor_col });
    }
    
    // Register callbacks with the UI Backend
    pub fn registerCallbacks(self: *CallbackHandler) void {
        const callbacks = UICallbacks{
            .on_cell = onCell,
            .on_clear_line = onClearLine,
            .on_clear_screen = onClearScreen,
            .on_insert_line = onInsertLine,
            .on_delete_line = onDeleteLine,
            .on_clear_eol = onClearEOL,
            .on_reverse_index = onReverseIndex,
            .on_line_feed = onLineFeed,
            .on_scroll_up = onScrollUp,
            .on_scroll_down = onScrollDown,
            .on_flush = onFlush,
        };
        
        ui_backend_set_callbacks(&callbacks, self);
        std.debug.print("[ZIG] Callbacks registered with UI Backend\n", .{});
    }
    
    // Check if UI backend is ready
    pub fn checkBackendStatus() bool {
        if (ui_backend_has_callbacks() != 0) {
            const status = ui_backend_get_status();
            std.debug.print("[ZIG] Backend status: {s}\n", .{status});
            return true;
        }
        return false;
    }
};

// ===========================================================================
// Public API Functions
// ===========================================================================

pub fn initializeUIBackend() void {
    ui_backend_init();
    std.debug.print("[ZIG] UI Backend initialized\n", .{});
}

pub fn isUIBackendEnabled() bool {
    return ui_backend_enabled() != 0;
}

pub fn requestFlush() void {
    ui_backend_flush();
}

// ===========================================================================
// Test Support
// ===========================================================================

test "CallbackHandler initialization" {
    const allocator = std.testing.allocator;
    
    var handler = try CallbackHandler.init(24, 80, allocator);
    defer handler.deinit();
    
    try std.testing.expectEqual(@as(usize, 24), handler.rows);
    try std.testing.expectEqual(@as(usize, 80), handler.cols);
    try std.testing.expectEqual(@as(usize, 0), handler.cursor_row);
    try std.testing.expectEqual(@as(usize, 0), handler.cursor_col);
}

test "Cell operations" {
    const allocator = std.testing.allocator;
    
    var handler = try CallbackHandler.init(10, 20, allocator);
    defer handler.deinit();
    
    // Test setting a cell
    handler.grid[0][0] = Cell{
        .char = 'A',
        .attr = 1,
        .fg = 0xFFFFFF,
        .bg = 0x000000,
    };
    
    try std.testing.expectEqual(@as(u8, 'A'), handler.grid[0][0].char);
}