# Day 3 - Ghostty集成实现

## 🎯 Day 3 目标
将libtmuxcore完全集成到Ghostty中，实现FFI桥接，使tmux功能在Ghostty内原生可用。

## 📋 详细任务清单

### Task 3.1: 更新Ghostty构建系统 (1小时)

#### 修改文件
`/Users/jqwang/98-ghosttyAI/ghostty/build.zig`

```zig
const std = @import("std");
const Builder = std.build.Builder;

pub fn build(b: *Builder) void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardReleaseOptions();

    const exe = b.addExecutable("ghostty", "src/main.zig");
    exe.setTarget(target);
    exe.setBuildMode(mode);

    // 链接系统库
    exe.linkSystemLibrary("c");
    exe.linkSystemLibrary("objc");
    exe.linkFramework("CoreFoundation");
    exe.linkFramework("CoreGraphics");
    exe.linkFramework("CoreText");
    exe.linkFramework("AppKit");

    // 🆕 链接libtmuxcore
    exe.addLibPath("../tmux");
    exe.linkSystemLibrary("tmuxcore");
    exe.addIncludePath("../tmux");
    exe.addIncludePath("../tmux/ui_backend");

    // 添加tmux模块路径
    exe.addPackagePath("tmux", "src/tmux/tmux_integration.zig");

    // 设置运行时库路径
    exe.addRPath("@executable_path/../Frameworks");
    exe.addRPath("@loader_path");

    exe.install();
}
```

### Task 3.2: 实现完整FFI桥接 (2.5小时)

#### 修改文件
`/Users/jqwang/98-ghosttyAI/ghostty/src/tmux/ffi_bridge.zig`

