// Terminal_tmux_integration_patch.zig - Patch to add native tmux to Terminal
// This file shows the changes needed to Terminal.zig

// Add these imports at the top of Terminal.zig:
const tmux_native = @import("tmux_native.zig");

// Add these fields to Terminal struct (around line 100):
// Native tmux implementation
tmux: ?*tmux_native.TmuxNative = null,

// Whether tmux mode is enabled
tmux_enabled: bool = false,

// Add this initialization in Terminal.init():
pub fn initWithTmux(self: *Terminal, allocator: Allocator) !void {
    // Initialize native tmux if enabled
    if (self.tmux_enabled) {
        self.tmux = try tmux_native.TmuxNative.init(allocator, self);
        log.info("Native tmux initialized", .{});
    }
}

// Add this to Terminal.deinit():
pub fn deinitTmux(self: *Terminal) void {
    if (self.tmux) |tmux| {
        tmux.deinit();
        self.tmux = null;
    }
}

// Modify keyboard input processing to handle tmux:
pub fn processKeyboardInput(self: *Terminal, key: u8) !bool {
    // First check if tmux wants to handle this key
    if (self.tmux) |tmux| {
        if (try tmux.processKey(key)) {
            // tmux handled the key, trigger re-render
            try self.renderTmux();
            return true;
        }
    }
    
    // Normal terminal processing continues...
    return false;
}

// Add tmux rendering support:
pub fn renderTmux(self: *Terminal) !void {
    if (self.tmux) |tmux| {
        // Clear screen first
        self.screen.clear();
        
        // Render all tmux panes
        try tmux.render();
        
        // Mark dirty for renderer
        self.dirty = true;
    }
}

// Add helper to check if we're in tmux mode:
pub fn isInTmuxMode(self: *const Terminal) bool {
    return self.tmux != null and self.tmux_enabled;
}

// Add method to enable/disable tmux:
pub fn setTmuxMode(self: *Terminal, enabled: bool, allocator: Allocator) !void {
    if (enabled and self.tmux == null) {
        self.tmux_enabled = true;
        self.tmux = try tmux_native.TmuxNative.init(allocator, self);
        log.info("Tmux mode enabled", .{});
    } else if (!enabled and self.tmux != null) {
        self.tmux_enabled = false;
        if (self.tmux) |tmux| {
            tmux.deinit();
            self.tmux = null;
        }
        log.info("Tmux mode disabled", .{});
    }
}