# Ghostty × tmux 集合分析与分离策略

## 1. 交集部分 (Intersection) - 两者都有，功能重叠

### 网格/屏幕管理
| 功能 | Ghostty (Zig) | tmux (C) | 决策 |
|------|---------------|----------|------|
| 网格数据结构 | `page.zig`, `PageList.zig` | `grid.c`, `grid-view.c` | **使用Ghostty**，已有完整实现 |
| 屏幕缓冲 | `Screen.zig` | `screen.c`, `screen-write.c` | **使用Ghostty**，更现代的设计 |
| 光标管理 | `Screen.zig::Cursor` | `screen.c::screen_cursor` | **使用Ghostty** |
| 文本属性 | `style.zig`, `sgr.zig` | `attributes.c` | **使用Ghostty** |

### 输入/输出处理
| 功能 | Ghostty (Zig) | tmux (C) | 决策 |
|------|---------------|----------|------|
| 终端解析 | `Parser.zig` | `input.c` | **使用Ghostty** |
| UTF-8处理 | `UTF8Decoder.zig` | `utf8.c` | **使用Ghostty** |
| 颜色管理 | `color.zig` | `colour.c` | **使用Ghostty** |
| 键盘输入 | `../input/` | `input-keys.c` | **使用Ghostty** |

### 基础功能
| 功能 | Ghostty (Zig) | tmux (C) | 决策 |
|------|---------------|----------|------|
| 内存管理 | Zig内置 | `xmalloc.c` | **使用Zig** |
| 日志系统 | `std.log` | `log.c` | **使用Zig** |
| 字符串处理 | `StringMap.zig` | 多个C文件 | **使用Zig** |

## 2. tmux独有 (tmux-only) - 只有tmux有，需要保留

这些是tmux的核心价值，Ghostty完全没有的功能：

```
tmux-only/
├── session/              # 会话管理 - tmux核心
│   ├── session.c         # 会话逻辑
│   ├── cmd-new-session.c # 创建会话
│   └── cmd-*-session.c   # 会话命令
│
├── window/               # 窗口管理 - tmux核心
│   ├── window.c          # 窗口逻辑
│   ├── window-buffer.c   # 窗口缓冲
│   ├── cmd-*-window.c    # 窗口命令
│   └── names.c           # 窗口命名
│
├── pane/                 # 面板管理 - tmux核心
│   ├── cmd-split-window.c    # 分割面板
│   ├── cmd-select-pane.c     # 选择面板
│   ├── cmd-resize-pane.c     # 调整大小
│   └── cmd-kill-pane.c       # 关闭面板
│
├── layout/               # 布局算法 - tmux独有
│   ├── layout.c          # 核心布局逻辑
│   ├── layout-set.c      # 预设布局
│   └── layout-custom.c   # 自定义布局
│
├── commands/             # 命令系统 - tmux独有
│   ├── cmd.c             # 命令框架
│   └── arguments.c       # 参数解析
│
└── support/              # tmux特有支持
    ├── options.c         # tmux配置系统
    ├── options-table.c   # 配置表
    ├── paste.c          # tmux剪贴板
    ├── format.c         # tmux格式化
    └── notify.c         # 事件通知
```

## 3. Ghostty独有 (Ghostty-only) - 只有Ghostty有

这些是Ghostty的独特功能，tmux完全不需要：

```
ghostty-only/
├── rendering/            # GPU渲染 - Ghostty独有
│   ├── ../renderer/      # 渲染器(Metal/OpenGL/WebGL)
│   ├── ../font/          # 字体渲染
│   └── kitty/graphics/   # 图像协议
│
├── platform/             # 平台集成 - Ghostty独有
│   ├── ../apprt/         # 应用运行时
│   ├── ../macos/         # macOS特定
│   └── ../os/            # 操作系统抽象
│
├── config/               # 配置系统 - Ghostty独有
│   ├── ../config/        # 配置解析
│   └── ../cli/           # 命令行界面
│
├── terminal-features/    # 现代终端特性
│   ├── kitty/            # Kitty协议
│   ├── hyperlink.zig    # 超链接
│   ├── mouse_shape.zig  # 鼠标形状
│   └── Selection.zig    # 选择功能
│
└── infrastructure/       # 基础设施
    ├── ../build/         # 构建系统
    ├── ../datastruct/    # 数据结构
    └── ../terminfo/      # Terminfo
```

