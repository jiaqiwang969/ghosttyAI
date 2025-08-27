# Day 2 - 核心库功能完善

## 🎯 Day 2 目标
导出完整的tmux核心功能API，实现会话/窗口/窗格管理接口，完成命令处理机制。

## 📋 详细任务清单

### Task 2.1: 导出会话管理API (2小时)

#### 创建文件
`/Users/jqwang/98-ghosttyAI/tmux/libtmuxcore_api.h`

```c
#ifndef LIBTMUXCORE_API_H
#define LIBTMUXCORE_API_H

#include <stdint.h>
#include <stdbool.h>

// 错误码定义
typedef enum {
    TMC_SUCCESS = 0,
    TMC_ERROR_INVALID_PARAM = -1,
    TMC_ERROR_NOT_FOUND = -2,
    TMC_ERROR_ALREADY_EXISTS = -3,
    TMC_ERROR_OUT_OF_MEMORY = -4,
    TMC_ERROR_NOT_INITIALIZED = -5
} tmc_error_t;

// 句柄类型
typedef void* tmc_session_t;
typedef void* tmc_window_t;
typedef void* tmc_pane_t;

// 初始化和清理
tmc_error_t tmc_init(void);
void tmc_cleanup(void);

// 会话管理
tmc_error_t tmc_session_new(const char *name, tmc_session_t *session);
tmc_error_t tmc_session_attach(tmc_session_t session);
tmc_error_t tmc_session_detach(tmc_session_t session);
tmc_error_t tmc_session_rename(tmc_session_t session, const char *new_name);
tmc_error_t tmc_session_destroy(tmc_session_t session);
tmc_error_t tmc_session_list(tmc_session_t **sessions, size_t *count);
tmc_session_t tmc_session_current(void);

// 窗口管理
tmc_error_t tmc_window_new(tmc_session_t session, const char *name, tmc_window_t *window);
tmc_error_t tmc_window_close(tmc_window_t window);
tmc_error_t tmc_window_rename(tmc_window_t window, const char *new_name);
tmc_error_t tmc_window_select(tmc_window_t window);
tmc_error_t tmc_window_next(void);
tmc_error_t tmc_window_previous(void);
tmc_window_t tmc_window_current(void);

// 窗格管理
tmc_error_t tmc_pane_split(tmc_window_t window, bool horizontal, 
                           uint32_t size_percent, tmc_pane_t *new_pane);
tmc_error_t tmc_pane_close(tmc_pane_t pane);
tmc_error_t tmc_pane_resize(tmc_pane_t pane, uint32_t width, uint32_t height);
tmc_error_t tmc_pane_select(tmc_pane_t pane);
tmc_error_t tmc_pane_zoom_toggle(tmc_pane_t pane);
tmc_pane_t tmc_pane_current(void);

// 命令执行
tmc_error_t tmc_command_execute(const char *command);
tmc_error_t tmc_command_send_keys(tmc_pane_t pane, const char *keys);

// 回调注册（与ui_backend协同）
typedef struct tmc_callbacks {
    void (*on_output)(tmc_pane_t pane, const char *data, size_t len);
    void (*on_bell)(tmc_pane_t pane);
    void (*on_title_change)(tmc_pane_t pane, const char *title);
    void (*on_activity)(tmc_window_t window);
} tmc_callbacks_t;

tmc_error_t tmc_callbacks_register(const tmc_callbacks_t *callbacks);

#endif /* LIBTMUXCORE_API_H */
```

### Task 2.2: 实现会话管理功能 (2.5小时)

#### 创建文件
`/Users/jqwang/98-ghosttyAI/tmux/libtmuxcore_session.c`

