# Ghostty × tmux 集成项目施工图
# Construction Blueprint for Ghostty × tmux Integration

## 🎯 项目愿景 Project Vision
将 tmux 核心以 libtmuxcore 库形式完全内嵌入 Ghostty，实现零 VT 序列开销的原生 GPU 加速终端复用器。

**核心创新**: 直接网格更新，绕过终端仿真层，实现 60+ FPS 渲染性能。

## 📊 执行摘要 Executive Summary

### 项目规模 Project Scale
- **工期**: 20 工作日（4 周）
- **团队**: 11 名工程师
- **代码变更**: ~5000 行新代码，~500 行修改
- **关键路径**: 10 天（不可压缩）

### 成功标准 Success Criteria
- ✅ 零 VT 序列生成/解析开销
- ✅ 60 FPS 稳定渲染
- ✅ 100% tmux 命令兼容性
- ✅ ABI 稳定性保证
- ✅ 85% 测试覆盖率

## 🏗️ 施工总览 Construction Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     施工阶段 Phases                          │
├─────────────────────────────────────────────────────────────┤
│ Week 1: 地基 Foundation                                      │
│   └─> UI Backend 抽象层                                      │
│                                                              │
│ Week 2: 框架 Framework                                       │
│   └─> 事件循环 + 回调系统                                    │
│                                                              │
│ Week 3: 集成 Integration                                     │
│   └─> FFI 桥接 + Ghostty 连接                               │
│                                                              │
│ Week 4: 完成 Completion                                      │
│   └─> 测试 + 优化 + 发布                                    │
└─────────────────────────────────────────────────────────────┘
```

## 👥 施工团队 Construction Team

### 组织架构 Organization Structure

```
项目总监 Project Director
    ├── 技术总监 ARCH-001 (Lead Architect)
    │
    ├── 核心团队 Core Team
    │   ├── CORE-001 (Sr. C Developer - tmux专家)
    │   └── CORE-002 (C Developer - libtmuxcore)
    │
    ├── 集成团队 Integration Team
    │   ├── INTG-001 (Sr. Zig Developer - FFI专家)
    │   ├── INTG-002 (Zig Developer - Ghostty集成)
    │   └── INTG-003 (Performance Engineer)
    │
    ├── 质量团队 Quality Team
    │   ├── QA-001 (Test Lead)
    │   ├── QA-002 (Test Engineer - 网格测试)
    │   └── QA-003 (Performance Test Engineer)
    │
    └── 运维团队 Operations Team
        ├── OPS-001 (CI/CD Engineer)
        └── OPS-002 (Release Engineer)
