// Patch to add to build.zig after line 12 (after config init)
// This adds tmux integration support

    // Add tmux integration option (default: true for direct integration)
    const enable_tmux = b.option(bool, "enable-tmux", "Enable tmux integration") orelse true;
    
    // Add build options for conditional compilation
    const options = b.addOptions();
    options.addOption(bool, "enable_tmux", enable_tmux);
    
    if (enable_tmux) {
        std.log.info("Building Ghostty with tmux integration enabled", .{});
    }
