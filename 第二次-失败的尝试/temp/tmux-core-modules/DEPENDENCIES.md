# tmux核心模块依赖关系分析

## 已提取文件统计
- 总文件数: 47个
- 总大小: 1.1MB
- 原始tmux源代码大小: ~3MB (减少约63%)

## 模块分类与依赖关系

### 1. Core模块 (核心数据结构)
位置：`tmux-core-modules/core/`

#### 网格系统 (Grid System)
- **grid.c** → 核心网格数据结构
  - 依赖: xmalloc.h, tmux.h
  - 功能: 存储终端字符内容的二维网格
- **grid-view.c** → 网格视图操作
  - 依赖: grid.c
  - 功能: 网格的视图层操作
- **grid-reader.c** → 网格读取器
  - 依赖: grid.c
  - 功能: 按行/列读取网格数据

#### 屏幕系统 (Screen System)
- **screen.c** → 屏幕缓冲区
  - 依赖: grid.c
  - 功能: 管理当前屏幕状态
- **screen-write.c** → 屏幕写入操作
  - 依赖: screen.c, grid.c
  - 功能: 写入字符到屏幕（需要修改为回调）
- **screen-redraw.c** → 屏幕重绘
  - 依赖: screen.c
  - 功能: 管理屏幕重绘逻辑（需要修改）

#### 会话/窗口/面板系统
- **session.c** → 会话管理
  - 依赖: window.c, options.c
  - 功能: 管理tmux会话
- **window.c** → 窗口管理
  - 依赖: screen.c, grid.c
  - 功能: 管理窗口和面板
- **window-buffer.c** → 窗口缓冲区
  - 依赖: window.c
  - 功能: 管理窗口的历史缓冲

#### 布局系统
- **layout.c** → 布局核心
  - 依赖: window.c
  - 功能: 面板布局算法
- **layout-set.c** → 预设布局
  - 依赖: layout.c
  - 功能: 内置布局模式
- **layout-custom.c** → 自定义布局
  - 依赖: layout.c
  - 功能: 用户自定义布局

#### 其他核心功能
- **paste.c** → 剪贴板管理
- **colour.c** → 颜色处理
- **attributes.c** → 文本属性
- **utf8.c, utf8-combined.c** → UTF-8支持
- **hyperlinks.c** → 超链接支持
- **resize.c** → 调整大小逻辑
- **alerts.c** → 警告/通知
- **names.c** → 名称生成
- **notify.c** → 事件通知

### 2. Input模块 (输入处理)
位置：`tmux-core-modules/input/`

- **input.c** → 输入序列处理器（需要大量修改）
- **input-keys.c** → 键盘映射
- **key-string.c** → 键名转换
- **key-bindings.c** → 按键绑定

### 3. Commands模块 (命令处理)
位置：`tmux-core-modules/commands/`

- **cmd.c** → 命令框架
- **cmd-split-window.c** → 分割窗口命令
- **cmd-new-window.c** → 新建窗口命令
- **cmd-new-session.c** → 新建会话命令
- **cmd-kill-pane.c** → 关闭面板命令
- **cmd-kill-window.c** → 关闭窗口命令
- **cmd-resize-pane.c** → 调整面板大小命令
- **cmd-select-pane.c** → 选择面板命令
- **cmd-select-window.c** → 选择窗口命令

### 4. Support模块 (支持功能)
位置：`tmux-core-modules/support/`

- **xmalloc.c** → 内存分配包装
- **log.c** → 日志系统（需要修改为回调）
- **environ.c** → 环境变量管理
- **format.c, format-draw.c** → 格式化字符串
- **options.c, options-table.c** → 配置选项系统
- **arguments.c** → 参数解析

## 需要解决的依赛问题

### 缺失的依赖
1. **tty.c** - 需要完全重写为回调系统
2. **client/server相关** - 需要创建stub或移除相关调用
3. **cmd-queue.c** - 命令队列系统，可能需要简化版本
4. **event loop** - 需要替换为Ghostty的事件循环

### 需要创建的新文件

#### 1. libtmuxcore.h - 公共API接口
```c
// 核心API声明
int tmc_init(void);
void tmc_cleanup(void);
struct tmc_session *tmc_session_new(const char *name);
struct tmc_window *tmc_window_new(struct tmc_session *s);
struct tmc_pane *tmc_pane_split(struct tmc_window *w, int horizontal);
```

#### 2. ui_backend.h - 回调接口
```c
struct ui_backend_vtable {
    void (*on_grid_update)(void *ctx, int x, int y, struct grid_cell *cell);
    void (*on_pane_resize)(void *ctx, int id, int w, int h);
    void (*on_layout_change)(void *ctx, struct layout_cell *lc);
};
```

#### 3. tty_stub.c - TTY功能桩
```c
// 提供tty相关函数的空实现，避免链接错误
void tty_write(void (*cmdfn)(struct tty *, const struct tty_ctx *),
    struct tty_ctx *ctx) {
    // 调用回调而非输出到终端
}
```

## 编译测试计划

### 第一步：创建简单的Makefile
```makefile
CC = clang
CFLAGS = -fPIC -DLIBTMUXCORE_BUILD -Iinclude
LDFLAGS = -shared -dynamiclib

SRCS = core/*.c support/*.c
OBJS = $(SRCS:.c=.o)

libtmuxcore.dylib: $(OBJS)
    $(CC) $(LDFLAGS) -o $@ $^
```

### 第二步：解决编译错误
1. 添加缺失的头文件
2. 创建必要的stub函数
3. 移除或替换client/server相关代码

### 第三步：创建测试程序
```c
// test_libtmuxcore.c
#include "libtmuxcore.h"

int main() {
    tmc_init();
    struct tmc_session *s = tmc_session_new("test");
    struct tmc_window *w = tmc_window_new(s);
    tmc_pane_split(w, 1); // horizontal split
    tmc_cleanup();
    return 0;
}
```

## 下一步行动

1. **创建stub文件** - 解决缺失的依赖
2. **编写Makefile** - 尝试编译
3. **修改screen-write.c** - 添加回调点
4. **创建API头文件** - 定义清晰的接口
5. **编写测试程序** - 验证基本功能