## 4. 集成策略

### 第一阶段：分离提取
1. **创建tmux-only目录** - 提取tmux独有功能
2. **识别接口点** - 确定tmux需要调用Ghostty的地方
3. **创建适配层** - 让tmux的C代码能调用Ghostty的Zig实现

### 第二阶段：接口设计
```c
// tmux_ghostty_bridge.h - tmux调用Ghostty功能的接口

// Grid操作 - tmux调用Ghostty的网格
struct ghostty_grid;
struct ghostty_grid* ghostty_grid_create(int width, int height);
void ghostty_grid_write_cell(struct ghostty_grid*, int x, int y, const char* utf8, uint32_t attr);
void ghostty_grid_clear(struct ghostty_grid*);

// Screen操作 - tmux调用Ghostty的屏幕
struct ghostty_screen;
struct ghostty_screen* ghostty_screen_create(struct ghostty_grid*);
void ghostty_screen_write(struct ghostty_screen*, const char* data, size_t len);

// 回调注册 - Ghostty通知tmux
void ghostty_register_resize_callback(void (*cb)(int w, int h));
void ghostty_register_input_callback(void (*cb)(const char* keys));
```

### 第三阶段：实现适配
```zig
// ghostty/src/tmux_adapter.zig - Ghostty端的适配层

const c = @cImport({
    @cInclude("tmux_ghostty_bridge.h");
});

export fn ghostty_grid_create(width: c_int, height: c_int) ?*anyopaque {
    const grid = PageList.init(...);
    return @ptrCast(grid);
}

export fn ghostty_grid_write_cell(grid_ptr: ?*anyopaque, x: c_int, y: c_int, utf8: [*c]const u8, attr: u32) void {
    const grid = @ptrCast(*PageList, @alignCast(grid_ptr));
    // 调用Ghostty的网格写入
}
```

## 5. 最终架构

```
┌─────────────────────────────────────────────────────┐
│                  Ghostty Application                  │
│                                                       │
│  ┌─────────────────────────────────────────────┐    │
│  │           Ghostty Terminal Core (Zig)        │    │
│  │  • Screen, Page, Parser                      │    │
│  │  • UTF-8, Color, Style                       │    │
│  │  • Rendering, Font, Platform                 │    │
│  └─────────────────┬───────────────────────────┘    │
│                    │                                  │
│         tmux_ghostty_bridge (C/Zig FFI)             │
│                    │                                  │
│  ┌─────────────────┴───────────────────────────┐    │
│  │          libtmuxcore (C) - tmux-only         │    │
│  │  • Session, Window, Pane Management          │    │
│  │  • Layout Algorithms                         │    │
│  │  • Command System                            │    │
│  └─────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────┘
```

## 6. 实施步骤

### Step 1: 提取tmux独有功能
```bash
# 只复制tmux独有的文件
mkdir -p tmux-only/{session,window,pane,layout,commands}
cp tmux/{session.c,window*.c,layout*.c,cmd*.c} tmux-only/
```

### Step 2: 移除重复代码
```bash
# 从tmux-only中删除与Ghostty重叠的部分
# - 删除grid.c, screen.c (使用Ghostty的)
# - 删除input.c, utf8.c (使用Ghostty的)
# - 删除colour.c, attributes.c (使用Ghostty的)
```

### Step 3: 创建桥接层
```bash
# 创建C到Zig的桥接
touch ghostty/src/tmux_adapter.zig
touch tmux-only/ghostty_bridge.h
```

### Step 4: 修改tmux代码
```c
// 修改tmux-only中的代码，调用Ghostty功能
// 例如在screen-write.c中：
#ifdef USE_GHOSTTY_GRID
    ghostty_grid_write_cell(grid, x, y, utf8, attr);
#else
    grid_set_cell(grid, x, y, &gc);  // 原始tmux代码
#endif
```

## 7. 预期结果

- **代码量减少**: tmux代码从1.1MB减少到约400KB（只保留独有功能）
- **无重复**: 完全消除了功能重复
- **清晰架构**: tmux负责会话/窗口逻辑，Ghostty负责终端渲染
- **高性能**: 避免了数据在两个网格系统间复制