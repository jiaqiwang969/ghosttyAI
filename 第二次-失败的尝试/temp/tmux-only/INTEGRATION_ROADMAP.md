# tmux-only 核心功能集成路线图

## 提取结果
- **tmux-only文件数**: 35个
- **代码量**: ~400KB（从1.1MB减少）
- **核心价值**: 会话管理、窗口复用、面板布局、持久化

## tmux-only 目录结构

```
tmux-only/
├── session/ (7个文件) - 会话管理核心
│   ├── session.c              # 会话生命周期管理
│   ├── cmd-new-session.c      # 创建会话
│   ├── cmd-kill-session.c     # 销毁会话
│   ├── cmd-rename-session.c   # 重命名会话
│   ├── cmd-attach-session.c   # 附加到会话
│   └── cmd-detach-client.c    # 分离客户端
│
├── window/ (6个文件) - 窗口管理核心
│   ├── window.c               # 窗口逻辑
│   ├── window-buffer.c        # 窗口缓冲区
│   ├── cmd-new-window.c       # 新建窗口
│   ├── cmd-kill-window.c      # 关闭窗口
│   ├── cmd-select-window.c    # 选择窗口
│   └── names.c                # 窗口命名
│
├── pane/ (6个文件) - 面板管理核心
│   ├── cmd-split-window.c     # 分割面板 ⭐ 最重要
│   ├── cmd-select-pane.c      # 选择面板
│   ├── cmd-resize-pane.c      # 调整面板大小
│   ├── cmd-kill-pane.c        # 关闭面板
│   ├── cmd-break-pane.c       # 分离面板
│   └── cmd-join-pane.c        # 合并面板
│
├── layout/ (4个文件) - 布局算法核心 ⭐
│   ├── layout.c               # 核心布局逻辑
│   ├── layout-set.c           # 预设布局(tiled, even-split等)
│   ├── layout-custom.c        # 自定义布局
│   └── resize.c               # 调整大小逻辑
│
├── commands/ (3个文件) - 命令框架
│   ├── cmd.c                  # 命令系统框架
│   ├── arguments.c            # 参数解析
│   └── cmd-find.c             # 查找逻辑
│
└── support/ (9个文件) - 支持功能
    ├── options.c              # 配置管理
    ├── options-table.c        # 配置表
    ├── paste.c                # 剪贴板
    ├── format.c               # 格式化
    ├── format-draw.c          # 格式化绘制
    ├── notify.c               # 事件通知
    ├── environ.c              # 环境变量
    └── alerts.c               # 警告系统
```

## 集成架构设计

### 1. 创建C-to-Zig桥接层

```c
// tmux-only/ghostty_interface.h
// tmux调用Ghostty功能的接口

// 网格操作 - tmux写入到Ghostty的PageList
void ghostty_write_cell(int pane_id, int x, int y, uint32_t codepoint, uint32_t attr);
void ghostty_clear_line(int pane_id, int y);
void ghostty_scroll_region(int pane_id, int top, int bottom, int lines);

// 屏幕操作 - tmux更新Ghostty的Screen
void ghostty_cursor_move(int pane_id, int x, int y);
void ghostty_screen_resize(int pane_id, int width, int height);

// PTY管理 - 每个pane的独立PTY
int ghostty_pty_create(int pane_id, const char* shell);
void ghostty_pty_write(int pane_id, const char* data, size_t len);
int ghostty_pty_read(int pane_id, char* buffer, size_t size);

// 事件回调 - Ghostty通知tmux
void tmux_on_key_input(const char* keys);
void tmux_on_mouse_event(int x, int y, int button);
void tmux_on_resize(int width, int height);
```

### 2. Ghostty端适配器实现

