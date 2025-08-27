# CORE-001 第二周任务清单
## c-tmux-specialist Week 2 Tasks

### 👤 角色定位
**专长**: C语言tmux核心开发，事件循环，终端控制  
**本周重点**: 事件循环vtable化，布局管理callbacks

### 📋 任务列表

## T-201: 事件循环vtable抽象 【高优先级】
**工期**: 3天 (周一-周三)  
**状态**: 待开始

### 输入文件分析
```bash
# 需要分析的tmux源文件
/Users/jqwang/98-ghosttyAI/tmux/event.c
/Users/jqwang/98-ghosttyAI/tmux/server-loop.c  
/Users/jqwang/98-ghosttyAI/tmux/proc.c
```

### 输出交付物
```
cache/week2/CORE-001/
├── event_loop_backend.h      # 事件循环抽象接口
├── event_loop_router.c       # 事件路由实现
├── event_callbacks.c         # 回调函数集合
└── tests/
    └── test_event_loop.c     # 事件循环测试
```

### 实现步骤
1. **周一上午**: 分析tmux事件循环机制
   - 理解event_add/event_del工作原理
   - 梳理所有事件类型(timer, io, signal)
   - 确定需要抽象的接口

2. **周一下午**: 设计vtable结构
   ```c
   typedef struct event_backend_ops {
       int (*add)(struct event_base*, struct event*);
       int (*del)(struct event_base*, struct event*);
       int (*dispatch)(struct event_base*);
       int (*loop)(struct event_base*, int flags);
   } event_backend_ops_t;
   ```

3. **周二全天**: 实现event_loop_router
   - 保持与libevent兼容
   - 支持callback模式切换
   - 线程安全考虑

4. **周三上午**: 性能测试和优化
   - 基准测试对比原生tmux
   - 确保性能损失<1%

### 验收标准
- [ ] 所有event操作通过vtable
- [ ] Poll/select/kqueue统一接口  
- [ ] 性能测试通过(差异<1%)
- [ ] 线程安全验证
- [ ] 100%向后兼容

### 关键风险
- libevent深度耦合可能难以解耦
- 性能开销需要严格控制

---

## T-203: 布局管理callbacks 【中优先级】
**工期**: 2天 (周四-周五)  
**状态**: 待T-202完成

### 输入文件分析
```bash
/Users/jqwang/98-ghosttyAI/tmux/layout.c
/Users/jqwang/98-ghosttyAI/tmux/layout-custom.c
/Users/jqwang/98-ghosttyAI/tmux/layout-set.c
```

### 输出交付物
```
cache/week2/CORE-001/
├── layout_callbacks.h        # 布局回调接口
├── layout_callbacks.c        # 布局回调实现
├── pane_events.c            # 窗格事件处理
└── tests/
    └── test_layout.c        # 布局测试
```

### 实现步骤
1. **周四上午**: 布局事件识别
   - 窗格创建/销毁事件
   - 窗格分割/合并事件
   - 尺寸调整事件

2. **周四下午**: 回调机制实现
   ```c
   typedef struct layout_callbacks {
       void (*on_pane_create)(struct window_pane*);
       void (*on_pane_destroy)(struct window_pane*);
       void (*on_pane_resize)(struct window_pane*, u_int, u_int);
       void (*on_layout_change)(struct window*);
   } layout_callbacks_t;
   ```

3. **周五上午**: 布局序列化
   - 布局状态导出
   - 布局恢复机制

4. **周五下午**: 集成测试

### 验收标准
- [ ] 所有布局变化触发callbacks
- [ ] 窗格操作事件完整
- [ ] 布局可序列化/反序列化
- [ ] 与T-202网格操作协同

---

## 📅 每日计划

### 周一 (8/26)
- 09:00 - 参加周会，确认任务
- 10:00 - 开始T-201，分析event.c
- 14:00 - 设计event_backend_ops结构
- 16:00 - 编写接口头文件
- 17:00 - 提交进展，日报

### 周二 (8/27)
- 09:00 - 站会同步
- 09:30 - 实现event_loop_router核心
- 14:00 - 处理线程安全问题
- 16:00 - 单元测试编写
- 17:00 - 与CORE-002同步接口

### 周三 (8/28)
- 09:00 - 站会
- 09:30 - 性能测试setup
- 11:00 - 运行benchmark对比
- 14:00 - 性能优化（如需要）
- 16:00 - 完成T-201，交付给INTG-001
- 17:00 - 准备T-203

### 周四 (8/29)
- 09:00 - 站会
- 09:30 - 开始T-203布局管理
- 11:00 - 实现layout_callbacks
- 14:00 - 窗格事件处理
- 16:00 - 与INTG-002协调copy-mode
- 17:00 - 进展同步

### 周五 (8/30)
- 09:00 - 站会
- 09:30 - 布局序列化实现
- 11:00 - 集成测试
- 14:00 - Bug修复
- 16:00 - 周总结文档
- 17:00 - Demo准备

## 🔄 协作接口

### 需要从其他人获取
- ARCH-001: 事件循环架构确认
- CORE-002: 网格操作接口(T-202)

### 需要提供给其他人
- INTG-001: event_loop_backend.h (周三)
- INTG-002: layout_callbacks.h (周五)
- QA-001: 测试用例说明

## 📊 风险和缓解

| 风险 | 影响 | 缓解措施 |
|------|------|----------|
| libevent解耦困难 | T-201延期 | 保留libevent兼容层 |
| 性能退化 | 不可接受 | 提前做性能基准测试 |
| 接口变化 | 影响下游 | 尽早锁定接口设计 |

## ✅ 周完成标准
- [ ] T-201完成，性能达标
- [ ] T-203完成，测试通过
- [ ] 文档完整
- [ ] 0编译警告
- [ ] 代码review通过

---

**角色**: CORE-001 (c-tmux-specialist)  
**更新日期**: 2025-08-25