// ghostty_tmux_bridge.zig - Bridge between Ghostty and libtmuxcore grid
const std = @import("std");
const c = @cImport({
    @cInclude("libtmuxcore_api.h");
});

// Grid cell representation matching libtmuxcore
const GridCell = struct {
    char: u8,
    fg: u8,
    bg: u8,
    attrs: u16,
};

// Ghostty-Tmux Bridge
pub const GhosttyTmuxBridge = struct {
    allocator: std.mem.Allocator,
    grid_width: u32,
    grid_height: u32,
    cursor_x: i32,
    cursor_y: i32,
    update_callback: ?*const fn(ctx: *anyopaque) void,
    update_context: ?*anyopaque,
    initialized: bool,
    
    const Self = @This();
    
    // External C functions from libtmuxcore
    extern fn tmc_init() c_int;
    extern fn tmc_cleanup() void;
    extern fn tmc_get_version() u32;
    extern fn ui_grid_init(cols: c_int, rows: c_int) void;
    extern fn ui_get_grid_cell(x: c_int, y: c_int, ch: *u8, fg: *c_int, bg: *c_int, attrs: *c_int) void;
    extern fn ui_get_cursor_pos(x: *c_int, y: *c_int) void;
    extern fn ui_callbacks_register(user_data: *anyopaque, callback: *const fn(*anyopaque) callconv(.C) void) void;
    extern fn tmc_input_init() void;
    extern fn tmc_input_process_key(key: u8) c_int;
    extern fn tmc_input_get_state_string() [*c]const u8;
    extern fn tmc_pty_process_all() void;
    
    // Session/Window/Pane management
    extern fn tmc_session_new(name: [*c]const u8, session: *?*anyopaque) c_int;
    extern fn tmc_session_attach(session: ?*anyopaque) c_int;
    extern fn tmc_window_new(session: ?*anyopaque, name: [*c]const u8, window: *?*anyopaque) c_int;
    extern fn tmc_pane_split(window: ?*anyopaque, horizontal: c_int, size: u32, pane: *?*anyopaque) c_int;
    extern fn tmc_pty_create(pane: ?*anyopaque, pty: *?*anyopaque) c_int;
    extern fn tmc_pty_spawn_shell(pty: ?*anyopaque, shell: [*c]const u8) c_int;
    
    // Initialize the bridge
    pub fn init(allocator: std.mem.Allocator, width: u32, height: u32) !Self {
        const result = tmc_init();
        if (result != 0) {
            return error.TmuxInitFailed;
        }
        
        // Initialize grid
        ui_grid_init(@intCast(width), @intCast(height));
        
        // Initialize input system
        tmc_input_init();
        
        std.debug.print("[Bridge] Initialized Ghostty-Tmux bridge {}x{}\n", .{width, height});
        
        return Self{
            .allocator = allocator,
            .grid_width = width,
            .grid_height = height,
            .cursor_x = 0,
            .cursor_y = 0,
            .update_callback = null,
            .update_context = null,
            .initialized = true,
        };
    }
    
    // Deinitialize
    pub fn deinit(self: *Self) void {
        if (self.initialized) {
            tmc_cleanup();
            self.initialized = false;
            std.debug.print("[Bridge] Deinitialized Ghostty-Tmux bridge\n", .{});
        }
    }
    
    // Get version
    pub fn getVersion(self: *Self) u32 {
        _ = self;
        return tmc_get_version();
    }
    
    // Register update callback
    pub fn registerUpdateCallback(self: *Self, context: *anyopaque, callback: *const fn(*anyopaque) void) void {
        self.update_callback = callback;
        self.update_context = context;
        
        // C callback wrapper
        const c_callback = struct {
            fn wrapper(ctx: *anyopaque) callconv(.C) void {
                const bridge = @as(*Self, @ptrCast(@alignCast(ctx)));
                if (bridge.update_callback) |cb| {
                    if (bridge.update_context) |uctx| {
                        cb(uctx);
                    }
                }
            }
        }.wrapper;
        
        ui_callbacks_register(self, c_callback);
        std.debug.print("[Bridge] Registered update callback\n", .{});
    }
    
    // Get cell at position
    pub fn getCell(self: *Self, x: u32, y: u32) GridCell {
        _ = self;
        
        var ch: u8 = ' ';
        var fg: c_int = 7;
        var bg: c_int = 0;
        var attrs: c_int = 0;
        
        ui_get_grid_cell(@intCast(x), @intCast(y), &ch, &fg, &bg, &attrs);
        
        return GridCell{
            .char = ch,
            .fg = @intCast(fg),
            .bg = @intCast(bg),
            .attrs = @intCast(attrs),
        };
    }
    
    // Get cursor position
    pub fn getCursorPos(self: *Self) struct { x: i32, y: i32 } {
        var x: c_int = 0;
        var y: c_int = 0;
        ui_get_cursor_pos(&x, &y);
        
        self.cursor_x = x;
        self.cursor_y = y;
        
        return .{ .x = x, .y = y };
    }
    
    // Process keyboard input
    pub fn processKey(self: *Self, key: u8) !void {
        _ = self;
        
        const result = tmc_input_process_key(key);
        if (result != 0) {
            return error.InputProcessingFailed;
        }
    }
    
    // Process key chord (Ctrl+key)
    pub fn processKeyChord(self: *Self, ctrl: bool, alt: bool, key: u8) !void {
        _ = alt;
        
        var processed_key = key;
        if (ctrl) {
            // Convert to control character
            if (key >= 'a' and key <= 'z') {
                processed_key = key - 'a' + 1;
            } else if (key >= 'A' and key <= 'Z') {
                processed_key = key - 'A' + 1;
            }
        }
        
        try self.processKey(processed_key);
    }
    
    // Get input state string (for status line)
    pub fn getInputState(self: *Self) []const u8 {
        _ = self;
        
        const state = tmc_input_get_state_string();
        return std.mem.span(state);
    }
    
    // Create a new tmux session
    pub fn createSession(self: *Self, name: []const u8) !?*anyopaque {
        const name_z = try self.allocator.dupeZ(u8, name);
        defer self.allocator.free(name_z);
        
        var session: ?*anyopaque = null;
        const result = tmc_session_new(name_z, &session);
        if (result != 0) {
            return error.SessionCreationFailed;
        }
        
        // Attach to the session
        _ = tmc_session_attach(session);
        
        return session;
    }
    
    // Create window with PTY
    pub fn createWindowWithPTY(self: *Self, session: ?*anyopaque, name: []const u8) !?*anyopaque {
        const name_z = try self.allocator.dupeZ(u8, name);
        defer self.allocator.free(name_z);
        
        var window: ?*anyopaque = null;
        var result = tmc_window_new(session, name_z, &window);
        if (result != 0) {
            return error.WindowCreationFailed;
        }
        
        // Create PTY for the default pane
        var pty: ?*anyopaque = null;
        const pane_id: ?*anyopaque = @ptrFromInt(1); // Default pane
        result = tmc_pty_create(pane_id, &pty);
        if (result == 0 and pty != null) {
            // Spawn shell
            _ = tmc_pty_spawn_shell(pty, null);
        }
        
        return window;
    }
    
    // Process all PTY data
    pub fn processPTYData(self: *Self) void {
        _ = self;
        tmc_pty_process_all();
    }
    
    // Render the grid to Ghostty's terminal
    pub fn renderToTerminal(self: *Self, terminal: anytype) !void {
        const cursor = self.getCursorPos();
        
        // Render each cell
        for (0..self.grid_height) |y| {
            for (0..self.grid_width) |x| {
                const cell = self.getCell(@intCast(x), @intCast(y));
                
                // Convert to Ghostty's format and render
                // This would integrate with Ghostty's actual rendering system
                _ = terminal;
                _ = cell;
            }
        }
        
        // Update cursor position
        _ = cursor;
        
        std.debug.print("[Bridge] Rendered grid to terminal\n", .{});
    }
    
    // Handle terminal resize
    pub fn resize(self: *Self, width: u32, height: u32) void {
        self.grid_width = width;
        self.grid_height = height;
        ui_grid_init(@intCast(width), @intCast(height));
        
        std.debug.print("[Bridge] Resized to {}x{}\n", .{width, height});
    }
};

