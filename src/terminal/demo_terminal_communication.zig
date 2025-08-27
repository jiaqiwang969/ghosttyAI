const std = @import("std");
const SessionManager = @import("SessionManager.zig");

/// 演示Ghostty终端间通信功能
pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();
    
    std.log.info("=== Ghostty Terminal Communication Demo ===", .{});
    
    // 创建SessionManager
    var manager = SessionManager.init(allocator);
    defer manager.deinit();
    
    // 模拟Surface指针（实际应用中是真实的Surface）
    var surface_a: u32 = 1;
    var surface_b: u32 = 2;
    var surface_remote: u32 = 3;
    
    // 场景1: 本地终端间通信
    try demo_local_communication(&manager, &surface_a, &surface_b);
    
    // 场景2: SSH远程通信
    try demo_ssh_communication(&manager, &surface_a, &surface_remote);
    
    // 场景3: 多终端协作
    try demo_multi_terminal_collaboration(&manager);
    
    std.log.info("Demo completed successfully!", .{});
}

/// 演示本地终端间通信
fn demo_local_communication(
    manager: *SessionManager,
    surface_a: *u32,
    surface_b: *u32,
) !void {
    std.log.info("\n--- Demo 1: Local Terminal Communication ---", .{});
    
    // 注册两个本地终端
    try manager.registerSession("terminal-a", surface_a, false);
    try manager.registerSession("terminal-b", surface_b, false);
    
    // A发送命令到B
    std.log.info("Terminal A sending command to Terminal B...", .{});
    try manager.sendToSession(
        "terminal-a",
        "terminal-b",
        "echo 'Hello from Terminal A'\n",
        false
    );
    
    // 建立双向链接
    std.log.info("Establishing bidirectional link...", .{});
    try manager.linkSessions("terminal-a", "terminal-b", true);
    
    // 模拟输出路由
    std.log.info("Routing output from Terminal A...", .{});
    try manager.routeOutput("terminal-a", "$ ls -la\ntotal 48\ndrwxr-xr-x  6 user  staff  192\n");
    
    // 显示会话状态
    const stdout = std.io.getStdOut().writer();
    try manager.listSessions(stdout);
}

/// 演示SSH远程通信
fn demo_ssh_communication(
    manager: *SessionManager,
    surface_local: *u32,
    surface_remote: *u32,
) !void {
    std.log.info("\n--- Demo 2: SSH Remote Communication ---", .{});
    
    // 注册远程会话
    try manager.registerSession("remote-server", surface_remote, true);
    
    // 建立本地到远程的链接
    std.log.info("Linking local to remote session...", .{});
    try manager.linkSessions("terminal-a", "remote-server", true);
    
    // 发送命令到远程服务器
    std.log.info("Sending command to remote server...", .{});
    try manager.sendToSession(
        "terminal-a",
        "remote-server",
        "uname -a\n",
        true  // 等待响应
    );
    
    // 模拟远程响应
    std.log.info("Remote server responding...", .{});
    try manager.sendToSession(
        "remote-server",
        "terminal-a",
        "Linux remote-server 5.15.0 x86_64 GNU/Linux\n",
        false
    );
}

/// 演示多终端协作场景
fn demo_multi_terminal_collaboration(manager: *SessionManager) !void {
    std.log.info("\n--- Demo 3: Multi-Terminal Collaboration ---", .{});
    
    // 创建一个开发环境：编辑器、编译器、日志监控
    var editor: u32 = 10;
    var compiler: u32 = 11;
    var logger: u32 = 12;
    
    try manager.registerSession("editor", &editor, false);
    try manager.registerSession("compiler", &compiler, false);
    try manager.registerSession("logger", &logger, false);
    
    // 编辑器 -> 编译器（单向）
    try manager.linkSessions("editor", "compiler", false);
    
    // 编译器 -> 日志（单向）
    try manager.linkSessions("compiler", "logger", false);
    
    // 编辑器 <-> 日志（双向，用于错误反馈）
    try manager.linkSessions("editor", "logger", true);
    
    std.log.info("Development environment setup:", .{});
    std.log.info("  editor -> compiler (on save)", .{});
    std.log.info("  compiler -> logger (output)", .{});
    std.log.info("  editor <-> logger (error feedback)", .{});
    
    // 模拟工作流
    std.log.info("\nSimulating workflow...", .{});
    
    // 1. 编辑器保存文件
    std.log.info("Editor: File saved", .{});
    try manager.sendToSession("editor", "compiler", ":w main.zig\n", false);
    
    // 2. 编译器自动编译
    std.log.info("Compiler: Building...", .{});
    try manager.sendToSession("compiler", "logger", "zig build\n", false);
    
    // 3. 日志显示结果
    std.log.info("Logger: Build successful", .{});
    try manager.sendToSession("logger", "editor", "✅ Build completed\n", false);
    
    // 显示最终状态
    const stdout = std.io.getStdOut().writer();
    try stdout.print("\n", .{});
    try manager.listSessions(stdout);
}

/// 命令行接口演示
pub fn demo_cli_interface() !void {
    std.log.info("\n--- Command Line Interface Demo ---", .{});
    
    // 展示用户如何使用命令行接口
    const commands =
        \\# 启动Ghostty with session ID
        \\$ ghostty --session-id dev-terminal
        \\
        \\# 发送命令到另一个终端
        \\$ ghostty send build-terminal "make clean && make"
        \\
        \\# 建立终端链接
        \\$ ghostty link log-terminal --bidirectional
        \\
        \\# 列出所有会话
        \\$ ghostty sessions
        \\
        \\# SSH with automatic session setup
        \\$ ghostty ssh user@remote --link-back
        \\
        \\# 在终端内使用特殊命令
        \\$ @send other-terminal "echo 'Direct message'"
        \\$ @link remote-session
        \\$ @unlink build-terminal
        \\$ @sessions
        \\
    ;
    
    std.log.info("Available commands:\n{s}", .{commands});
}

/// OSC序列通信协议示例
pub fn demo_osc_protocol() !void {
    std.log.info("\n--- OSC Sequence Protocol Demo ---", .{});
    
    // OSC 777 - Ghostty专用通信序列
    const osc_examples =
        \\# Setup communication channel
        \\ESC]777;ghostty-comm;setup;session-id\a
        \\
        \\# Send data to remote
        \\ESC]777;ghostty-comm;data;target-id;base64-encoded-data\a
        \\
        \\# Register remote session
        \\ESC]777;ghostty-comm;register;local-id;remote-id\a
        \\
        \\# Query session info
        \\ESC]777;ghostty-comm;query;sessions\a
        \\
        \\# Response format
        \\ESC]777;ghostty-comm;response;type;data\a
        \\
    ;
    
    std.log.info("OSC Protocol Examples:\n{s}", .{osc_examples});
}

test "demo runs without errors" {
    try main();
    try demo_cli_interface();
    try demo_osc_protocol();
}