```c
#include "libtmuxcore_api.h"
#include "tmux.h"
#include "session.h"

// 全局初始化状态
static bool tmc_initialized = false;
static struct event_base *tmc_event_base = NULL;

tmc_error_t 
tmc_init(void)
{
    if (tmc_initialized)
        return TMC_SUCCESS;
    
    // 初始化tmux全局状态
    tzset();
    log_open("libtmuxcore");
    
    // 创建event base
    tmc_event_base = event_base_new();
    if (!tmc_event_base)
        return TMC_ERROR_OUT_OF_MEMORY;
    
    // 初始化会话RB tree
    RB_INIT(&sessions);
    RB_INIT(&windows);
    
    tmc_initialized = true;
    return TMC_SUCCESS;
}

void
tmc_cleanup(void)
{
    if (!tmc_initialized)
        return;
    
    // 清理所有会话
    struct session *s, *s_next;
    RB_FOREACH_SAFE(s, sessions, &sessions, s_next) {
        session_destroy(s, 0, __func__);
    }
    
    // 清理event base
    if (tmc_event_base) {
        event_base_free(tmc_event_base);
        tmc_event_base = NULL;
    }
    
    log_close();
    tmc_initialized = false;
}

tmc_error_t
tmc_session_new(const char *name, tmc_session_t *session_out)
{
    if (!tmc_initialized)
        return TMC_ERROR_NOT_INITIALIZED;
    
    if (!name || !session_out)
        return TMC_ERROR_INVALID_PARAM;
    
    // 检查重名
    if (session_find(name) != NULL)
        return TMC_ERROR_ALREADY_EXISTS;
    
    // 创建新会话
    struct session *s = session_create(name, NULL, NULL, 
                                       NULL, -1, NULL);
    if (!s)
        return TMC_ERROR_OUT_OF_MEMORY;
    
    *session_out = (tmc_session_t)s;
    return TMC_SUCCESS;
}

tmc_error_t
tmc_session_attach(tmc_session_t session)
{
    if (!tmc_initialized)
        return TMC_ERROR_NOT_INITIALIZED;
    
    struct session *s = (struct session *)session;
    if (!s)
        return TMC_ERROR_INVALID_PARAM;
    
    // 设置为当前会话
    server_client_set_session(NULL, s);
    
    return TMC_SUCCESS;
}

tmc_session_t
tmc_session_current(void)
{
    if (!tmc_initialized)
        return NULL;
    
    // 获取当前客户端的会话
    struct client *c = server_client_get_current();
    if (c && c->session)
        return (tmc_session_t)c->session;
    
    return NULL;
}
```

### Task 2.3: 实现窗口/窗格管理 (2小时)

#### 创建文件
`/Users/jqwang/98-ghosttyAI/tmux/libtmuxcore_window.c`

```c
#include "libtmuxcore_api.h"
#include "tmux.h"
#include "window.h"

tmc_error_t
tmc_window_new(tmc_session_t session, const char *name, tmc_window_t *window_out)
{
    struct session *s = (struct session *)session;
    if (!s || !window_out)
        return TMC_ERROR_INVALID_PARAM;
    
    // 创建新窗口
    struct winlink *wl = session_new(s, name, NULL, NULL, -1, NULL);
    if (!wl)
        return TMC_ERROR_OUT_OF_MEMORY;
    
    *window_out = (tmc_window_t)wl->window;
    return TMC_SUCCESS;
}

tmc_error_t
tmc_pane_split(tmc_window_t window, bool horizontal, 
               uint32_t size_percent, tmc_pane_t *new_pane)
{
    struct window *w = (struct window *)window;
    if (!w || !new_pane)
        return TMC_ERROR_INVALID_PARAM;
    
    // 获取当前活动窗格
    struct window_pane *wp = w->active;
    if (!wp)
        return TMC_ERROR_NOT_FOUND;
    
    // 计算分割类型和大小
    enum layout_type type = horizontal ? LAYOUT_TOPBOTTOM : LAYOUT_LEFTRIGHT;
    u_int size = (horizontal ? wp->sy : wp->sx) * size_percent / 100;
    
    // 执行分割
    struct window_pane *new_wp = window_split_pane(wp, type, size, 0);
    if (!new_wp)
        return TMC_ERROR_OUT_OF_MEMORY;
    
    // 启动shell
    if (window_pane_start_cmd(new_wp, NULL, 0, NULL) != 0) {
        window_remove_pane(w, new_wp);
        return TMC_ERROR_OUT_OF_MEMORY;
    }
    
    *new_pane = (tmc_pane_t)new_wp;
    return TMC_SUCCESS;
}

tmc_error_t
tmc_pane_resize(tmc_pane_t pane, uint32_t width, uint32_t height)
{
    struct window_pane *wp = (struct window_pane *)pane;
    if (!wp)
        return TMC_ERROR_INVALID_PARAM;
    
    // 调整窗格大小
    window_pane_resize(wp, width, height);
    
    // 重新布局
    layout_fix_panes(wp->window);
    
    return TMC_SUCCESS;
}

tmc_error_t
tmc_pane_select(tmc_pane_t pane)
{
    struct window_pane *wp = (struct window_pane *)pane;
    if (!wp)
        return TMC_ERROR_INVALID_PARAM;
    
    // 设置为活动窗格
    window_set_active_pane(wp->window, wp, 1);
    
    return TMC_SUCCESS;
}
```

### Task 2.4: 实现命令处理机制 (1.5小时)

#### 创建文件
`/Users/jqwang/98-ghosttyAI/tmux/libtmuxcore_command.c`

