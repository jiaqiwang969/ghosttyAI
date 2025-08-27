# Day 1 - tmux源码深度修改指南

## 🎯 Day 1 目标
修改tmux源码，添加LIBTMUXCORE_BUILD条件编译，实现UI backend路由机制，替换所有tty输出为结构化回调。

## 📋 详细任务清单

### Task 1.1: 修改tty.c添加backend路由 (2小时)

#### 文件位置
`/Users/jqwang/98-ghosttyAI/tmux/tty.c`

#### 修改内容
```c
// 在文件开头添加（约第50行）
#ifdef LIBTMUXCORE_BUILD
#include "ui_backend/ui_backend.h"
static struct ui_backend_vtable *ui_backend = NULL;
#endif

// 修改tty_write函数（约1200行）
void
tty_write(void (*cmdfn)(struct tty *, const struct tty_ctx *),
    struct tty_ctx *ctx)
{
#ifdef LIBTMUXCORE_BUILD
    if (ui_backend && ui_backend->handle_output) {
        // 路由到结构化输出
        ui_backend->handle_output(ctx);
        return;
    }
#endif
    // 原始tty输出代码...
}

// 修改每个tty_cmd_*函数
void
tty_cmd_clearline(struct tty *tty, const struct tty_ctx *ctx)
{
#ifdef LIBTMUXCORE_BUILD
    if (ui_backend && ui_backend->clear_line) {
        ui_backend->clear_line(ctx->ocy);
        return;
    }
#endif
    // 原始实现...
}
```

### Task 1.2: 创建ui_backend接口 (1.5小时)

#### 创建文件
`/Users/jqwang/98-ghosttyAI/tmux/ui_backend/ui_backend.h`

```c
#ifndef UI_BACKEND_H
#define UI_BACKEND_H

#include "../tmux.h"

// UI后端回调函数表
struct ui_backend_vtable {
    // 基础输出
    void (*handle_output)(const struct tty_ctx *ctx);
    void (*write_cell)(u_int x, u_int y, const struct grid_cell *gc);
    
    // 光标控制
    void (*move_cursor)(u_int x, u_int y);
    void (*show_cursor)(int visible);
    
    // 屏幕操作
    void (*clear_screen)(void);
    void (*clear_line)(u_int y);
    void (*scroll_region)(u_int top, u_int bottom, int lines);
    
    // 窗格管理
    void (*split_pane)(int horizontal, u_int size);
    void (*resize_pane)(u_int id, u_int width, u_int height);
    void (*close_pane)(u_int id);
    
    // 会话管理
    void (*new_session)(const char *name);
    void (*attach_session)(u_int id);
    void (*detach_session)(void);
};

// 全局注册函数
void ui_backend_register(struct ui_backend_vtable *vtable);
void ui_backend_unregister(void);

// 辅助函数
int ui_backend_is_active(void);

#endif /* UI_BACKEND_H */
```

### Task 1.3: 修改screen-write.c (2小时)

#### 文件位置
`/Users/jqwang/98-ghosttyAI/tmux/screen-write.c`

#### 修改内容
```c
// 添加backend支持
#ifdef LIBTMUXCORE_BUILD
#include "ui_backend/ui_backend.h"
extern struct ui_backend_vtable *ui_backend;
#endif

// 修改screen_write_cell函数
void
screen_write_cell(struct screen_write_ctx *ctx, const struct grid_cell *gc)
{
#ifdef LIBTMUXCORE_BUILD
    if (ui_backend && ui_backend->write_cell) {
        ui_backend->write_cell(ctx->cx, ctx->cy, gc);
    }
#endif
    // 原始实现...
}

// 修改screen_write_cursormove
void
screen_write_cursormove(struct screen_write_ctx *ctx, int px, int py,
    int origin)
{
#ifdef LIBTMUXCORE_BUILD
    if (ui_backend && ui_backend->move_cursor) {
        ui_backend->move_cursor(px, py);
    }
#endif
    // 原始实现...
}
```

### Task 1.4: 创建Makefile.libtmuxcore (1小时)

#### 文件位置
`/Users/jqwang/98-ghosttyAI/tmux/Makefile.libtmuxcore`