```

### 人员职责矩阵 RACI Matrix

| 组件 Component | 负责 R | 批准 A | 咨询 C | 通知 I |
|---------------|--------|--------|--------|--------|
| UI Backend设计 | CORE-001 | ARCH-001 | INTG-001 | 全体 |
| 事件循环抽象 | CORE-001 | ARCH-001 | CORE-002 | INTG-* |
| FFI桥接层 | INTG-001 | ARCH-001 | CORE-001 | QA-* |
| Ghostty集成 | INTG-002 | INTG-001 | ARCH-001 | QA-* |
| 性能优化 | INTG-003 | ARCH-001 | QA-003 | 全体 |
| 测试策略 | QA-001 | ARCH-001 | 全体 | OPS-* |

## 📋 详细施工任务 Detailed Construction Tasks

### 第一周：地基工程 Week 1: Foundation

#### 🏷️ T-101: 提取 tty_write 钩子
**负责人**: CORE-001  
**工期**: 2天  
**依赖**: 无  

**施工步骤**:
1. 分析 tmux/tty.c 中所有 22 个 tty_cmd_* 函数
2. 创建 ui_backend.h 接口定义
3. 实现钩子注入机制
4. 保持原有 TTY 路径完整

**验收标准**:
- [ ] ui_backend.h 包含所有 22 个命令
- [ ] 编译通过，无警告
- [ ] 传统 tmux 功能未受影响

**代码位置**:
```c
// tmux/ui_backend.h
typedef struct ui_backend {
    void (*cmd_clear_line)(struct tty_ctx*);
    void (*cmd_cursor_move)(struct tty_ctx*);
    void (*cmd_insert_characters)(struct tty_ctx*);
    // ... 19 more commands
} ui_backend_t;
```

---

#### 🏷️ T-102: UI Backend 架构设计
**负责人**: ARCH-001  
**工期**: 1天  
**依赖**: 无  

**设计要点**:
- 帧批处理机制（16.67ms 聚合）
- 脏区跟踪算法
- 背压控制策略
- ABI 稳定性保证

**交付物**:
- [ ] 设计文档（含 PlantUML 图）
- [ ] API 规范
- [ ] 性能模型

---

#### 🏷️ T-103: Backend 路由器实现
**负责人**: CORE-002  
**工期**: 3天  
**依赖**: T-101, T-102  

**实现内容**:
```c
// tmux/backend_router.c
void route_tty_cmd(struct tty* tty, tty_cmd_fn cmd, struct tty_ctx* ctx) {
    if (tty->backend_mode == BACKEND_UI) {
        ui_backend_dispatch(tty->ui_backend, cmd, ctx);
    } else {
        // Traditional TTY path
        (*cmd)(tty, ctx);
    }
}
```

**测试要求**:
- [ ] 单元测试覆盖所有路由路径
- [ ] 性能基准测试（<100ns 开销）

---

#### 🏷️ T-104: Ghostty Backend 桩实现
**负责人**: INTG-001  
**工期**: 2天  
**依赖**: T-102  

**实现内容**:
```c
// tmux/backend_ghostty.c
static void ghostty_cmd_insert_line(struct tty_ctx* ctx) {
    // Convert to span update
    tmc_span_t span = {
        .row = ctx->ocy,
        .col_start = ctx->ocx,
        .col_end = ctx->ocx + ctx->num,
        .cells = convert_to_cells(ctx)
    };
    
    // Aggregate into frame buffer
    frame_buffer_add_span(&g_frame_buffer, &span);
}
```

---

#### 🏷️ T-105: 网格操作测试
**负责人**: QA-002  
**工期**: 2天  
**依赖**: T-103  

**测试场景**:
- [ ] ASCII 文本渲染
- [ ] Unicode/Emoji 处理
- [ ] 双宽字符边界
- [ ] 滚动区域更新
- [ ] 属性变化（颜色、样式）

---

#### 🏷️ T-106: 第一周集成测试
**负责人**: QA-001  
**工期**: 1天  
**依赖**: T-101 到 T-105  

**集成点验证**:
- [ ] Backend 路由正确性
- [ ] 内存泄漏检查
- [ ] 性能回归测试

### 第二周：框架搭建 Week 2: Framework

#### 🏷️ T-201: 事件循环 vtable
**负责人**: CORE-001  
**工期**: 3天  
**依赖**: T-103  

**实现内容**:
```c
// tmux/loop_vtable.c
typedef struct {
    uint32_t size;  // ABI stability
    int (*add_fd)(tmc_io_t*, int fd, int events, tmc_io_fd_cb_t, void*);
    int (*add_timer)(tmc_io_t*, uint64_t ms, tmc_io_timer_cb_t, void*);
    void (*post)(tmc_io_t*, void (*fn)(void*), void*);
} tmc_loop_vtable_t;
```

**关键要求**:
- [ ] 线程安全保证
- [ ] 回调不持锁
- [ ] 支持跨线程 post

---

#### 🏷️ T-202: 网格回调实现
**负责人**: CORE-002  
**工期**: 2天  
**依赖**: T-103  

**帧批处理实现**:
```c
// tmux/grid_callbacks.c
void emit_frame(frame_aggregator_t* agg) {
    tmc_frame_t frame = {
        .size = sizeof(tmc_frame_t),
        .frame_seq = agg->next_seq++,
        .timestamp_ns = get_monotonic_time(),
        .span_count = agg->pending_count,
        .spans = agg->pending_spans,
        .flags = calculate_frame_flags(agg)
    };
    
    // Invoke callback
    if (g_callbacks.on_frame) {
        g_callbacks.on_frame(g_client, &frame, g_user_data);
    }
}
```

---

#### 🏷️ T-203: 布局回调
**负责人**: CORE-001  
**工期**: 2天  
**依赖**: T-103  

**布局更新处理**:
```c
// tmux/layout_callbacks.c
void notify_layout_change(struct window* w) {
    tmc_layout_update_t update = {
        .size = sizeof(tmc_layout_update_t),
        .window_id = w->id,
        .active_pane_id = w->active->id,
        .node_count = count_layout_nodes(w->layout_root),
        .nodes = serialize_layout(w->layout_root)
    };
    
    g_callbacks.on_layout(g_server, &update, g_user_data);
}
```

---

#### 🏷️ T-204: 复制模式回调
**负责人**: INTG-002  
**工期**: 2天  
**依赖**: T-202  

**复制模式集成**:
- [ ] 选区跟踪
- [ ] 搜索高亮
- [ ] 历史滚动
- [ ] 剪贴板集成

### 第三周：集成工程 Week 3: Integration

#### 🏷️ T-301: FFI 绑定层
**负责人**: INTG-001  
**工期**: 3天  
**依赖**: T-201  

**Zig FFI 实现**:
```zig
// ghostty/libtmuxcore.zig
pub const TmuxCore = struct {
    server: *c.tmc_server_t,
    callbacks: c.tmc_ui_vtable_t,
    
    pub fn init(config: Config) !TmuxCore {
        const c_config = try config.toCConfig();
        const server = c.tmc_server_new(
            &c_config,
            &loop_vtable,
            @ptrCast(*c.tmc_io_t, &ghostty_loop),
            &ui_callbacks,
            null
        ) orelse return error.InitFailed;
        
        return TmuxCore{ .server = server };
    }
};
```

---

#### 🏷️ T-302: Ghostty 集成
**负责人**: INTG-002  
**工期**: 3天  
**依赖**: T-301  

**终端缓冲区连接**:
```zig
// ghostty/tmux_bridge.zig
fn onFrame(client: *c.tmc_client_t, frame: *const c.tmc_frame_t, user: ?*anyopaque) callconv(.C) void {
    const self = @ptrCast(*TmuxBridge, @alignCast(@alignOf(TmuxBridge), user));
    
    // Convert spans to Ghostty grid updates
    for (frame.spans[0..frame.span_count]) |span| {
        self.terminal.updateCells(
            span.row,
            span.col_start,
            span.col_end,
            convertCells(span.cells)
        );
    }
    
    // Trigger render
    self.terminal.scheduleRender();
}
```

---

#### 🏷️ T-303: 内存安全层
**负责人**: INTG-001  
**工期**: 2天  
**依赖**: T-301  

**边界检查实现**:
- [ ] 自动生命周期管理
- [ ] 缓冲区溢出保护
- [ ] 空指针检查
- [ ] 类型安全包装

---

#### 🏷️ T-304: 错误处理
**负责人**: INTG-002  
**工期**: 1天  
**依赖**: T-302  

**错误传播机制**:
```zig
// ghostty/error_handling.zig
pub const TmuxError = error{
    ServerInitFailed,
    ClientAttachFailed,
    CommandFailed,
    CallbackError,
    MemoryExhausted,
};

