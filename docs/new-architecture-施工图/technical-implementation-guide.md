# 技术实施指南 Technical Implementation Guide

## 🎯 核心修改点 Core Modification Points

### 1. tty_write 拦截实现 tty_write Interception

#### 位置 Location
- **文件**: `tmux/tty.c`
- **函数**: `tty_write()` (约第 1234 行)
- **影响范围**: 22 个 `tty_cmd_*` 函数

#### 实现代码 Implementation Code

```c
// === 原始代码 Original Code ===
void tty_write(void (*cmdfn)(struct tty *, const struct tty_ctx *),
              struct tty_ctx *ctx) {
    struct window_pane *wp = ctx->wp;
    struct client *c;
    
    if (wp == NULL)
        return;
    
    TAILQ_FOREACH(c, &clients, entry) {
        if (!tty_client_ready(c, wp))
            continue;
        
        ctx->tty = &c->tty;
        cmdfn(&c->tty, ctx);
    }
}

// === 修改后代码 Modified Code ===
void tty_write(void (*cmdfn)(struct tty *, const struct tty_ctx *),
              struct tty_ctx *ctx) {
    struct window_pane *wp = ctx->wp;
    struct client *c;
    
    if (wp == NULL)
        return;
    
    TAILQ_FOREACH(c, &clients, entry) {
        if (!tty_client_ready(c, wp))
            continue;
        
        ctx->tty = &c->tty;
        
        // NEW: Route to backend
        if (c->tty.backend_mode == TTY_BACKEND_UI) {
            ui_backend_write(c->tty.ui_backend, cmdfn, ctx);
        } else {
            // Original TTY path
            cmdfn(&c->tty, ctx);
        }
    }
}
```

#### 需要拦截的命令 Commands to Intercept

```c
// ui_backend.h - 所有需要实现的命令
typedef struct ui_backend_ops {
    void (*cmd_alignmenttest)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_cell)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_cells)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_clearendofline)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_clearendofscreen)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_clearline)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_clearscreen)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_clearstartofline)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_clearstartofscreen)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_deletecharacter)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_deleteline)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_insertcharacter)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_insertline)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_linefeed)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_reverseindex)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_scrolldown)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_scrollup)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_setselection)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_rawstring)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_syncstart)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_syncend)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_syncupdate)(struct ui_backend*, const struct tty_ctx*);
} ui_backend_ops_t;
```

### 2. Backend Router 实现 Backend Router Implementation

#### 文件结构 File Structure
```
tmux/
├── backend_router.c     [NEW - CORE-002]
├── backend_router.h     [NEW - CORE-002]
├── backend_ghostty.c    [NEW - INTG-001]
├── backend_tty.c        [NEW - CORE-002]
└── ui_backend.h         [NEW - ARCH-001]
```

#### Router 核心逻辑 Router Core Logic

```c
// backend_router.c
#include "backend_router.h"
#include "ui_backend.h"

// 命令映射表 Command mapping table
static const struct {
    tty_cmd_fn tty_fn;
    ui_cmd_fn ui_fn;
} cmd_map[] = {
    { tty_cmd_cell, ui_cmd_cell },
    { tty_cmd_cells, ui_cmd_cells },
    { tty_cmd_clearline, ui_cmd_clearline },
    // ... 其他19个命令
};

void backend_route(struct tty* tty, tty_cmd_fn cmd, struct tty_ctx* ctx) {
    if (tty->backend_mode == TTY_BACKEND_UI) {
        // Find corresponding UI command
        for (size_t i = 0; i < nitems(cmd_map); i++) {
            if (cmd_map[i].tty_fn == cmd) {
                cmd_map[i].ui_fn(tty->ui_backend, ctx);
                return;
            }
        }
        // Fallback for unknown commands
        log_debug("Unknown command for UI backend: %p", cmd);
    } else {
        // Traditional TTY backend
        (*cmd)(tty, ctx);
    }
}
```

### 3. 帧聚合系统 Frame Aggregation System

#### 数据结构 Data Structures