```zig
// ghostty/src/tmux_core_adapter.zig

const std = @import("std");
const Terminal = @import("terminal/Terminal.zig");
const Screen = @import("terminal/Screen.zig");
const PageList = @import("terminal/PageList.zig");

pub const TmuxCoreAdapter = struct {
    // 管理多个pane，每个有自己的Terminal
    panes: std.ArrayList(PaneState),
    active_pane: usize,
    
    const PaneState = struct {
        terminal: *Terminal,
        pty: std.os.fd_t,
        x: u32,
        y: u32,
        width: u32,
        height: u32,
    };
    
    // 导出给C的函数
    export fn ghostty_write_cell(pane_id: c_int, x: c_int, y: c_int, 
                                 codepoint: u32, attr: u32) void {
        const pane = &self.panes.items[@intCast(usize, pane_id)];
        pane.terminal.screen.writeCell(x, y, .{
            .char = codepoint,
            .attr = attr,
        });
    }
    
    export fn ghostty_pty_create(pane_id: c_int, shell: [*c]const u8) c_int {
        // 创建新的PTY for this pane
        const pty = std.os.openpt(...);
        self.panes.items[@intCast(usize, pane_id)].pty = pty;
        return pty;
    }
};
```

### 3. 修改tmux-only中的关键调用点

```c
// tmux-only/layout/layout.c
// 修改布局计算后的渲染调用

void layout_resize_pane(struct window_pane *wp, int sx, int sy) {
    // 原始tmux代码
    wp->sx = sx;
    wp->sy = sy;
    
#ifdef GHOSTTY_INTEGRATION
    // 通知Ghostty调整对应pane的大小
    ghostty_screen_resize(wp->id, sx, sy);
#else
    // 原始的tty调整代码
    tty_resize(wp->tty);
#endif
}
```

```c
// tmux-only/window/window.c
// 修改窗口创建时的PTY处理

struct window_pane *window_pane_create(struct window *w) {
    struct window_pane *wp = xmalloc(sizeof *wp);
    
#ifdef GHOSTTY_INTEGRATION
    // 让Ghostty创建和管理PTY
    wp->fd = ghostty_pty_create(wp->id, shell);
#else
    // 原始的PTY创建代码
    wp->fd = openpty(...);
#endif
    
    return wp;
}
```

### 4. 编译为动态库

```makefile
# tmux-only/Makefile

CC = clang
CFLAGS = -fPIC -DGHOSTTY_INTEGRATION -I../ghostty/include
LDFLAGS = -shared -dynamiclib

SRCS = session/*.c window/*.c pane/*.c layout/*.c commands/*.c support/*.c
OBJS = $(SRCS:.c=.o)

libtmuxcore.dylib: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# 生成的库只包含tmux独有逻辑
# 所有网格/屏幕/输入处理都委托给Ghostty
```

### 5. Ghostty集成点

```zig
// ghostty/build.zig
// 添加tmuxcore支持

pub fn build(b: *std.Build) void {
    const exe = b.addExecutable(.{
        .name = "ghostty",
        // ...
    });
    
    // 链接tmux核心库
    if (b.option(bool, "tmux-core", "Enable tmux core integration") orelse false) {
        exe.addIncludePath("../tmux-only/include");
        exe.linkSystemLibrary("tmuxcore");
        exe.addCSourceFile("src/tmux_core_adapter.zig", &.{});
        exe.defineCMacro("ENABLE_TMUX_CORE", null);
    }
}
```

## 实施计划

### Phase 1: 基础集成 (3天)
1. 创建ghostty_interface.h
2. 实现基本的C-to-Zig桥接
3. 编译libtmuxcore.dylib
4. 验证基本链接

### Phase 2: 核心功能 (5天)
1. 实现分屏功能(split-window)
2. 实现窗口切换
3. 实现PTY管理
4. 测试基本操作

### Phase 3: 完善功能 (3天)
1. 实现布局算法
2. 实现会话持久化
3. 优化性能
4. 完整测试

## 预期成果

用户将获得：
- **分屏功能**: Cmd+D水平分割，Cmd+Shift+D垂直分割
- **窗口管理**: Cmd+T新窗口，Cmd+1/2/3切换窗口
- **会话持久**: 断开SSH后重连，工作状态还在
- **智能布局**: 自动调整面板大小，预设布局
- **完美融合**: 所有UI都是Ghostty渲染，所有逻辑都是tmux管理

这就是真正的嵌入式集成！