fn handleTmuxError(err: c.tmc_err_t) TmuxError!void {
    switch (err) {
        c.TMC_OK => {},
        c.TMC_ERR_NOMEM => return error.MemoryExhausted,
        else => return error.CommandFailed,
    }
}
```

### 第四周：完成与优化 Week 4: Completion

#### 🏷️ T-401: 集成测试套件
**负责人**: QA-001  
**工期**: 3天  
**依赖**: 所有第三周任务  

**测试覆盖**:
- [ ] vim/neovim 兼容性
- [ ] htop 渲染正确性
- [ ] git log --graph 显示
- [ ] 1000+ panes 压力测试
- [ ] 热插拔测试

---

#### 🏷️ T-402: 性能基准测试
**负责人**: QA-003  
**工期**: 2天  
**依赖**: T-401  

**基准指标**:
- [ ] 启动时间 < 50ms
- [ ] 帧延迟 < 16.67ms (60 FPS)
- [ ] 内存使用 < 10MB/pane
- [ ] CPU 使用 < 5% idle

---

#### 🏷️ T-403: 性能优化
**负责人**: INTG-003  
**工期**: 3天  
**依赖**: T-402  

**优化目标**:
- [ ] Span 合并率 > 50%
- [ ] 批处理效率 > 10 updates/frame
- [ ] 零拷贝路径优化
- [ ] SIMD 加速（可选）

---

#### 🏷️ T-404: Bug 修复
**负责人**: CORE-002  
**工期**: 2天  
**依赖**: T-401  

**修复优先级**:
1. 崩溃和数据丢失
2. 功能缺失
3. 性能问题
4. 视觉问题

## 🔄 关键集成点 Critical Integration Points

### 1. tty_write 拦截点
```c
// 位置: tmux/tty.c:1234
// 修改: 添加 backend 路由
void tty_write(void (*cmdfn)(struct tty *, const struct tty_ctx *),
              struct tty_ctx *ctx) {
    // NEW: Route to backend
    if (ctx->tty->backend) {
        backend_write(ctx->tty->backend, cmdfn, ctx);
        return;
    }
    // Original path unchanged
    (*cmdfn)(ctx->tty, ctx);
}
```

### 2. 帧聚合点
```c
// 位置: tmux/frame_aggregator.c
// 功能: 16.67ms 批处理窗口
typedef struct {
    uint64_t last_emit_ns;
    tmc_span_t spans[MAX_SPANS_PER_FRAME];
    uint32_t span_count;
    
    // Dirty tracking
    uint32_t min_row, max_row;
    uint32_t min_col, max_col;
} frame_aggregator_t;
```

### 3. FFI 边界
```zig
// 位置: ghostty/src/tmux/ffi.zig
// 功能: C-Zig 类型转换
pub fn convertCell(c_cell: c.tmc_cell_t) Cell {
    return Cell{
        .char = @intCast(u21, c_cell.ch),
        .fg = Color.fromRgb(c_cell.fg_rgb),
        .bg = Color.fromRgb(c_cell.bg_rgb),
        .attrs = @bitCast(Attributes, c_cell.attrs),
    };
}
```

## 📊 进度跟踪 Progress Tracking

### 关键路径 Critical Path
```
T-101 (2d) → T-103 (3d) → T-201 (3d) → T-301 (3d) → T-401 (3d) → T-504 (2d)
总计: 16 天（包含缓冲）
```

### 里程碑 Milestones

| 里程碑 | 日期 | 验收标准 | 负责人 |
|--------|------|----------|--------|
| M1: Backend 路由完成 | Day 5 | 所有 tty_cmd 可路由 | CORE-001 |
| M2: 事件循环集成 | Day 10 | 回调系统工作 | CORE-001 |
| M3: FFI 桥接完成 | Day 15 | Zig 可调用 tmux | INTG-001 |
| M4: 首次端到端运行 | Day 18 | 基本 tmux 功能 | QA-001 |
| M5: 生产就绪 | Day 20 | 所有测试通过 | ARCH-001 |

## 🚨 风险管理 Risk Management

### 技术风险 Technical Risks

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|----------|
| ABI 不兼容 | 中 | 高 | 使用 size 字段，版本协商 |
| 性能退化 | 低 | 高 | 早期基准测试，持续监控 |
| 内存泄漏 | 中 | 中 | ASAN/Valgrind，自动测试 |
| 线程死锁 | 低 | 高 | 无锁设计，回调不持锁 |

### 项目风险 Project Risks

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|----------|
| 人员可用性 | 中 | 中 | 交叉培训，文档完善 |
| 需求变更 | 低 | 高 | 每周评审，及早发现 |
| 集成复杂度 | 高 | 中 | 增量集成，频繁测试 |

## 📈 质量保证 Quality Assurance

### 测试策略 Test Strategy

```
单元测试 (持续)
    ├── 每个函数 100% 覆盖
    ├── 边界条件测试
    └── 错误路径测试

