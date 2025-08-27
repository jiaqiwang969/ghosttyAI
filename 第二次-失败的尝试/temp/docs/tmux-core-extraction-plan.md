# tmux核心模块提取计划

## 分析目标
提取tmux中管理会话、窗口、面板和网格数据的核心逻辑，去除所有终端输出、客户端-服务器架构和进程管理相关代码。

## 核心模块分类

### 1. 数据结构核心 (必须提取)
这些文件包含了tmux的核心数据结构，必须完整保留：

- **grid.c, grid-view.c, grid-reader.c** - 网格数据结构（存储终端内容）
- **screen.c, screen-write.c, screen-redraw.c** - 屏幕缓冲区管理
- **window.c, window-buffer.c** - 窗口管理
- **session.c** - 会话管理
- **layout.c, layout-set.c, layout-custom.c** - 面板布局算法
- **options.c, options-table.c** - 配置选项管理
- **paste.c** - 剪贴板管理
- **colour.c** - 颜色处理
- **attributes.c** - 文本属性
- **utf8.c, utf8-combined.c** - UTF-8处理
- **hyperlinks.c** - 超链接支持

### 2. 输入处理 (需要修改)
这些文件处理键盘和鼠标输入，需要修改以适配Ghostty：

- **input.c** - 输入序列处理（需要大量修改）
- **input-keys.c** - 键盘映射
- **key-string.c** - 键名转换
- **key-bindings.c** - 按键绑定（可能不需要）

### 3. 输出层 (需要完全重写)
这些文件负责终端输出，需要完全替换为回调机制：

- **tty.c** - TTY输出（完全替换为回调）
- **tty-term.c** - 终端能力（可能不需要）
- **tty-keys.c** - TTY键盘输入（不需要）
- **tty-features.c** - 终端特性检测（不需要）
- **tty-acs.c** - 字符集（可能需要部分）

### 4. 命令系统 (选择性提取)
只提取核心命令，去除客户端相关命令：

需要的命令：
- **cmd-split-window.c** - 分割窗口
- **cmd-new-window.c** - 新建窗口
- **cmd-new-session.c** - 新建会话
- **cmd-kill-pane.c** - 关闭面板
- **cmd-kill-window.c** - 关闭窗口
- **cmd-resize-pane.c** - 调整面板大小
- **cmd-select-pane.c** - 选择面板
- **cmd-select-window.c** - 选择窗口

不需要的命令（客户端相关）：
- cmd-attach-session.c
- cmd-detach-client.c
- cmd-list-clients.c
- cmd-switch-client.c

### 5. 不需要的模块 (完全排除)
这些模块与客户端-服务器架构相关，完全不需要：

- **client.c** - 客户端逻辑
- **server.c, server-client.c, server-fn.c** - 服务器逻辑
- **proc.c** - 进程管理
- **job.c** - 作业管理
- **control.c, control-notify.c** - 控制模式
- **file.c** - 文件传输
- **tmux.c** - 主程序入口

### 6. 支持模块 (需要保留)
这些是基础支持模块：

- **xmalloc.c** - 内存分配
- **log.c** - 日志（修改为回调）
- **environ.c** - 环境变量
- **format.c, format-draw.c** - 格式化字符串

## 提取步骤

### 第一步：创建目录结构
```bash
mkdir -p /Users/jqwang/98-ghosttyAI/tmux-core-modules/{core,input,output,commands,support}
```

### 第二步：复制核心文件
```bash
# 数据结构核心
cp grid*.c screen*.c window*.c session.c layout*.c tmux-core-modules/core/

# 输入处理
cp input*.c key*.c tmux-core-modules/input/

# 支持模块
cp xmalloc.c log.c environ.c format*.c options*.c tmux-core-modules/support/

# 必要的头文件
cp tmux.h tmux-protocol.h compat.h xmalloc.h tmux-core-modules/
```

### 第三步：创建接口头文件
创建新的 `libtmuxcore.h`，定义清晰的API接口。

## 修改重点

### 1. tty.c 修改
将所有 `tty_write()` 和 `tty_cmd_*` 函数改为调用回调：

```c
// 原始代码
void tty_write(void (*cmdfn)(struct tty *, const struct tty_ctx *),
    struct tty_ctx *ctx)
{
    // 输出到终端
    tty_putcode(tty, TTYC_CUP, ctx->ocy, ctx->ocx);
    ...
}

// 修改后
void tty_write(void (*cmdfn)(struct tty *, const struct tty_ctx *),
    struct tty_ctx *ctx)
{
#ifdef LIBTMUXCORE_BUILD
    if (ui_backend_vtable && ui_backend_vtable->on_tty_write) {
        ui_backend_vtable->on_tty_write(ctx);
        return;
    }
#endif
    // 原始代码...
}
```

### 2. screen-write.c 修改
将网格写入改为回调通知：

```c
// 添加回调通知
void screen_write_cell(struct screen_write_ctx *ctx, 
                       const struct grid_cell *gc)
{
    // 原始网格操作
    grid_view_set_cell(s->grid, s->cx, s->cy, gc);
    
#ifdef LIBTMUXCORE_BUILD
    // 通知UI更新
    if (ui_backend_vtable && ui_backend_vtable->on_cell_update) {
        ui_backend_vtable->on_cell_update(s->cx, s->cy, gc);
    }
#endif
}
```

### 3. 创建回调接口
```c
// ui_backend.h
struct ui_backend_vtable {
    void (*on_grid_update)(int x, int y, const struct grid_cell *cell);
    void (*on_pane_resize)(int pane_id, int w, int h);
    void (*on_layout_change)(struct layout_cell *lc);
    void (*on_cursor_move)(int x, int y);
    void (*on_bell)(void);
    void (*on_title_change)(const char *title);
};
```

## 预期结果

提取后的 `tmux-core-modules` 目录将包含约30-40个核心源文件，总代码量约减少60%。这些文件将：

1. 保留所有会话/窗口/面板管理逻辑
2. 保留网格和屏幕缓冲区管理
3. 去除所有TTY输出代码
4. 去除客户端-服务器架构
5. 添加清晰的回调接口

## 下一步计划

1. 执行文件提取
2. 创建简化的Makefile
3. 尝试编译为静态库
4. 逐步添加回调接口
5. 测试基本功能