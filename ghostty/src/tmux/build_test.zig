const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    
    // Test executable
    const test_exe = b.addExecutable(.{
        .name = "test_ghostty_tmux",
        .root_source_file = .{ .path = "test_ghostty_tmux.zig" },
        .target = target,
        .optimize = optimize,
    });
    
    // Link with libtmuxcore
    test_exe.linkSystemLibrary("tmuxcore");
    test_exe.addLibraryPath(.{ .path = "../../.." });
    test_exe.addIncludePath(.{ .path = "../../.." });
    
    // Add Ghostty modules path
    test_exe.addIncludePath(.{ .path = ".." });
    
    b.installArtifact(test_exe);
    
    // Run command
    const run_cmd = b.addRunArtifact(test_exe);
    run_cmd.step.dependOn(b.getInstallStep());
    
    const run_step = b.step("run", "Run the test");
    run_step.dependOn(&run_cmd.step);
}
