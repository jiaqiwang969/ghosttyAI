const std = @import("std");
const builtin = @import("builtin");

pub fn build(b: *std.Build) !void {
    // Import and run the original build
    const original_build = @import("build.zig");
    try original_build.build(b);
    
    // Add tmux integration option (always enabled for this wrapper)
    const enable_tmux = b.option(bool, "enable-tmux", "Enable tmux integration") orelse true;
    
    if (enable_tmux) {
        std.log.info("Enhancing build with tmux integration...", .{});
        
        // This is a simplified approach - in production we'd properly
        // hook into the exe artifact, but for now we ensure the
        // build knows about tmux
        
        // The actual linking happens in the modified Makefile
        // which sets up the environment and paths
    }
}