```zig
const std = @import("std");
const c = @import("c_types.zig");

// 导入libtmuxcore C API
pub extern "c" fn tmc_init() c.tmc_error_t;
pub extern "c" fn tmc_cleanup() void;

pub extern "c" fn tmc_session_new(name: [*c]const u8, session: *c.tmc_session_t) c.tmc_error_t;
pub extern "c" fn tmc_session_attach(session: c.tmc_session_t) c.tmc_error_t;
pub extern "c" fn tmc_session_detach(session: c.tmc_session_t) c.tmc_error_t;
pub extern "c" fn tmc_session_current() c.tmc_session_t;

pub extern "c" fn tmc_window_new(session: c.tmc_session_t, name: [*c]const u8, window: *c.tmc_window_t) c.tmc_error_t;
pub extern "c" fn tmc_window_close(window: c.tmc_window_t) c.tmc_error_t;
pub extern "c" fn tmc_window_select(window: c.tmc_window_t) c.tmc_error_t;
pub extern "c" fn tmc_window_current() c.tmc_window_t;

pub extern "c" fn tmc_pane_split(window: c.tmc_window_t, horizontal: bool, size_percent: u32, new_pane: *c.tmc_pane_t) c.tmc_error_t;
pub extern "c" fn tmc_pane_close(pane: c.tmc_pane_t) c.tmc_error_t;
pub extern "c" fn tmc_pane_resize(pane: c.tmc_pane_t, width: u32, height: u32) c.tmc_error_t;
pub extern "c" fn tmc_pane_select(pane: c.tmc_pane_t) c.tmc_error_t;

pub extern "c" fn tmc_command_execute(command: [*c]const u8) c.tmc_error_t;
pub extern "c" fn tmc_command_send_keys(pane: c.tmc_pane_t, keys: [*c]const u8) c.tmc_error_t;

// UI Backend注册
pub extern "c" fn ui_backend_register(vtable: *c.ui_backend_vtable) void;
pub extern "c" fn ui_backend_unregister() void;

// Zig安全包装
pub const TmuxCore = struct {
    const Self = @This();
    
    initialized: bool = false,
    vtable: ?*c.ui_backend_vtable = null,
    allocator: std.mem.Allocator,
    
    pub fn init(allocator: std.mem.Allocator) !Self {
        const result = tmc_init();
        if (result != c.TMC_SUCCESS) {
            return error.TmuxInitFailed;
        }
        
        return Self{
            .initialized = true,
            .allocator = allocator,
        };
    }
    
    pub fn deinit(self: *Self) void {
        if (self.initialized) {
            if (self.vtable) |vtable| {
                ui_backend_unregister();
                self.allocator.destroy(vtable);
            }
            tmc_cleanup();
            self.initialized = false;
        }
    }
    
    pub fn registerBackend(self: *Self, callbacks: TmuxCallbacks) !void {
        // 创建vtable
        const vtable = try self.allocator.create(c.ui_backend_vtable);
        errdefer self.allocator.destroy(vtable);
        
        // 设置回调函数
        vtable.* = .{
            .handle_output = handleOutputWrapper,
            .write_cell = writeCellWrapper,
            .move_cursor = moveCursorWrapper,
            .show_cursor = showCursorWrapper,
            .clear_screen = clearScreenWrapper,
            .clear_line = clearLineWrapper,
            .scroll_region = scrollRegionWrapper,
            // ... 其他回调
        };
        
        // 保存回调上下文
        callback_context = callbacks;
        
        // 注册到tmux
        ui_backend_register(vtable);
        self.vtable = vtable;
    }
    
    // 会话管理
    pub fn newSession(self: *Self, name: []const u8) !Session {
        var session: c.tmc_session_t = undefined;
        const c_name = try self.allocator.dupeZ(u8, name);
        defer self.allocator.free(c_name);
        
        const result = tmc_session_new(c_name.ptr, &session);
        if (result != c.TMC_SUCCESS) {
            return error.SessionCreationFailed;
        }
        
        return Session{ .handle = session };
    }
    
    // 窗口管理
    pub fn newWindow(self: *Self, session: Session, name: []const u8) !Window {
        var window: c.tmc_window_t = undefined;
        const c_name = try self.allocator.dupeZ(u8, name);
        defer self.allocator.free(c_name);
        
        const result = tmc_window_new(session.handle, c_name.ptr, &window);
        if (result != c.TMC_SUCCESS) {
            return error.WindowCreationFailed;
        }
        
        return Window{ .handle = window };
    }
    
    // 窗格分割
    pub fn splitPane(self: *Self, window: Window, horizontal: bool, size_percent: u32) !Pane {
        var new_pane: c.tmc_pane_t = undefined;
        
        const result = tmc_pane_split(window.handle, horizontal, size_percent, &new_pane);
        if (result != c.TMC_SUCCESS) {
            return error.PaneSplitFailed;
        }
        
        return Pane{ .handle = new_pane };
    }
};

// 句柄包装类型
pub const Session = struct {
    handle: c.tmc_session_t,
    
    pub fn attach(self: Session) !void {
        const result = tmc_session_attach(self.handle);
        if (result != c.TMC_SUCCESS) {
            return error.SessionAttachFailed;
        }
    }
};

pub const Window = struct {
    handle: c.tmc_window_t,
    
    pub fn close(self: Window) !void {
        const result = tmc_window_close(self.handle);
        if (result != c.TMC_SUCCESS) {
            return error.WindowCloseFailed;
        }
    }
};

pub const Pane = struct {
    handle: c.tmc_pane_t,
    
    pub fn sendKeys(self: Pane, keys: []const u8) !void {
        const c_keys = try std.cstr.addNullByte(std.heap.c_allocator, keys);
        defer std.heap.c_allocator.free(c_keys);
        
        const result = tmc_command_send_keys(self.handle, c_keys.ptr);
        if (result != c.TMC_SUCCESS) {
            return error.SendKeysFailed;
        }
    }
};
```

### Task 3.3: 集成到Terminal模块 (2小时)

#### 修改文件
`/Users/jqwang/98-ghosttyAI/ghostty/src/terminal/Terminal.zig`

