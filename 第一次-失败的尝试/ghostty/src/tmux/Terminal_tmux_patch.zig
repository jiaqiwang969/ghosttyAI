// Terminal_tmux_patch.zig - 补丁文件：为Ghostty Terminal添加tmux支持
// 应用到: ghostty/src/terminal/Terminal.zig

const std = @import("std");
const TmuxIntegration = @import("../tmux/integration.zig").TmuxIntegration;

// 在Terminal结构体中添加tmux字段
pub const Terminal = struct {
    // ... 原有字段 ...
    
    // 添加tmux集成
    tmux: TmuxIntegration,
    tmux_enabled: bool,
    
    // ... 其他字段 ...
    
    // 修改init函数
    pub fn init(self: *Terminal, allocator: std.mem.Allocator, config: Config) !void {
        // ... 原有初始化代码 ...
        
        // 初始化tmux
        self.tmux = TmuxIntegration{};
        self.tmux_enabled = config.enable_tmux orelse false;
        
        if (self.tmux_enabled) {
            try self.tmux.init();
            std.log.info("Tmux integration enabled in Terminal", .{});
        }
        
        // ... 继续初始化 ...
    }
    
    // 修改键盘输入处理
    pub fn processKeyEvent(self: *Terminal, event: KeyEvent) !void {
        // 如果tmux启用，先让tmux处理
        if (self.tmux_enabled) {
            const ctrl = event.mods.ctrl;
            const key = event.key;
            
            // 检查Ctrl-B
            if (ctrl and key == 'b') {
                try self.tmux.processKey(0x02, true);
                
                // 如果是tmux前缀键，阻止默认处理
                if (self.tmux.prefix_active) {
                    return;
                }
            }
            
            // 如果前缀键激活，所有输入发送到tmux
            if (self.tmux.prefix_active) {
                try self.tmux.processKey(@intCast(key), ctrl);
                
                // 检查是否还在前缀模式
                if (!self.tmux.isPrefixActive()) {
                    self.tmux.prefix_active = false;
                }
                return;
            }
        }
        
        // ... 原有键盘处理代码 ...
    }
    
    // 修改渲染函数
    pub fn render(self: *Terminal, surface: *Surface) !void {
        // 如果tmux启用，从tmux获取内容
        if (self.tmux_enabled) {
            // 从tmux grid渲染到terminal grid
            self.tmux.renderToTerminal(self);
        }
        
        // ... 原有渲染代码 ...
    }
    
    // 在write函数中检查tmux
    pub fn write(self: *Terminal, data: []const u8) !void {
        // 如果tmux启用且不在前缀模式，数据发送到当前PTY
        if (self.tmux_enabled and !self.tmux.prefix_active) {
            self.tmux.writeToPTY(data);
            return;
        }
        
        // ... 原有写入代码 ...
    }
    
    // 添加辅助函数：写入单元格（供tmux使用）
    pub fn writeCell(self: *Terminal, x: usize, y: usize, ch: u8, fg: i32, bg: i32) void {
        if (x >= self.cols or y >= self.rows) return;
        
        var cell = &self.grid[y][x];
        cell.char = ch;
        cell.fg = @intCast(fg);
        cell.bg = @intCast(bg);
        
        // 标记需要重绘
        self.dirty = true;
    }
    
    // 清理时释放tmux
    pub fn deinit(self: *Terminal) void {
        if (self.tmux_enabled) {
            self.tmux.deinit();
        }
        
        // ... 原有清理代码 ...
    }
};