```c
#include "libtmuxcore_api.h"
#include "tmux.h"
#include "cmd.h"

tmc_error_t
tmc_command_execute(const char *command)
{
    if (!command)
        return TMC_ERROR_INVALID_PARAM;
    
    // 解析命令
    struct cmd_parse_result *pr = cmd_parse_from_string(command, NULL);
    if (pr->status != CMD_PARSE_SUCCESS) {
        cmd_parse_free(pr);
        return TMC_ERROR_INVALID_PARAM;
    }
    
    // 创建命令队列项
    struct cmdq_item *item = cmdq_get_command(pr->cmdlist, NULL);
    if (!item) {
        cmd_parse_free(pr);
        return TMC_ERROR_OUT_OF_MEMORY;
    }
    
    // 执行命令
    cmdq_append(NULL, item);
    cmdq_run(NULL);
    
    cmd_parse_free(pr);
    return TMC_SUCCESS;
}

tmc_error_t
tmc_command_send_keys(tmc_pane_t pane, const char *keys)
{
    struct window_pane *wp = (struct window_pane *)pane;
    if (!wp || !keys)
        return TMC_ERROR_INVALID_PARAM;
    
    // 解析并发送按键
    size_t len = strlen(keys);
    for (size_t i = 0; i < len; i++) {
        // 发送字符到窗格的PTY
        if (wp->fd != -1) {
            if (write(wp->fd, &keys[i], 1) == -1)
                return TMC_ERROR_INVALID_PARAM;
        }
    }
    
    return TMC_SUCCESS;
}
```

### Task 2.5: 更新Makefile并编译 (1小时)

#### 修改文件
`/Users/jqwang/98-ghosttyAI/tmux/Makefile.libtmuxcore`

```makefile
# 添加新的源文件
API_SRCS = \
    libtmuxcore_session.c \
    libtmuxcore_window.c \
    libtmuxcore_command.c

CORE_SRCS += $(API_SRCS)

# 添加导出符号列表
EXPORT_SYMBOLS = -exported_symbols_list libtmuxcore.symbols

# 创建符号文件
libtmuxcore.symbols:
	@echo "_tmc_*" > $@
	@echo "_ui_backend_*" >> $@

# 更新链接命令
libtmuxcore.dylib: $(OBJS) libtmuxcore.symbols
	$(CC) $(LDFLAGS) $(EXPORT_SYMBOLS) -o $@ $(OBJS)
	codesign -s - $@
```

## 🔧 验证步骤

### 1. API测试程序
```c
// test_api.c
#include "libtmuxcore_api.h"
#include <stdio.h>
#include <assert.h>

int main() {
    // 初始化
    assert(tmc_init() == TMC_SUCCESS);
    
    // 创建会话
    tmc_session_t session;
    assert(tmc_session_new("test-session", &session) == TMC_SUCCESS);
    
    // 创建窗口
    tmc_window_t window;
    assert(tmc_window_new(session, "test-window", &window) == TMC_SUCCESS);
    
    // 分割窗格
    tmc_pane_t new_pane;
    assert(tmc_pane_split(window, true, 50, &new_pane) == TMC_SUCCESS);
    
    // 执行命令
    assert(tmc_command_execute("list-sessions") == TMC_SUCCESS);
    
    printf("All API tests passed!\n");
    
    // 清理
    tmc_cleanup();
    return 0;
}
```

### 2. 编译和运行测试
```bash
cd /Users/jqwang/98-ghosttyAI/tmux/
make -f Makefile.libtmuxcore clean
make -f Makefile.libtmuxcore

# 编译测试程序
clang -o test_api test_api.c -L. -ltmuxcore
./test_api
```

## ⏰ 时间安排

| 时间段 | 任务 | 产出 |
|--------|------|------|
| 09:00-11:00 | Task 2.1 + 2.2 | API定义，会话管理实现 |
| 11:00-13:00 | Task 2.3 | 窗口/窗格管理完成 |
| 14:00-15:30 | Task 2.4 | 命令处理机制完成 |
| 15:30-16:30 | Task 2.5 | 编译和集成测试 |
| 16:30-17:30 | 验证和调试 | 所有API可用 |

## ✅ Day 2 完成标准

- [ ] libtmuxcore_api.h定义完整
- [ ] 会话管理API实现并测试通过
- [ ] 窗口/窗格管理功能正常
- [ ] 命令执行机制工作
- [ ] libtmuxcore.dylib包含所有API
- [ ] 测试程序验证通过
- [ ] Git提交："[WEEK6-D2] Complete tmux core API implementation"

---
*Day 2: Exposing the full power of tmux as a library!*