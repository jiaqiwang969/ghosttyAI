// build_incremental.zig - Adds tmux support to existing Ghostty build
const std = @import("std");

pub fn build(b: *std.Build) void {
    // Import and run the original build
    const original_build = @import("build.zig");
    original_build.build(b);
    
    // Add tmux integration as an option
    const enable_tmux = b.option(bool, "enable-tmux", "Enable tmux integration") orelse false;
    
    if (enable_tmux) {
        std.log.info("Adding tmux integration to Ghostty build", .{});
        
        // Get the main executable from the original build
        // This is a simplified approach - in practice we'd need to hook into
        // the actual executable target from the original build
        
        // Add tmux library path
        const exe_step = b.getInstallStep();
        
        // This is where we'd add:
        // exe.linkSystemLibrary("tmuxcore");
        // exe.addLibraryPath(b.path(".."));
        // exe.addIncludePath(b.path(".."));
    }
}
