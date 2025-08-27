// Termio_native_tmux_patch.zig - Native tmux integration for Termio
// This replaces the external libtmuxcore with native Zig implementation

// Add to imports in Termio.zig:
const tmux_native = @import("../terminal/tmux_native.zig");

// Replace the existing tmux_handle field with:
/// Native tmux implementation (no external dependencies!)
tmux_native: ?*tmux_native.TmuxNative = null,

// Modify the init function to initialize native tmux:
pub fn initNativeTmux(self: *Termio) !void {
    // Initialize native tmux implementation
    log.info("Initializing native tmux integration...", .{});
    
    self.tmux_native = try tmux_native.TmuxNative.init(
        self.alloc,
        &self.terminal
    );
    
    // Enable tmux mode on the terminal
    self.terminal.tmux_enabled = true;
    self.terminal.tmux = self.tmux_native;
    
    log.info("Native tmux integration enabled - Ctrl-B commands available", .{});
}

// Replace the queueWrite function with native tmux handling:
pub inline fn queueWriteWithNativeTmux(
    self: *Termio,
    td: *ThreadData,
    data: []const u8,
    linefeed: bool,
) !void {
    // Check if native tmux wants to handle the input
    if (self.tmux_native) |tmux| {
        for (data) |byte| {
            if (try tmux.processKey(byte)) {
                // Tmux handled the key, update display
                try self.terminal.renderTmux();
                
                // Notify renderer to update
                self.renderer_state.mutex.lock();
                defer self.renderer_state.mutex.unlock();
                self.terminal_stream.handler.queueRender() catch unreachable;
                
                return; // Don't send to PTY
            }
        }
    }
    
    // Normal PTY write if tmux didn't handle it
    try self.backend.queueWrite(self.alloc, td, data, linefeed);
}

// Add cleanup in deinit:
pub fn deinitNativeTmux(self: *Termio) void {
    if (self.tmux_native) |tmux| {
        tmux.deinit();
        self.tmux_native = null;
    }
}