```c
// frame_aggregator.h
typedef struct frame_aggregator {
    // 配置 Configuration
    uint64_t frame_interval_ns;    // 16666667 for 60 FPS
    uint64_t max_latency_ns;       // 8000000 (8ms)
    
    // 状态 State
    uint64_t last_frame_time_ns;
    uint64_t frame_seq;
    
    // 缓冲区 Buffers
    tmc_span_t* pending_spans;
    uint32_t pending_count;
    uint32_t pending_capacity;
    
    // 脏区跟踪 Dirty tracking
    uint32_t min_row, max_row;
    uint32_t min_col, max_col;
    bool full_refresh;
    
    // 统计 Statistics
    uint64_t frames_emitted;
    uint64_t spans_merged;
    uint64_t cells_updated;
} frame_aggregator_t;
```

#### 聚合算法 Aggregation Algorithm

```c
// frame_aggregator.c
void aggregate_grid_update(frame_aggregator_t* agg, 
                          const struct tty_ctx* ctx) {
    uint64_t now = get_monotonic_time_ns();
    
    // Convert tty_ctx to span
    tmc_span_t span = {
        .row = ctx->ocy,
        .col_start = ctx->ocx,
        .col_end = ctx->ocx + ctx->num,
        .cells = convert_cells(ctx)
    };
    
    // Add to pending
    add_span(agg, &span);
    
    // Update dirty region
    update_dirty_rect(agg, &span);
    
    // Check if should emit frame
    if (should_emit_frame(agg, now)) {
        emit_frame(agg);
    }
}

static bool should_emit_frame(frame_aggregator_t* agg, uint64_t now) {
    uint64_t elapsed = now - agg->last_frame_time_ns;
    
    // Emit if:
    // 1. Frame interval reached (vsync)
    if (elapsed >= agg->frame_interval_ns)
        return true;
    
    // 2. Maximum latency reached
    if (elapsed >= agg->max_latency_ns && agg->pending_count > 0)
        return true;
    
    // 3. Buffer full
    if (agg->pending_count >= agg->pending_capacity)
        return true;
    
    // 4. Urgent update (cursor, bell)
    if (has_urgent_update(agg))
        return true;
    
    return false;
}
```

### 4. FFI Bridge 实现 FFI Bridge Implementation

#### Zig 端接口 Zig Side Interface

```zig
// libtmuxcore.zig
const c = @cImport({
    @cInclude("libtmuxcore.h");
});

pub const TmuxCore = struct {
    server: *c.tmc_server_t,
    client: *c.tmc_client_t,
    
    const Callbacks = struct {
        fn onFrame(
            client: *c.tmc_client_t,
            frame: *const c.tmc_frame_t,
            user: ?*anyopaque
        ) callconv(.C) void {
            const self = @ptrCast(*TmuxCore, @alignCast(@alignOf(TmuxCore), user));
            self.handleFrame(frame);
        }
        
        fn onLayout(
            server: *c.tmc_server_t,
            layout: *const c.tmc_layout_update_t,
            user: ?*anyopaque
        ) callconv(.C) void {
            const self = @ptrCast(*TmuxCore, @alignCast(@alignOf(TmuxCore), user));
            self.handleLayout(layout);
        }
    };
    
    pub fn init(allocator: std.mem.Allocator) !TmuxCore {
        // Create config
        var config = std.mem.zeroes(c.tmc_server_config_t);
        config.size = @sizeOf(c.tmc_server_config_t);
        config.frame_buffer_size = 256;
        config.max_fps = 60;
        config.ui_draws_borders = true;
        
        // Create callbacks
        var callbacks = std.mem.zeroes(c.tmc_ui_vtable_t);
        callbacks.size = @sizeOf(c.tmc_ui_vtable_t);
        callbacks.on_frame = Callbacks.onFrame;
        callbacks.on_layout = Callbacks.onLayout;
        
        // Initialize server
        var server: *c.tmc_server_t = undefined;
        const err = c.tmc_server_new(
            &server,
            &config,
            &loop_vtable,
            null,
            &callbacks,
            @ptrCast(*anyopaque, self)
        );
        
        if (err != c.TMC_OK) {
            return error.ServerInitFailed;
        }
        
        return TmuxCore{
            .server = server,
            .client = null,
        };
    }
    
    fn handleFrame(self: *TmuxCore, frame: *const c.tmc_frame_t) void {
        // Process frame
        const spans = frame.spans[0..frame.span_count];
        
        for (spans) |span| {
            self.processSpan(&span);
        }
        
        // Trigger render
        self.scheduleRender();
    }
};
```