集成测试 (每日)
    ├── 组件接口测试
    ├── 端到端场景
    └── 回归测试

性能测试 (每周)
    ├── 基准测试
    ├── 压力测试
    └── 内存分析

验收测试 (里程碑)
    ├── 用户场景
    ├── 兼容性测试
    └── 生产模拟
```

### 代码质量标准 Code Quality Standards

- **代码覆盖率**: ≥ 85%
- **圈复杂度**: ≤ 10
- **代码审查**: 100% (至少 2 人)
- **静态分析**: 0 警告
- **文档**: 所有公共 API

## 🚀 交付计划 Delivery Plan

### Week 1 交付物
- ✅ ui_backend.h 接口
- ✅ Backend 路由器
- ✅ Ghostty backend 桩
- ✅ 基础测试套件

### Week 2 交付物
- ✅ 事件循环抽象
- ✅ 回调系统
- ✅ 帧批处理
- ✅ 布局管理

### Week 3 交付物
- ✅ FFI 绑定
- ✅ Ghostty 集成
- ✅ 错误处理
- ✅ 内存安全

### Week 4 交付物
- ✅ 完整测试套件
- ✅ 性能优化
- ✅ 文档完成
- ✅ 发布包

## 📝 实施检查清单 Implementation Checklist

### 开发环境准备
- [ ] tmux 源码 (v3.3+)
- [ ] Ghostty 源码 (latest)
- [ ] Zig 0.11+
- [ ] C11 编译器
- [ ] PlantUML
- [ ] 测试框架

### 每日检查
- [ ] 代码提交（至少 2 次）
- [ ] 测试运行
- [ ] 文档更新
- [ ] 进度报告

### 每周检查
- [ ] 里程碑评审
- [ ] 性能测试
- [ ] 风险评估
- [ ] 团队同步

## 🎯 成功标准 Success Criteria

### 功能标准
- ✅ 100% tmux 命令兼容
- ✅ 所有快捷键工作
- ✅ 会话持久化
- ✅ 远程连接支持

### 性能标准
- ✅ 60 FPS 渲染
- ✅ < 1ms 输入延迟
- ✅ < 10MB 内存/窗格
- ✅ < 5% CPU 空闲

### 质量标准
- ✅ 0 已知崩溃
- ✅ 85% 测试覆盖
- ✅ 100% API 文档
- ✅ ABI 稳定保证

## 📚 参考资源 References

### 技术文档
- [tmux 源码分析](../architecture-view/integration-architecture.md)
- [tty_write 拦截详解](../architecture-view/tty-write-interception.md)
- [ABI 稳定性策略](../architecture-view/abi-stability.md)
- [帧批处理设计](../architecture-view/frame-batching.md)
- [背压控制机制](../architecture-view/backpressure-control.md)

### 项目管理
- [团队结构](../project-manager-view/team-structure.md)
- [任务分配矩阵](../project-manager-view/task-assignment-matrix.md)
- [进度跟踪](../project-manager-view/ownership-gantt.puml)

---

**文档版本**: 1.0.0  
**最后更新**: 2025-08-25  
**下次评审**: Week 1 结束

> 本施工图为活文档，将根据项目进展持续更新。