```zig
const std = @import("std");
const tmux = @import("../tmux/tmux_integration.zig");

pub const Terminal = struct {
    const Self = @This();
    
    // 原有字段
    grid: Grid,
    cursor: Cursor,
    // ...
    
    // 🆕 tmux集成
    tmux_enabled: bool = false,
    tmux_core: ?tmux.TmuxCore = null,
    current_session: ?tmux.Session = null,
    current_window: ?tmux.Window = null,
    current_pane: ?tmux.Pane = null,
    
    pub fn init(allocator: std.mem.Allocator, config: Config) !Self {
        var self = Self{
            .grid = try Grid.init(allocator, config.cols, config.rows),
            .cursor = Cursor.init(),
            // ... 其他初始化
            .tmux_enabled = config.enable_tmux,
        };
        
        // 初始化tmux集成
        if (config.enable_tmux) {
            try self.initTmux(allocator);
        }
        
        return self;
    }
    
    fn initTmux(self: *Self, allocator: std.mem.Allocator) !void {
        // 初始化tmux核心
        self.tmux_core = try tmux.TmuxCore.init(allocator);
        errdefer if (self.tmux_core) |*core| core.deinit();
        
        // 注册回调
        const callbacks = tmux.TmuxCallbacks{
            .context = self,
            .on_write_cell = tmuxWriteCell,
            .on_move_cursor = tmuxMoveCursor,
            .on_clear_screen = tmuxClearScreen,
            .on_scroll = tmuxScroll,
        };
        
        try self.tmux_core.?.registerBackend(callbacks);
        
        // 创建默认会话
        self.current_session = try self.tmux_core.?.newSession("main");
        try self.current_session.?.attach();
        
        // 创建默认窗口
        self.current_window = try self.tmux_core.?.newWindow(self.current_session.?, "shell");
    }
    
    pub fn deinit(self: *Self) void {
        if (self.tmux_core) |*core| {
            core.deinit();
        }
        self.grid.deinit();
    }
    
    // tmux命令接口
    pub fn tmuxCommand(self: *Self, command: []const u8) !void {
        if (!self.tmux_enabled or self.tmux_core == null) {
            return error.TmuxNotEnabled;
        }
        
        // 解析命令
        if (std.mem.eql(u8, command, "split-window -h")) {
            // 水平分割
            const new_pane = try self.tmux_core.?.splitPane(self.current_window.?, false, 50);
            self.current_pane = new_pane;
        } else if (std.mem.eql(u8, command, "split-window -v")) {
            // 垂直分割
            const new_pane = try self.tmux_core.?.splitPane(self.current_window.?, true, 50);
            self.current_pane = new_pane;
        } else if (std.mem.startsWith(u8, command, "new-window")) {
            // 新建窗口
            const window = try self.tmux_core.?.newWindow(self.current_session.?, "new");
            self.current_window = window;
        }
        // ... 其他命令
    }
    
    // tmux回调实现
    fn tmuxWriteCell(ctx: *anyopaque, x: u32, y: u32, cell: tmux.Cell) void {
        const self = @ptrCast(*Terminal, @alignCast(@alignOf(Terminal), ctx));
        self.grid.setCell(x, y, Cell{
            .char = cell.char,
            .fg = cell.fg,
            .bg = cell.bg,
            .attrs = cell.attrs,
        });
    }
    
    fn tmuxMoveCursor(ctx: *anyopaque, x: u32, y: u32) void {
        const self = @ptrCast(*Terminal, @alignCast(@alignOf(Terminal), ctx));
        self.cursor.x = x;
        self.cursor.y = y;
    }
    
    fn tmuxClearScreen(ctx: *anyopaque) void {
        const self = @ptrCast(*Terminal, @alignCast(@alignOf(Terminal), ctx));
        self.grid.clear();
    }
    
    fn tmuxScroll(ctx: *anyopaque, lines: i32) void {
        const self = @ptrCast(*Terminal, @alignCast(@alignOf(Terminal), ctx));
        self.grid.scroll(lines);
    }
};
```

### Task 3.4: 添加命令解析和键绑定 (1.5小时)

#### 创建文件
`/Users/jqwang/98-ghosttyAI/ghostty/src/tmux/command_parser.zig`

```zig
const std = @import("std");

pub const TmuxCommand = union(enum) {
    split_horizontal: u32,  // 百分比
    split_vertical: u32,    // 百分比
    new_window: []const u8, // 窗口名
    new_session: []const u8, // 会话名
    select_pane: enum { up, down, left, right },
    resize_pane: struct { direction: enum { up, down, left, right }, amount: u32 },
    kill_pane,
    kill_window,
    detach,
    list_sessions,
    list_windows,
    list_panes,
};

pub fn parseCommand(input: []const u8) !TmuxCommand {
    var iter = std.mem.tokenize(u8, input, " \t");
    const cmd = iter.next() orelse return error.EmptyCommand;
    
    if (std.mem.eql(u8, cmd, "split-window")) {
        const flag = iter.next() orelse "-h";
        const size_str = iter.next() orelse "50";
        const size = try std.fmt.parseInt(u32, size_str, 10);
        
        if (std.mem.eql(u8, flag, "-h")) {
            return TmuxCommand{ .split_horizontal = size };
        } else if (std.mem.eql(u8, flag, "-v")) {
            return TmuxCommand{ .split_vertical = size };
        }
    } else if (std.mem.eql(u8, cmd, "new-window")) {
        const name = iter.next() orelse "shell";
        return TmuxCommand{ .new_window = name };
    } else if (std.mem.eql(u8, cmd, "new-session")) {
        const name = iter.next() orelse "new";
        return TmuxCommand{ .new_session = name };
    } else if (std.mem.eql(u8, cmd, "select-pane")) {
        const dir = iter.next() orelse return error.MissingArgument;
        if (std.mem.eql(u8, dir, "-U")) {
            return TmuxCommand{ .select_pane = .up };
        } else if (std.mem.eql(u8, dir, "-D")) {
            return TmuxCommand{ .select_pane = .down };
        } else if (std.mem.eql(u8, dir, "-L")) {
            return TmuxCommand{ .select_pane = .left };
        } else if (std.mem.eql(u8, dir, "-R")) {
            return TmuxCommand{ .select_pane = .right };
        }
    }
    // ... 其他命令
    
    return error.UnknownCommand;
}

// 默认键绑定
pub const KeyBindings = struct {
    pub const prefix = .{ .ctrl = true, .key = 'b' };
    
    pub const bindings = .{
        .{ .key = '%', .command = "split-window -h" },
        .{ .key = '"', .command = "split-window -v" },
        .{ .key = 'c', .command = "new-window" },
        .{ .key = 'x', .command = "kill-pane" },
        .{ .key = '&', .command = "kill-window" },
        .{ .key = 'd', .command = "detach" },
        .{ .key = 'o', .command = "select-pane -t :.+" },
        .{ .key = 'n', .command = "next-window" },
        .{ .key = 'p', .command = "previous-window" },
        // ... 更多绑定
    };
};
```