#### 类型转换 Type Conversion

```zig
// type_converter.zig
pub fn convertCell(c_cell: c.tmc_cell_t) Cell {
    return Cell{
        .codepoint = c_cell.codepoint,
        .fg = convertColor(c_cell.fg_rgb),
        .bg = convertColor(c_cell.bg_rgb),
        .attrs = @bitCast(Attributes, c_cell.attrs),
        .width = c_cell.width,
    };
}

fn convertColor(rgb: u32) Color {
    if (rgb == c.TMC_COLOR_DEFAULT) {
        return Color.default;
    }
    return Color{
        .r = @intCast(u8, (rgb >> 16) & 0xFF),
        .g = @intCast(u8, (rgb >> 8) & 0xFF),
        .b = @intCast(u8, rgb & 0xFF),
    };
}
```

### 5. 事件循环集成 Event Loop Integration

#### Loop vtable 实现 Loop vtable Implementation

```c
// loop_vtable.c
typedef struct ghostty_loop_impl {
    tmc_loop_vtable_t vtable;
    void* ghostty_loop;  // Ghostty's event loop
} ghostty_loop_impl_t;

static int ghostty_add_fd(tmc_io_t* io, int fd, int events, 
                         tmc_io_fd_cb_t cb, void* user) {
    ghostty_loop_impl_t* impl = (ghostty_loop_impl_t*)io;
    
    // Convert to Ghostty events
    uint32_t ghostty_events = 0;
    if (events & TMC_IO_READ) ghostty_events |= GHOSTTY_READ;
    if (events & TMC_IO_WRITE) ghostty_events |= GHOSTTY_WRITE;
    
    // Register with Ghostty loop
    return ghostty_register_fd(impl->ghostty_loop, fd, ghostty_events, cb, user);
}

static tmc_loop_vtable_t ghostty_loop_vtable = {
    .size = sizeof(tmc_loop_vtable_t),
    .version = 1,
    .add_fd = ghostty_add_fd,
    .mod_fd = ghostty_mod_fd,
    .del_fd = ghostty_del_fd,
    .add_timer = ghostty_add_timer,
    .del_timer = ghostty_del_timer,
    .post = ghostty_post,
    .wake = ghostty_wake,
};
```

## 🔧 构建系统集成 Build System Integration

### tmux 端修改 tmux Side Changes

```makefile
# Makefile.am 修改
if ENABLE_LIBTMUXCORE
lib_LTLIBRARIES = libtmuxcore.la
libtmuxcore_la_SOURCES = \
    backend_router.c \
    backend_ghostty.c \
    frame_aggregator.c \
    loop_vtable.c \
    $(original_sources)
    
libtmuxcore_la_CFLAGS = -DLIBTMUXCORE_BUILD
libtmuxcore_la_LDFLAGS = -version-info 1:0:0
endif
```

### Ghostty 端集成 Ghostty Side Integration

```zig
// build.zig 修改
const tmux_core = b.addStaticLibrary(.{
    .name = "tmuxcore",
    .target = target,
    .optimize = optimize,
});

tmux_core.addCSourceFiles(&.{
    "deps/tmux/backend_router.c",
    "deps/tmux/backend_ghostty.c",
    "deps/tmux/frame_aggregator.c",
    // ... other sources
}, &.{
    "-std=c11",
    "-DLIBTMUXCORE_BUILD",
});

tmux_core.linkLibC();
tmux_core.installHeader("deps/tmux/libtmuxcore.h", "libtmuxcore.h");

exe.linkLibrary(tmux_core);
```

