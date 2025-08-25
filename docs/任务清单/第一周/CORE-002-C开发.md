# CORE-002 任务清单 - C开发工程师
## 第一周：实现 Backend Router

### 任务概览
**角色**：C Developer (libtmuxcore)  
**任务ID**：T-103  
**预计工期**：3天  
**开始时间**：2025-01-07 (周二) 下午  
**完成时间**：2025-01-09 (周四) 下午  

### 主要任务：实现多后端路由器

#### 输入 (Inputs)
1. **架构设计** (来自 ARCH-001)
   - `ui_backend.h` - UI backend 接口定义
   - `backend_vtable_spec.md` - vtable 规范

2. **Hook 实现** (来自 CORE-001，周二下午获得)
   - `tty_write_hooks.c` - 提取的 hooks
   - `tty_cmd_inventory.md` - 函数清单

3. **源代码**
   - `/Users/jqwang/98-ghosttyAI/tmux/tty.c`
   - `/Users/jqwang/98-ghosttyAI/tmux/options.c` - 配置系统

#### 输出 (Outputs)
1. **backend_router.c** - 路由器核心实现
   ```c
   struct backend_router {
       struct ui_backend *active_backend;
       struct ui_backend *tty_backend;
       struct ui_backend *ghostty_backend;
       enum backend_type current_type;
   };
   
   void router_switch_backend(struct backend_router *router, enum backend_type type);
   void router_route_call(struct backend_router *router, enum tty_cmd cmd, ...);
   ```

2. **backend_tty.c** - 传统 TTY 后端适配器
   - 将现有 TTY 代码包装成 ui_backend 接口
   - 保持 100% 兼容性

3. **backend_config.c** - 后端配置管理
   - 运行时切换机制
   - 配置文件支持
   - 环境变量支持

4. **router_test.c** - 路由器测试套件

#### 具体步骤 (Steps)

**周二下午 (获得 hooks 后)**
1. **16:00-18:00** - 学习 CORE-001 的 hooks 实现

**周三全天**
1. **9:00-10:00** - 设计路由器架构
2. **10:00-12:00** - 实现 backend_router 核心
   ```c
   // 路由决策逻辑
   if (router->current_type == BACKEND_GHOSTTY) {
       router->ghostty_backend->draw_cells(spans, count);
   } else {
       router->tty_backend->draw_cells(spans, count);
   }
   ```
3. **13:00-15:00** - 实现 backend_tty 适配器
4. **15:00-17:00** - 实现配置管理
5. **17:00-18:00** - 基础测试

**周四全天**
1. **9:00-11:00** - 完善路由逻辑
   - 错误处理
   - 回退机制
   - 性能优化
2. **11:00-12:00** - 与 INTG-001 集成测试
3. **13:00-15:00** - 性能测试和优化
4. **15:00-16:00** - 完成文档
5. **16:00-17:00** - 交付给 QA-002
6. **17:00-18:00** - 代码审查修复

#### 关键实现点 (Implementation Points)
```c
// backend_router.c 核心结构
typedef struct backend_router {
    // 后端注册表
    struct ui_backend *backends[BACKEND_MAX];
    int backend_count;
    
    // 当前活跃后端
    struct ui_backend *active;
    
    // 性能统计
    struct {
        uint64_t call_count;
        uint64_t total_time;
    } stats[TTY_CMD_MAX];
} backend_router_t;

// 路由函数示例
void router_draw_cells(backend_router_t *r, struct grid_cell_span *spans, int n) {
    if (!r->active) {
        log_error("No active backend");
        return;
    }
    
    uint64_t start = get_time_ns();
    r->active->draw_cells(spans, n);
    uint64_t elapsed = get_time_ns() - start;
    
    r->stats[TTY_CMD_CELLS].call_count++;
    r->stats[TTY_CMD_CELLS].total_time += elapsed;
}
```

#### 验收标准 (Acceptance Criteria)
- [ ] 路由器支持至少 2 个后端（TTY 和 Ghostty）
- [ ] 运行时可切换后端
- [ ] TTY 后端 100% 兼容现有行为
- [ ] 性能开销 <2% (路由层)
- [ ] 完整的错误处理和日志
- [ ] 通过所有单元测试

#### 协作要求 (Collaboration)
- **依赖 CORE-001**：需要 hooks 实现
- **依赖 ARCH-001**：需要接口定义
- **协作 INTG-001**：集成 Ghostty backend
- **输出给 QA-002**：可测试的路由器

#### 配置示例 (Configuration)
```conf
# .tmux.conf
set -g backend-type ghostty    # 使用 Ghostty 后端
set -g backend-fallback tty     # 回退到 TTY
set -g backend-log-level debug  # 调试日志
```

#### 性能目标 (Performance Targets)
- 路由开销：<100ns per call
- 内存占用：<1MB 额外内存
- 零拷贝：直接传递指针
- 缓存友好：热路径内联

#### 风险点 (Risk Points)
1. **风险**：路由开销影响性能
   - **缓解**：使用函数指针直接调用
2. **风险**：后端切换时状态不一致
   - **缓解**：实现状态同步机制
3. **风险**：兼容性问题
   - **缓解**：充分测试 TTY 后端

#### 测试计划 (Test Plan)
1. **单元测试**
   - 每个路由函数
   - 后端切换逻辑
   - 错误处理路径
2. **集成测试**
   - 与 INTG-001 的 Ghostty backend
   - TTY 后端兼容性
3. **性能测试**
   - 路由开销测量
   - 压力测试

#### 每日汇报 (Daily Report)
```
STATUS [CORE-002] [日期]
任务：T-103 Backend Router
完成：[已完成功能]
进行中：[当前开发]
阻塞：[问题]
需要：[依赖项]
明日：[计划]
```