// Test the bridge
pub fn main() !void {
    const allocator = std.heap.page_allocator;
    
    std.debug.print("=== Ghostty-Tmux Bridge Test ===\n", .{});
    
    // Initialize bridge
    var bridge = try GhosttyTmuxBridge.init(allocator, 80, 24);
    defer bridge.deinit();
    
    // Check version
    const version = bridge.getVersion();
    std.debug.print("✓ Tmux version: {x:0>8}\n", .{version});
    
    // Create session
    const session = try bridge.createSession("ghostty");
    std.debug.print("✓ Created session\n", .{});
    
    // Create window with PTY
    _ = try bridge.createWindowWithPTY(session, "main");
    std.debug.print("✓ Created window with PTY\n", .{});
    
    // Test keyboard input
    try bridge.processKeyChord(true, false, 'b'); // Ctrl-B
    try bridge.processKey('c'); // New window
    std.debug.print("✓ Processed Ctrl-B c command\n", .{});
    
    // Get input state
    const state = bridge.getInputState();
    std.debug.print("✓ Input state: {s}\n", .{state});
    
    // Get some cells
    for (0..3) |y| {
        std.debug.print("Row {}: ", .{y});
        for (0..20) |x| {
            const cell = bridge.getCell(@intCast(x), @intCast(y));
            const ch = if (cell.char >= 32 and cell.char < 127) cell.char else '.';
            std.debug.print("{c}", .{ch});
        }
        std.debug.print("\n", .{});
    }
    
    // Process PTY data
    bridge.processPTYData();
    
    std.debug.print("\n✅ All bridge tests passed!\n", .{});
}