## 🧪 测试验证点 Test Verification Points

### 单元测试 Unit Tests

```c
// test_backend_router.c
void test_router_ui_backend(void) {
    struct tty tty = {0};
    struct tty_ctx ctx = {0};
    
    tty.backend_mode = TTY_BACKEND_UI;
    tty.ui_backend = create_test_backend();
    
    // Test each command
    backend_route(&tty, tty_cmd_cell, &ctx);
    assert(test_backend_received_cell());
    
    backend_route(&tty, tty_cmd_clearline, &ctx);
    assert(test_backend_received_clearline());
}

void test_frame_aggregation(void) {
    frame_aggregator_t* agg = frame_aggregator_new(60); // 60 FPS
    
    // Rapid updates should batch
    for (int i = 0; i < 100; i++) {
        struct tty_ctx ctx = make_test_ctx(i);
        aggregate_grid_update(agg, &ctx);
    }
    
    // Should have emitted fewer frames than updates
    assert(agg->frames_emitted < 10);
    assert(agg->spans_merged > 50);
}
```

### 集成测试 Integration Tests

```zig
// test_integration.zig
test "tmux core initialization" {
    const core = try TmuxCore.init(testing.allocator);
    defer core.deinit();
    
    try testing.expect(core.server != null);
}

test "frame callback delivery" {
    var core = try TmuxCore.init(testing.allocator);
    defer core.deinit();
    
    var received_frame = false;
    core.onFrame = struct {
        fn callback(frame: *const c.tmc_frame_t) void {
            received_frame = true;
        }
    }.callback;
    
    // Create client and trigger update
    const client = try core.attachClient(80, 24);
    try core.sendText("Hello, tmux!");
    
    // Wait for frame
    try testing.expect(received_frame);
}
```

## 📋 验收清单 Acceptance Checklist

### Week 1 验收 Week 1 Acceptance
- [ ] `tty_write()` 成功拦截所有 22 个命令
- [ ] Backend router 正确路由到 UI/TTY
- [ ] 基础 span 转换工作
- [ ] 无内存泄漏（Valgrind 验证）
- [ ] 传统 tmux 功能未受影响

### Week 2 验收 Week 2 Acceptance
- [ ] 事件循环成功抽象
- [ ] 帧批处理达到 60 FPS
- [ ] 回调系统稳定工作
- [ ] 布局更新正确传递
- [ ] 复制模式基本功能

### Week 3 验收 Week 3 Acceptance
- [ ] FFI 绑定类型安全
- [ ] Ghostty 成功接收网格更新
- [ ] 输入事件正确路由
- [ ] 错误处理机制完善
- [ ] 内存边界检查通过

### Week 4 验收 Week 4 Acceptance
- [ ] vim/neovim 完美兼容
- [ ] htop 显示正确
- [ ] 1000 panes 压力测试通过
- [ ] 性能达标（<1ms 延迟）
- [ ] 文档完整

## 🚨 常见问题解决 Common Issues

### 问题 1: 回调死锁
**症状**: UI 冻结，CPU 100%
**原因**: 回调中持有锁
**解决**: 
```c
// 错误 Wrong
void on_grid_callback() {
    pthread_mutex_lock(&grid_lock);
    update_grid();  // May call back into tmux!
    pthread_mutex_unlock(&grid_lock);
}

// 正确 Correct
void on_grid_callback() {
    grid_update_t* update = copy_update();
    schedule_on_main_thread(update);
}
```

### 问题 2: 帧撕裂
**症状**: 部分更新可见
**原因**: 未等待完整帧
**解决**: 使用帧序号确保完整性

### 问题 3: 内存增长
**症状**: 长时间运行内存持续增长
**原因**: Span 缓冲区未释放
**解决**: 实现环形缓冲区，及时释放旧帧

---

本指南将随项目进展持续更新。如有问题，请联系相应组件负责人。