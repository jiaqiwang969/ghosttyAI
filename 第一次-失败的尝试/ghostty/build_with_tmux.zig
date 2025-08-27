const std = @import("std");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    
    // 加载原始build
    const original_build = @import("build.zig");
    try original_build.build(b);
    
    // 找到ghostty可执行文件并添加tmux链接
    const exe_step = b.graph.findByName("ghostty");
    if (exe_step) |step| {
        if (step.cast(std.Build.Step.Compile)) |exe| {
            // 添加libtmuxcore链接
            exe.addLibraryPath(.{ .path = "../tmux" });
            exe.linkSystemLibrary("tmuxcore");
            exe.defineCMacro("GHOSTTY_TMUX_ENABLED", "1");
            std.log.info("Added tmux linking to ghostty", .{});
        }
    }
}