```makefile
# libtmuxcore动态库构建

CC = clang
CFLAGS = -fPIC -DLIBTMUXCORE_BUILD -O2 -arch arm64
LDFLAGS = -dynamiclib -install_name @rpath/libtmuxcore.dylib

# 源文件（精选核心功能）
CORE_SRCS = \
    cmd.c \
    cmd-new-session.c \
    cmd-new-window.c \
    cmd-split-window.c \
    grid.c \
    grid-view.c \
    input.c \
    input-keys.c \
    screen.c \
    screen-write.c \
    session.c \
    tmux.c \
    tty.c \
    tty-keys.c \
    tty-term.c \
    utf8.c \
    window.c \
    ui_backend/ui_backend.c \
    ui_backend/ui_backend_router.c

OBJS = $(CORE_SRCS:.c=.o)

libtmuxcore.dylib: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	codesign -s - $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) libtmuxcore.dylib

install: libtmuxcore.dylib
	cp libtmuxcore.dylib ../ghostty/
	cp ui_backend/ui_backend.h ../ghostty/src/tmux/

.PHONY: clean install
```

### Task 1.5: 实现ui_backend路由器 (1.5小时)

#### 创建文件
`/Users/jqwang/98-ghosttyAI/tmux/ui_backend/ui_backend_router.c`

```c
#include "ui_backend.h"
#include <stdlib.h>

// 全局vtable指针
struct ui_backend_vtable *ui_backend = NULL;

void
ui_backend_register(struct ui_backend_vtable *vtable)
{
    if (vtable == NULL)
        return;
    
    ui_backend = vtable;
}

void
ui_backend_unregister(void)
{
    ui_backend = NULL;
}

int
ui_backend_is_active(void)
{
    return (ui_backend != NULL);
}

// 默认回调实现（用于测试）
static void
default_handle_output(const struct tty_ctx *ctx)
{
    // 记录输出用于调试
    if (ctx && ctx->cell)
        log_debug("UI Backend: output at (%u, %u)", ctx->ocx, ctx->ocy);
}

static void
default_write_cell(u_int x, u_int y, const struct grid_cell *gc)
{
    if (gc)
        log_debug("UI Backend: cell at (%u, %u) = '%c'", x, y, gc->data.data[0]);
}

// 创建默认vtable用于测试
struct ui_backend_vtable *
ui_backend_create_default(void)
{
    struct ui_backend_vtable *vtable = calloc(1, sizeof(*vtable));
    if (vtable) {
        vtable->handle_output = default_handle_output;
        vtable->write_cell = default_write_cell;
    }
    return vtable;
}
```

## 🔧 验证步骤

### 1. 编译测试
```bash
cd /Users/jqwang/98-ghosttyAI/tmux/
make -f Makefile.libtmuxcore clean
make -f Makefile.libtmuxcore
```

### 2. 符号验证
```bash
nm -g libtmuxcore.dylib | grep ui_backend
# 应该看到:
# _ui_backend_register
# _ui_backend_unregister
# _ui_backend_is_active
```

### 3. 基础测试
```c
// test_backend.c
#include "ui_backend/ui_backend.h"
#include <stdio.h>

int main() {
    struct ui_backend_vtable *vtable = ui_backend_create_default();
    ui_backend_register(vtable);
    
    printf("Backend active: %d\n", ui_backend_is_active());
    
    // 测试回调
    if (vtable->write_cell)
        vtable->write_cell(10, 20, NULL);
    
    ui_backend_unregister();
    free(vtable);
    
    return 0;
}
```

## ⏰ 时间安排

| 时间段 | 任务 | 产出 |
|--------|------|------|
| 09:00-11:00 | Task 1.1 + 1.2 | tty.c修改，ui_backend.h创建 |
| 11:00-13:00 | Task 1.3 | screen-write.c修改完成 |
| 14:00-15:00 | Task 1.4 | Makefile.libtmuxcore完成 |
| 15:00-16:30 | Task 1.5 | ui_backend_router实现 |
| 16:30-17:30 | 编译测试验证 | libtmuxcore.dylib可用 |

## ✅ Day 1 完成标准

- [ ] tty.c成功添加LIBTMUXCORE_BUILD路由
- [ ] ui_backend接口定义完整
- [ ] screen-write.c支持结构化输出
- [ ] libtmuxcore.dylib成功编译
- [ ] 基础测试通过
- [ ] Git提交："[WEEK6-D1] tmux source modified for libtmuxcore"

## 🚨 注意事项

1. **保留原始功能** - 所有修改都在`#ifdef LIBTMUXCORE_BUILD`内
2. **最小化改动** - 只修改必要的输出路径
3. **测试充分** - 每个修改都要验证不影响原tmux
4. **文档记录** - 记录所有修改位置便于回滚

## 📝 Day 1 交付物

1. 修改后的tmux源文件（tty.c, screen-write.c等）
2. 新增ui_backend目录和文件
3. Makefile.libtmuxcore构建脚本
4. 编译成功的libtmuxcore.dylib
5. 测试报告和验证结果

---
*Day 1: Building the foundation for embedded tmux!*