### Task 3.5: 编译并集成测试 (1小时)

#### 创建构建脚本
`/Users/jqwang/98-ghosttyAI/scripts/build_integrated_ghostty.sh`

```bash
#!/bin/bash
set -e

PROJECT_ROOT="/Users/jqwang/98-ghosttyAI"

echo "=== Building libtmuxcore ==="
cd "$PROJECT_ROOT/tmux"
make -f Makefile.libtmuxcore clean
make -f Makefile.libtmuxcore

echo "=== Copying library to Ghostty ==="
cp libtmuxcore.dylib "$PROJECT_ROOT/ghostty/"
cp libtmuxcore_api.h "$PROJECT_ROOT/ghostty/src/tmux/"
cp ui_backend/ui_backend.h "$PROJECT_ROOT/ghostty/src/tmux/"

echo "=== Building Ghostty with tmux integration ==="
cd "$PROJECT_ROOT/ghostty"
zig build -Denable-tmux=true

echo "=== Installing to app bundle ==="
cp libtmuxcore.dylib "macos/build/Release/Ghostty.app/Contents/Frameworks/"
install_name_tool -add_rpath "@executable_path/../Frameworks" \
    "macos/build/Release/Ghostty.app/Contents/MacOS/ghostty"

echo "=== Verification ==="
otool -L "macos/build/Release/Ghostty.app/Contents/MacOS/ghostty" | grep tmux || true
nm "macos/build/Release/Ghostty.app/Contents/MacOS/ghostty" | grep tmc_ | head -5

echo "✅ Build complete!"
```

## 🔧 验证步骤

### 1. 功能测试
```bash
# 启动集成后的Ghostty
/Users/jqwang/98-ghosttyAI/ghostty/macos/build/Release/Ghostty.app/Contents/MacOS/ghostty

# 在Ghostty内测试tmux命令
# Ctrl-B % - 水平分割
# Ctrl-B " - 垂直分割
# Ctrl-B c - 新建窗口
# Ctrl-B n/p - 切换窗口
```

### 2. 集成验证脚本
```zig
// test_integration.zig
const std = @import("std");
const Terminal = @import("src/terminal/Terminal.zig").Terminal;

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    
    const config = .{
        .cols = 80,
        .rows = 24,
        .enable_tmux = true,
    };
    
    var terminal = try Terminal.init(gpa.allocator(), config);
    defer terminal.deinit();
    
    // 测试tmux命令
    try terminal.tmuxCommand("new-session main");
    try terminal.tmuxCommand("new-window test");
    try terminal.tmuxCommand("split-window -h");
    
    std.debug.print("Integration test passed!\n", .{});
}
```

## ⏰ 时间安排

| 时间段 | 任务 | 产出 |
|--------|------|------|
| 09:00-10:00 | Task 3.1 | Ghostty构建系统更新 |
| 10:00-12:30 | Task 3.2 | 完整FFI桥接实现 |
| 13:30-15:30 | Task 3.3 | Terminal模块集成 |
| 15:30-17:00 | Task 3.4 | 命令解析和键绑定 |
| 17:00-18:00 | Task 3.5 | 编译和集成测试 |

## ✅ Day 3 完成标准

- [ ] Ghostty成功链接libtmuxcore
- [ ] FFI桥接完全实现
- [ ] Terminal模块支持tmux功能
- [ ] 基本tmux命令可用
- [ ] 键绑定正常工作
- [ ] 集成测试通过
- [ ] Git提交："[WEEK6-D3] Complete Ghostty tmux integration"

---
*Day 3: Bringing tmux to life inside Ghostty!*