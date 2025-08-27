# 真正的 Ghostty × tmux 内嵌集成方案

## 问题根源
当前的实现根本性错误在于：
1. 创建了一个"假的"libtmuxcore，只是模拟了API
2. 最终还是通过运行tmux进程来实现功能
3. tmux进程接管了整个终端UI，而不是只提供逻辑

## 正确的实现路径

### Phase 1: 真正提取tmux核心 (1-2周)

#### 1.1 修改tmux源代码的tty输出层
```c
// tmux/tty.c - 需要修改的核心文件
// 当前: tty_write() 直接输出VT100序列到终端
// 目标: tty_write() 调用回调函数传递结构化数据

// 在 tty.c:1200 附近添加:
#ifdef LIBTMUXCORE_BUILD
    if (ui_backend_vtable && ui_backend_vtable->on_grid_update) {
        // 不输出到终端，而是传递grid数据
        struct grid_update update = {
            .x = s->cx,
            .y = s->cy,
            .cells = grid_view_get_cells(s->grid, s->cy),
            .count = grid_view_get_line(s->grid, s->cy)->cellused
        };
        ui_backend_vtable->on_grid_update(ui_backend_vtable->user_data, &update);
        return;
    }
#endif
    // 原始的tty输出代码...
```

#### 1.2 提取核心组件为库
需要包含的tmux源文件：
- `grid.c`, `grid-view.c` - 网格数据结构
- `screen.c`, `screen-write.c` - 屏幕操作（修改为回调）
- `window.c`, `window-copy.c` - 窗口管理
- `session.c` - 会话管理
- `cmd-*.c` - 命令处理（选择性包含）
- `input.c`, `input-keys.c` - 输入处理

排除的文件：
- `tty-*.c` - 终端输出（替换为回调）
- `client.c`, `server.c` - 客户端/服务器架构
- `job.c`, `proc.c` - 进程管理（Ghostty负责）

#### 1.3 构建真正的动态库
```makefile
# tmux/Makefile.libtmuxcore
LIBTMUXCORE_SOURCES = \
    grid.c grid-view.c grid-reader.c \
    screen.c screen-write.c screen-redraw.c \
    window.c window-buffer.c window-client.c \
    session.c layout.c layout-custom.c \
    input.c input-keys.c \
    utf8.c key-string.c \
    options.c options-table.c \
    paste.c colour.c attributes.c \
    # 注意：不包含 tty.c client.c server.c

CFLAGS += -DLIBTMUXCORE_BUILD -fPIC
LDFLAGS += -shared -dynamiclib

libtmuxcore.dylib: $(LIBTMUXCORE_SOURCES:.c=.o)
    $(CC) $(LDFLAGS) -o $@ $^
```

### Phase 2: 实现UI后端回调系统 (1周)

#### 2.1 定义回调接口
```c
// tmux/ui_backend.h
struct ui_backend_vtable {
    void *user_data;
    
    // 网格更新回调
    void (*on_grid_update)(void *user_data, int pane_id, 
                          int x, int y, const struct grid_cell *cells, int count);
    
    // 光标位置回调
    void (*on_cursor_move)(void *user_data, int pane_id, int x, int y);
    
    // 面板布局变化回调
    void (*on_layout_change)(void *user_data, const struct layout_cell *root);
    
    // 面板创建/销毁回调
    void (*on_pane_create)(void *user_data, int pane_id, int x, int y, int w, int h);
    void (*on_pane_destroy)(void *user_data, int pane_id);
    
    // 状态变化回调
    void (*on_status_update)(void *user_data, const char *left, const char *right);
};
```

#### 2.2 修改screen-write.c
```c
// tmux/screen-write.c
void screen_write_cell(struct screen_write_ctx *ctx, const struct grid_cell *gc)
{
    struct screen *s = ctx->s;
    
#ifdef LIBTMUXCORE_BUILD
    // 通过回调传递，而不是写入grid
    if (ui_backend_vtable && ui_backend_vtable->on_grid_update) {
        ui_backend_vtable->on_grid_update(
            ui_backend_vtable->user_data,
            ctx->pane_id,  // 需要添加pane_id到context
            s->cx, s->cy,
            gc, 1
        );
        // 更新光标位置
        s->cx++;
        if (s->cx >= s->grid->sx) {
            s->cx = 0;
            s->cy++;
        }
        return;
    }
#endif
    // 原始的grid写入代码...
}
```

