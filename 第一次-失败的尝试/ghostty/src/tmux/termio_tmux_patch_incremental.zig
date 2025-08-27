// termio_tmux_patch_incremental.zig - Incremental patch to add tmux to Termio
// Purpose: Show how to add tmux support to Termio.zig incrementally
// Date: 2025-08-26
// Task: T-503 - Incremental integration approach

// This file demonstrates the minimal changes needed to add tmux support
// to the existing Termio.zig without breaking the current build.

// ============ ADDITIONS TO TERMIO.ZIG ============

// 1. Add this import at the top of Termio.zig (around line 30):
// But ONLY when tmux feature is enabled via build flag
const build_options = @import("build_options");
const tmux_enabled = build_options.enable_tmux or false;

const tmux_integration = if (tmux_enabled) 
    @import("../tmux/termio_tmux_integration.zig") 
    else 
    struct {};

// 2. Add this field to the Termio struct (around line 70):
// This is conditional - only exists when tmux is enabled
tmux_ext: if (tmux_enabled) ?*tmux_integration.TmuxExtension else void,

// 3. Add to the init function after terminal creation (around line 310):
// This is the incremental integration point
pub fn initWithOptionalTmux(self: *Termio, alloc: Allocator, opts: termio.Options) !void {
    // ... existing init code to create terminal ...
    
    // After line with: .terminal = term,
    // Add this conditional tmux initialization:
    if (tmux_enabled) {
        // Check if config wants tmux
        const tmux_config = tmux_integration.TmuxConfig{
            .enable_tmux = opts.full_config.@"enable-tmux" orelse false,
            .default_session_name = opts.full_config.@"tmux-session-name" orelse "main",
        };
        
        if (tmux_config.enable_tmux) {
            var tmux_ext = try alloc.create(tmux_integration.TmuxExtension);
            tmux_ext.* = try tmux_integration.TmuxExtension.init(alloc, tmux_config);
            
            // Attach to terminal
            try tmux_ext.attachToTerminal(&term);
            
            self.tmux_ext = tmux_ext;
            std.log.info("tmux integration enabled", .{});
        } else {
            self.tmux_ext = null;
        }
    }
    
    // ... rest of existing init code ...
}

// 4. Add to the deinit function (wherever Terminal cleanup happens):
pub fn deinitWithTmux(self: *Termio) void {
    // ... existing deinit code ...
    
    if (tmux_enabled) {
        if (self.tmux_ext) |ext| {
            ext.deinit(self.alloc);
            self.alloc.destroy(ext);
        }
    }
}

// 5. Add method to handle tmux commands (new method):
pub fn handleTmuxCommand(self: *Termio, command: []const u8) !void {
    if (!tmux_enabled) {
        return error.TmuxNotEnabled;
    }
    
    if (self.tmux_ext) |ext| {
        try ext.handleCommand(command);
    } else {
        return error.TmuxNotInitialized;
    }
}

// 6. In processInput, check for tmux commands (modify existing):
pub fn processInputWithTmux(self: *Termio, data: []const u8) !void {
    // Check if this is a tmux command
    if (tmux_enabled and std.mem.startsWith(u8, data, "@tmux ")) {
        const cmd = data[6..];
        try self.handleTmuxCommand(cmd);
        return;
    }
    
    // Normal input processing continues as before...
    // ... existing processInput code ...
}

// ============ BUILD CONFIGURATION ============

// In build.zig, add this option:
const enable_tmux = b.option(bool, "enable-tmux", "Enable tmux integration") orelse false;

// Pass it to the exe:
exe.addOptions("build_options", .{
    .enable_tmux = enable_tmux,
});

// If tmux is enabled, link the library:
if (enable_tmux) {
    exe.linkSystemLibrary("tmuxcore");
    exe.addLibraryPath(.{ .path = ".." });
    exe.addIncludePath(.{ .path = ".." });
}

// ============ USAGE ============

// Build without tmux (default, unchanged):
// zig build

// Build with tmux integration:
// zig build -Denable-tmux=true

// ============ BENEFITS OF THIS APPROACH ============

// 1. Zero impact on existing build when tmux is disabled
// 2. All tmux code is conditionally compiled out when not needed
// 3. Can be tested incrementally without breaking production
// 4. Easy to remove if needed - just delete the tmux modules
// 5. Configuration-driven - users opt-in to tmux support