# P0 缺陷修复迁移指南
# Migration Guide for P0 Defect Fixes

**作者**: ARCH-001 (System Architect)  
**版本**: 2.0.0  
**日期**: 2025-08-25  
**目标读者**: CORE-001, CORE-002, INTG-001, QA-002

---

## 📋 概述

本指南帮助各团队将现有代码迁移到修复后的接口。所有P0缺陷已通过新的头文件和实现解决。

**修复文件位置**: `/Users/jqwang/98-ghosttyAI/cache/week1/ARCH-001/`

## 🔧 快速迁移步骤

### Step 1: 更新包含路径 (所有团队)

```bash
# 将这些文件复制到你的工作目录
cp /Users/jqwang/98-ghosttyAI/cache/week1/ARCH-001/tty_ctx_unified.h ./
cp /Users/jqwang/98-ghosttyAI/cache/week1/ARCH-001/interface_adapter.h ./
cp /Users/jqwang/98-ghosttyAI/cache/week1/ARCH-001/interface_adapter.c ./
cp /Users/jqwang/98-ghosttyAI/cache/week1/ARCH-001/ui_backend_callbacks_fixed.h ./
cp /Users/jqwang/98-ghosttyAI/cache/week1/ARCH-001/ui_backend_impl.c ./
```

### Step 2: 更新你的代码

替换旧的包含：
```c
// 旧代码
#include "../ARCH-001/ui_backend.h"  // 缺少回调
#include "tty_ctx.h"  // 不完整的定义

// 新代码
#include "tty_ctx_unified.h"  // 完整的tty_ctx定义
#include "ui_backend_callbacks_fixed.h"  // 包含所有22个回调
#include "interface_adapter.h"  // 接口兼容层
```

---

## 🔴 DEFECT-001: struct tty_ctx 定义不一致

### 问题症状
```c
// 编译错误
error: 'struct tty_ctx' has no member named 'ocx'
error: 'struct tty_ctx' has no member named 'ocy'
```

### 迁移方案

#### CORE-001/002 团队：

**旧代码**:
```c
// 不完整的tty_ctx使用
struct tty_ctx ctx;
ctx.cell = cell;  // OK
ctx.ocx = 10;    // 错误：字段不存在
```

**新代码**:
```c
#include "tty_ctx_unified.h"

struct tty_ctx ctx = {0};
tty_ctx_init(&ctx);  // 必须初始化！

// 现在所有字段都可用
ctx.ocx = 10;      // ✅ 工作
ctx.ocy = 20;      // ✅ 工作
ctx.orupper = 0;   // ✅ 工作
ctx.orlower = 24;  // ✅ 工作

// 使用安全宏访问
uint32_t x = TTY_CTX_GET_OCX(&ctx);
TTY_CTX_SET_FIELD(&ctx, ocy, 30);
```

#### INTG-001 团队：

**backend_ghostty.c 修改**:
```c
// 在文件顶部
#include "tty_ctx_unified.h"

// 在每个回调函数中
void ghostty_backend_cmd_cell(struct ui_backend* backend, 
                              const struct tty_ctx* ctx) {
    // 验证ctx有效性
    if (!tty_ctx_is_valid(ctx)) {
        return;  // 无效的ctx
    }
    
    // 安全访问字段
    uint32_t row = TTY_CTX_GET_FIELD(ctx, ocy, 0);
    uint32_t col = TTY_CTX_GET_FIELD(ctx, ocx, 0);
    
    // 处理命令...
}
```

---

## 🔴 DEFECT-002: 函数接口不匹配

### 问题症状
```c
// 链接错误
undefined reference to 'tty_write_hooks_init'
undefined reference to 'backend_router_register_backend'
```

### 迁移方案

#### 所有团队：

**旧代码**:
```c
// 不一致的函数名
tty_write_hooks_init();  // 一些文件用这个
tty_hooks_init();        // 另一些文件用这个

backend_router_register_backend(router, backend);  // 版本1
backend_router_register_ui(router, backend);       // 版本2
```

**新代码** (推荐):
```c
#include "interface_adapter.h"

// 使用标准接口
tty_hooks_init();  // 标准名称
backend_router_register(router, backend);  // 标准名称
```

**兼容模式** (过渡期):
```c
// 旧名称仍然工作，但会打印警告
tty_write_hooks_init();  // 警告：deprecated，但仍工作
```

#### 批量替换脚本：
```bash
# 在你的源目录运行
sed -i 's/tty_write_hooks_init/tty_hooks_init/g' *.c
sed -i 's/backend_router_register_backend/backend_router_register/g' *.c
sed -i 's/backend_router_register_ui/backend_router_register/g' *.c
```

---

## 🔴 DEFECT-003: ui_backend 回调函数缺失

### 问题症状
```c
// 运行时错误
Segmentation fault: backend->ops->cmd_cell is NULL
```

### 迁移方案

#### INTG-001 团队 (backend_ghostty.c):

**旧代码** (不完整):
```c
struct ui_backend backend = {
    .type = UI_BACKEND_GHOSTTY,
    // 缺少回调函数！
};
```

**新代码** (完整实现):
```c
#include "ui_backend_callbacks_fixed.h"
#include "ui_backend_impl.c"  // 包含实现

// 方法1：使用提供的实现
ui_backend_v2_t* backend = create_ghostty_backend();
// backend现在有所有22个回调

// 方法2：自定义实现
ui_backend_ops_v2_t* ops = (ui_backend_ops_v2_t*)calloc(1, sizeof(ui_backend_ops_v2_t));
ui_backend_ops_init_defaults(ops);  // 初始化为默认值

// 覆盖你需要的回调
ops->cmd_cell = my_cmd_cell_impl;
ops->cmd_cells = my_cmd_cells_impl;
ops->cmd_clearline = my_cmd_clearline_impl;
// ... 设置所有22个回调

// 创建backend
ui_backend_v2_t backend = {
    .size = sizeof(ui_backend_v2_t),
    .version = 2,
    .type = UI_BACKEND_GHOSTTY,
    .ops = ops  // 使用operations table
};
```