### Phase 3: Ghostty端集成 (1周)

#### 3.1 实现FFI桥接
```zig
// ghostty/src/tmux/core_integration.zig
const c = @cImport({
    @cInclude("libtmuxcore.h");
});

pub const TmuxCore = struct {
    session: ?*c.tmc_session_t,
    
    // 回调实现
    vtable: c.ui_backend_vtable = .{
        .user_data = null,
        .on_grid_update = onGridUpdate,
        .on_cursor_move = onCursorMove,
        .on_layout_change = onLayoutChange,
        .on_pane_create = onPaneCreate,
        .on_pane_destroy = onPaneDestroy,
    },
    
    pub fn init(self: *TmuxCore) !void {
        if (c.tmc_init() != c.TMC_SUCCESS) {
            return error.TmuxInitFailed;
        }
        
        // 注册我们的回调
        c.ui_backend_register(&self.vtable);
    }
    
    // 网格更新回调 - 这是关键！
    fn onGridUpdate(user_data: ?*anyopaque, pane_id: c_int, 
                   x: c_int, y: c_int, 
                   cells: [*c]const c.grid_cell, count: c_int) callconv(.C) void {
        // 将tmux的grid数据转换为Ghostty的Terminal格式
        const terminal = @ptrCast(*Terminal, @alignCast(@alignOf(Terminal), user_data));
        
        // 更新Ghostty的screen buffer
        var i: usize = 0;
        while (i < count) : (i += 1) {
            const cell = cells[i];
            terminal.screen.writeCell(@intCast(usize, x + i), @intCast(usize, y), Cell{
                .char = cell.data,
                .attr = translateAttributes(cell.attr),
            });
        }
        
        // 触发Ghostty重绘
        terminal.dirty = true;
    }
};
```

#### 3.2 修改Terminal.zig集成
```zig
// ghostty/src/terminal/Terminal.zig
pub const Terminal = struct {
    // 新增tmux模式
    tmux_core: ?TmuxCore = null,
    tmux_enabled: bool = false,
    
    pub fn init(self: *Terminal) !void {
        // 原有初始化代码...
        
        // 如果启用tmux模式
        if (std.os.getenv("GHOSTTY_TMUX_ENABLED")) |_| {
            self.tmux_core = TmuxCore{};
            try self.tmux_core.?.init();
            self.tmux_enabled = true;
            
            // 创建默认会话
            var session: c.tmc_session_t = undefined;
            _ = c.tmc_session_new("ghostty", &session);
            _ = c.tmc_session_attach(session);
            self.tmux_core.?.session = session;
        }
    }
    
    pub fn processInput(self: *Terminal, key: Key) !void {
        if (self.tmux_enabled) {
            // 所有输入发送给tmux处理
            _ = c.tmc_input_process_key(translateKey(key));
            // tmux会通过回调更新我们的grid
            return;
        }
        
        // 原有的输入处理...
    }
};
```

### Phase 4: 测试与验证 (1周)

#### 4.1 基础功能测试
```bash
# 编译真正的libtmuxcore
cd /Users/jqwang/98-ghosttyAI/tmux
make -f Makefile.libtmuxcore

# 验证符号
nm -D libtmuxcore.dylib | grep -E "grid_|screen_|window_|session_"

# 编译Ghostty with tmux
cd /Users/jqwang/98-ghosttyAI/ghostty
zig build -Dtmuxcore=true

# 运行测试
GHOSTTY_TMUX_ENABLED=1 ./zig-out/bin/ghostty
```

#### 4.2 验证要点
- [ ] Ghostty的UI完全保留（边框、状态栏都是GPU渲染）
- [ ] tmux的分屏功能正常（但分割线是Ghostty绘制的）
- [ ] 输入正确路由到对应的pane
- [ ] 没有任何VT100序列输出到终端
- [ ] 性能达标（60-120Hz刷新率）

## 时间估算
- Phase 1: 5-10天（最复杂，需要深入理解tmux代码）
- Phase 2: 3-5天（定义清晰的接口）
- Phase 3: 3-5天（Zig FFI相对简单）
- Phase 4: 2-3天（测试和调试）

总计：2-3周完成真正的集成

## 关键成功要素
1. **必须修改真正的tmux源代码** - 不能用stub
2. **完全去除tty输出** - 全部改为回调
3. **保持tmux逻辑完整** - 只改输出层，不改核心逻辑
4. **Ghostty完全控制UI** - tmux只提供数据