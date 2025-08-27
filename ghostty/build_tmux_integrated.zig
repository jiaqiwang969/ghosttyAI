// build_tmux_integrated.zig - Ghostty build with full tmux integration
const std = @import("std");

pub fn build(b: *std.Build) !void {
    // Standard build options
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    
    // Create the main executable
    const exe = b.addExecutable(.{
        .name = "ghostty-tmux",
        .root_source_file = .{ .path = "src/main.zig" },
        .target = target,
        .optimize = optimize,
    });
    
    // Add tmux integration module
    exe.addModule("tmux", b.addModule("tmux", .{
        .source_file = .{ .path = "src/tmux/tmux_integration.zig" },
    }));
    
    // Link with libtmuxcore
    exe.addLibraryPath(.{ .path = "../tmux" });
    exe.linkSystemLibrary("tmuxcore");
    
    // macOS specific settings
    if (target.isdarwin()) {
        exe.linkFramework("CoreFoundation");
        exe.linkFramework("CoreGraphics");
        exe.linkFramework("CoreText");
        exe.linkFramework("Metal");
        exe.linkFramework("MetalKit");
        exe.linkFramework("QuartzCore");
        exe.linkFramework("AppKit");
        exe.linkFramework("IOKit");
        
        // Set rpath for dynamic library loading
        exe.addRPath(.{ .path = "@executable_path/../Frameworks" });
        exe.addRPath(.{ .path = "@loader_path" });
    }
    
    // Define build flags
    exe.defineCMacro("GHOSTTY_TMUX_ENABLED", "1");
    exe.defineCMacro("LIBTMUXCORE_BUILD", "1");
    
    // Add include paths
    exe.addIncludePath(.{ .path = "../tmux" });
    exe.addIncludePath(.{ .path = "../tmux/ui_backend" });
    
    // Install the executable
    b.installArtifact(exe);
    
    // Create run command
    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }
    
    const run_step = b.step("run", "Run Ghostty with tmux integration");
    run_step.dependOn(&run_cmd.step);
    
    // Create test command
    const test_tmux = b.addTest(.{
        .root_source_file = .{ .path = "src/tmux/test_tmux_integration.zig" },
        .target = target,
        .optimize = optimize,
    });
    
    test_tmux.addLibraryPath(.{ .path = "../tmux" });
    test_tmux.linkSystemLibrary("tmuxcore");
    
    const test_step = b.step("test-tmux", "Test tmux integration");
    test_step.dependOn(&test_tmux.step);
    
    // Info step
    const info_step = b.step("info", "Show tmux integration info");
    const info_cmd = b.addSystemCommand(&.{
        "echo",
        "Ghostty×tmux Integration Build:",
        "\n  - libtmuxcore linked from ../tmux",
        "\n  - tmux module at src/tmux/",
        "\n  - UI callbacks enabled",
        "\n  - Ready for embedded terminal multiplexing",
    });
    info_step.dependOn(&info_cmd.step);
}