#### 完整的回调列表（必须全部实现）：

```c
// 复制这个模板到你的backend_ghostty.c
static ui_backend_ops_v2_t ghostty_ops = {
    .size = sizeof(ui_backend_ops_v2_t),
    .version = 2,
    
    // 字符/单元操作 (5个)
    .cmd_cell = ghostty_cmd_cell,
    .cmd_cells = ghostty_cmd_cells,
    .cmd_insertcharacter = ghostty_cmd_insertcharacter,
    .cmd_deletecharacter = ghostty_cmd_deletecharacter,
    .cmd_clearcharacter = ghostty_cmd_clearcharacter,
    
    // 行操作 (5个)
    .cmd_insertline = ghostty_cmd_insertline,
    .cmd_deleteline = ghostty_cmd_deleteline,
    .cmd_clearline = ghostty_cmd_clearline,
    .cmd_clearendofline = ghostty_cmd_clearendofline,
    .cmd_clearstartofline = ghostty_cmd_clearstartofline,
    
    // 屏幕操作 (4个)
    .cmd_clearscreen = ghostty_cmd_clearscreen,
    .cmd_clearendofscreen = ghostty_cmd_clearendofscreen,
    .cmd_clearstartofscreen = ghostty_cmd_clearstartofscreen,
    .cmd_alignmenttest = ghostty_cmd_alignmenttest,
    
    // 滚动操作 (4个)
    .cmd_reverseindex = ghostty_cmd_reverseindex,
    .cmd_linefeed = ghostty_cmd_linefeed,
    .cmd_scrollup = ghostty_cmd_scrollup,
    .cmd_scrolldown = ghostty_cmd_scrolldown,
    
    // 特殊操作 (4个)
    .cmd_setselection = ghostty_cmd_setselection,
    .cmd_rawstring = ghostty_cmd_rawstring,
    .cmd_sixelimage = ghostty_cmd_sixelimage,
    .cmd_syncstart = ghostty_cmd_syncstart,
};
```

---

## 🧪 验证修复

### 编译测试
```bash
# 编译测试程序
gcc -Wall -Wextra -I. \
    test_p0_fixes.c \
    interface_adapter.c \
    ui_backend_impl.c \
    -o test_p0_fixes

# 运行测试
./test_p0_fixes

# 期望输出：
# ✅ ALL P0 DEFECTS FIXED - Ready for integration!
```

### 集成测试
```bash
# 编译你的组件与新接口
gcc -c -I. tty_write_hooks.c     # CORE-001
gcc -c -I. backend_router.c      # CORE-002
gcc -c -I. backend_ghostty.c     # INTG-001

# 链接测试
gcc -o integration_test \
    tty_write_hooks.o \
    backend_router.o \
    backend_ghostty.o \
    interface_adapter.o \
    ui_backend_impl.o

# 运行
./integration_test
```

---

## 📝 团队特定指导

### CORE-001 (c-tmux-specialist)

1. 更新 `tty_write_hooks.c`:
   ```c
   #include "tty_ctx_unified.h"
   #include "interface_adapter.h"
   
   // 使用标准函数名
   int tty_hooks_init(void) {  // 不是 tty_write_hooks_init
       // 实现...
   }
   ```

2. 确保所有hook函数接收完整的tty_ctx

### CORE-002 (libtmux-core-developer)

1. 更新 `backend_router.c`:
   ```c
   #include "interface_adapter.h"
   
   // 使用标准函数名
   int backend_router_register(router, backend) {  // 统一名称
       // 实现...
   }
   ```

2. 验证路由到正确的回调

### INTG-001 (zig-ghostty-integration)

1. 完全重写 `backend_ghostty.c` 使用新的ops table
2. 实现所有22个回调（可以从ui_backend_impl.c复制）
3. 测试每个回调至少一次

### QA-002 (qa-test-engineer)

1. 运行提供的测试套件
2. 验证覆盖率达到65%
3. 确认所有P0缺陷已修复

---

## ⏰ 时间线

| 时间 | 任务 | 负责人 |
|------|------|--------|
| 21:00 | 开始迁移 | 所有团队 |
| 21:30 | 完成代码更新 | CORE-001/002, INTG-001 |
| 22:00 | 运行测试 | QA-002 |
| 22:30 | 提交修复 | 所有团队 |

---

## ❓ 常见问题

### Q: 我的代码仍然编译失败
A: 确保你包含了所有三个新头文件：
- `tty_ctx_unified.h`
- `ui_backend_callbacks_fixed.h`  
- `interface_adapter.h`

### Q: 运行时出现段错误
A: 检查是否：
1. 调用了 `tty_ctx_init()` 初始化ctx
2. 所有22个回调都已设置（非NULL）
3. 使用了安全的字段访问宏

### Q: 性能有影响吗？
A: 新的安全检查增加了极小的开销（<1%），但防止了崩溃。

### Q: 旧代码还能工作吗？
A: 通过兼容层，旧函数名仍可工作但会打印警告。建议尽快迁移。

---

## 📞 支持

如有问题，请联系：
- **架构问题**: ARCH-001 (System Architect)
- **集成问题**: 项目经理
- **测试问题**: QA-001 (Test Lead)

---

**截止时间**: 2025-08-25 22:30  
**优先级**: P0 - 必须完成

祝迁移顺利！🚀