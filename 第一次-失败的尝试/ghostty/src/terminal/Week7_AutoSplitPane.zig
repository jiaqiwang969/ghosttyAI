// Week7_AutoSplitPane.zig - 第七周：Ghostty启动时自动创建4分屏
// 这个文件将被集成到Ghostty，实现打开即显示4个pane

const std = @import("std");
const Terminal = @import("Terminal.zig");
const Screen = @import("Screen.zig");

const log = std.log.scoped(.auto_split);

/// 自动分屏配置
pub const AutoSplitConfig = struct {
    enabled: bool = true,
    layout: Layout = .quad,
    
    pub const Layout = enum {
        single,     // 不分屏
        dual_v,     // 左右2分屏
        dual_h,     // 上下2分屏  
        quad,       // 4分屏（2x2）
        triple,     // 3分屏
    };
};

/// Ghostty启动时自动创建4分屏
pub const AutoSplitPane = struct {
    terminal: *Terminal,
    config: AutoSplitConfig,
    panes: [4]Pane = undefined,
    active_pane: usize = 0,
    
    pub const Pane = struct {
        id: usize,
        x: f32,      // 0.0 - 1.0 (相对位置)
        y: f32,      // 0.0 - 1.0
        width: f32,  // 0.0 - 1.0 (相对宽度)
        height: f32, // 0.0 - 1.0
        screen: *Screen,
        title: []const u8,
        active: bool = false,
    };
    
    /// 初始化并自动创建4分屏
    pub fn init(allocator: std.mem.Allocator, terminal: *Terminal) !*AutoSplitPane {
        const self = try allocator.create(AutoSplitPane);
        
        self.* = .{
            .terminal = terminal,
            .config = .{
                .enabled = true,
                .layout = .quad,
            },
            .panes = undefined,
            .active_pane = 0,
        };
        
        // 自动创建4分屏布局
        try self.createQuadLayout(allocator);
        
        log.info("Auto 4-pane split created successfully!", .{});
        
        return self;
    }
    
    /// 创建4分屏布局（2x2）
    fn createQuadLayout(self: *AutoSplitPane, allocator: std.mem.Allocator) !void {
        // 左上角 - Pane 0
        self.panes[0] = .{
            .id = 0,
            .x = 0.0,
            .y = 0.0,
            .width = 0.5,
            .height = 0.5,
            .screen = try self.createPaneScreen(allocator),
            .title = "Pane 1: Development",
            .active = true,
        };
        
        // 右上角 - Pane 1
        self.panes[1] = .{
            .id = 1,
            .x = 0.5,
            .y = 0.0,
            .width = 0.5,
            .height = 0.5,
            .screen = try self.createPaneScreen(allocator),
            .title = "Pane 2: Testing",
        };
        
        // 左下角 - Pane 2
        self.panes[2] = .{
            .id = 2,
            .x = 0.0,
            .y = 0.5,
            .width = 0.5,
            .height = 0.5,
            .screen = try self.createPaneScreen(allocator),
            .title = "Pane 3: Logs",
        };
        
        // 右下角 - Pane 3
        self.panes[3] = .{
            .id = 3,
            .x = 0.5,
            .y = 0.5,
            .width = 0.5,
            .height = 0.5,
            .screen = try self.createPaneScreen(allocator),
            .title = "Pane 4: Terminal",
        };
        
        // 在每个pane显示初始内容
        try self.initializePaneContent();
        
        log.info("Created 4-pane layout (2x2 grid)", .{});
    }
    
    /// 为每个pane创建Screen
    fn createPaneScreen(self: *AutoSplitPane, allocator: std.mem.Allocator) !*Screen {
        _ = self;
        // Create a screen with proper initialization
        const screen = try Screen.init(
            allocator,
            40,  // Default cols for each pane
            12,  // Default rows for each pane 
            100, // Max scrollback
        );
        // Return pointer to the created screen
        const screen_ptr = try allocator.create(Screen);
        screen_ptr.* = screen;
        return screen_ptr;
    }
    
    /// 初始化每个pane的内容
    fn initializePaneContent(self: *AutoSplitPane) !void {
        // Pane 0: 显示欢迎信息
        try self.writeToPaneString(0, "╔═══════════════════════════════════╗");
        try self.writeToPaneString(0, "║  Ghostty with Native tmux         ║");
        try self.writeToPaneString(0, "║  Pane 1: Development Environment  ║");
        try self.writeToPaneString(0, "╚═══════════════════════════════════╝");
        try self.writeToPaneString(0, "");
        try self.writeToPaneString(0, "$ Ready for development...");
        
        // Pane 1: 测试信息
        try self.writeToPaneString(1, "╔═══════════════════════════════════╗");
        try self.writeToPaneString(1, "║  Pane 2: Test Runner              ║");
        try self.writeToPaneString(1, "╚═══════════════════════════════════╝");
        try self.writeToPaneString(1, "");
        try self.writeToPaneString(1, "Tests: 0 passed, 0 failed");
        
        // Pane 2: 日志
        try self.writeToPaneString(2, "╔═══════════════════════════════════╗");
        try self.writeToPaneString(2, "║  Pane 3: System Logs              ║");
        try self.writeToPaneString(2, "╚═══════════════════════════════════╝");
        try self.writeToPaneString(2, "");
        try self.writeToPaneString(2, "[INFO] Ghostty started with tmux");
        try self.writeToPaneString(2, "[INFO] 4-pane layout initialized");
        
        // Pane 3: 终端
        try self.writeToPaneString(3, "╔═══════════════════════════════════╗");
        try self.writeToPaneString(3, "║  Pane 4: Terminal                 ║");
        try self.writeToPaneString(3, "╚═══════════════════════════════════╝");
        try self.writeToPaneString(3, "");
        try self.writeToPaneString(3, "$ ");
    }
    
    /// 向指定pane写入字符串
    fn writeToPaneString(self: *AutoSplitPane, pane_id: usize, text: []const u8) !void {
        if (pane_id >= self.panes.len) return;
        
        log.debug("Writing to pane {}: {s}", .{ pane_id, text });
        
        // 这里应该将文本写入到对应pane的screen
        // 简化实现，实际需要调用Screen的写入方法
        _ = self.panes[pane_id].screen; // TODO: Actually write to the screen
    }
    
    /// 处理键盘输入（tmux风格）
    pub fn handleKeyInput(self: *AutoSplitPane, key: u8) !bool {
        _ = self; // TODO: Implement key handling
        // Ctrl-B prefix
        if (key == 0x02) { // Ctrl-B
            log.info("tmux prefix detected in 4-pane mode", .{});
            return true;
        }
        
        // 其他键盘命令...
        return false;
    }
    
    /// 渲染所有panes到terminal
    pub fn render(self: *AutoSplitPane) !void {
        // Get terminal dimensions
        const term_width = self.terminal.cols;
        const term_height = self.terminal.rows;
        
        for (self.panes, 0..) |pane, i| {
            const pixel_x = @as(u32, @intFromFloat(@as(f32, @floatFromInt(term_width)) * pane.x));
            const pixel_y = @as(u32, @intFromFloat(@as(f32, @floatFromInt(term_height)) * pane.y));
            const pixel_width = @as(u32, @intFromFloat(@as(f32, @floatFromInt(term_width)) * pane.width));
            const pixel_height = @as(u32, @intFromFloat(@as(f32, @floatFromInt(term_height)) * pane.height));
            
            // 绘制pane边框
            if (i == self.active_pane) {
                self.drawActiveBorder(pixel_x, pixel_y, pixel_width, pixel_height);
            } else {
                self.drawInactiveBorder(pixel_x, pixel_y, pixel_width, pixel_height);
            }
            
            // 渲染pane内容
            self.renderPaneContent(pane, pixel_x, pixel_y, pixel_width, pixel_height);
        }
        
        // 绘制状态栏
        self.drawStatusLine();
    }
    
    fn drawActiveBorder(self: *AutoSplitPane, x: u32, y: u32, w: u32, h: u32) void {
        _ = self;
        _ = x;
        _ = y; 
        _ = w;
        _ = h;
        // 绘制高亮边框
    }
    
    fn drawInactiveBorder(self: *AutoSplitPane, x: u32, y: u32, w: u32, h: u32) void {
        _ = self;
        _ = x;
        _ = y;
        _ = w;
        _ = h;
        // 绘制普通边框
    }
    
    fn renderPaneContent(self: *AutoSplitPane, pane: Pane, x: u32, y: u32, w: u32, h: u32) void {
        _ = self;
        _ = pane;
        _ = x;
        _ = y;
        _ = w;
        _ = h;
        // 渲染pane的screen内容
    }
    
    fn drawStatusLine(self: *AutoSplitPane) void {
        _ = self;
        // 绘制tmux风格状态栏
        // [0] 0:dev* 1:test- 2:log 3:term
    }
};