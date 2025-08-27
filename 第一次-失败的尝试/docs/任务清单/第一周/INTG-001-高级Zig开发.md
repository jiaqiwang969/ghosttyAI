# INTG-001 任务清单 - 高级Zig开发工程师
## 第一周：Ghostty Backend Stub 实现

### 任务概览
**角色**：Senior Zig Developer (FFI Specialist)  
**任务ID**：T-104  
**预计工期**：2天  
**开始时间**：2025-01-08 (周三)  
**完成时间**：2025-01-09 (周四)  

### 主要任务：创建 Ghostty 后端初始实现

#### 输入 (Inputs)
1. **架构设计** (来自 ARCH-001)
   - `ui_backend.h` - UI backend 接口定义
   - `backend_vtable_spec.md` - 回调规范

2. **网格结构** (来自 tmux 源码)
   - `/Users/jqwang/98-ghosttyAI/tmux/grid.c` - 网格数据结构
   - `/Users/jqwang/98-ghosttyAI/tmux/screen.c` - 屏幕管理

3. **参考实现**
   - `/Users/jqwang/98-ghosttyAI/example/ghostty_tmx_bridge.zig`
   - `/Users/jqwang/98-ghosttyAI/example/libtmuxcore.h`

#### 输出 (Outputs)
1. **backend_ghostty.c** - Ghostty 后端 C 实现
   ```c
   // Ghostty 后端实现
   static void ghostty_draw_cells(struct grid_cell_span *spans, int count) {
       // 合并相邻 spans
       // 批量发送给 Ghostty
       // 性能优化：减少回调次数
   }
   
   static struct ui_backend ghostty_backend = {
       .draw_cells = ghostty_draw_cells,
       .cursor_move = ghostty_cursor_move,
       .clear_region = ghostty_clear_region,
       // ... 所有回调
   };
   ```

2. **span_optimizer.c** - Span 合并优化器
   - 将零散的单元格合并为行
   - 减少回调次数
   - 提高渲染效率

3. **ghostty_types.h** - Ghostty 数据类型定义
   - C 侧的类型定义
   - 与 Zig 的映射关系
   - 内存布局保证

4. **performance_metrics.md** - 性能基准
   - 回调频率分析
   - 内存使用统计
   - 优化建议

#### 具体步骤 (Steps)

**周三全天**
1. **9:00-10:00** - 分析 grid.c 数据结构
2. **10:00-12:00** - 实现核心回调
   - `draw_cells` - 最重要的接口
   - `cursor_move` - 光标控制
   - `clear_region` - 区域清除
3. **13:00-15:00** - 实现 span 优化器
   ```c
   // Span 合并算法
   void optimize_spans(struct grid_cell_span *input, int count,
                       struct grid_cell_span **output, int *out_count) {
       // 合并相邻的相同属性 spans
       // 整行优化
       // 减少碎片
   }
   ```
4. **15:00-17:00** - 性能测试和优化
5. **17:00-18:00** - 与 CORE-002 集成测试

**周四全天**
1. **9:00-11:00** - 实现剩余回调
   - 滚动操作
   - 属性设置
   - 模式切换
2. **11:00-12:00** - 内存管理优化
3. **13:00-14:00** - 创建测试用例
4. **14:00-15:00** - 性能基准测试
5. **15:00-16:00** - 文档编写
6. **16:00-17:00** - 交付给 QA-002
7. **17:00-18:00** - 问题修复

#### 关键优化点 (Optimization Points)
```c
// 批量处理示例
typedef struct batch_context {
    struct grid_cell_span spans[MAX_BATCH_SIZE];
    int span_count;
    uint64_t last_flush_time;
} batch_context_t;

void ghostty_draw_cells(struct grid_cell_span *spans, int count) {
    static batch_context_t batch = {0};
    
    // 添加到批次
    memcpy(&batch.spans[batch.span_count], spans, 
           count * sizeof(struct grid_cell_span));
    batch.span_count += count;
    
    // 批量刷新条件
    if (batch.span_count >= MAX_BATCH_SIZE ||
        (get_time_ns() - batch.last_flush_time) > FLUSH_INTERVAL_NS) {
        flush_batch(&batch);
    }
}
```

#### 性能目标 (Performance Goals)
1. **回调频率**：≤1 per vsync (60Hz)
2. **Span 合并率**：>60% 减少
3. **内存占用**：<100KB 缓冲区
4. **延迟**：<1ms 批处理延迟

#### 验收标准 (Acceptance Criteria)
- [ ] 所有必需的回调都已实现
- [ ] Span 合并有效减少回调次数
- [ ] 内存使用在限制内
- [ ] 性能达到目标（≤1 callback/frame）
- [ ] 与 CORE-002 的路由器集成成功
- [ ] 基础渲染测试通过

#### 协作要求 (Collaboration)
- **依赖 ARCH-001**：UI backend 接口定义
- **协作 CORE-002**：集成到路由器
- **输出给 QA-002**：可测试的后端
- **未来协作 INTG-002**：Week 3 的 Zig FFI

#### FFI 准备 (FFI Preparation)
```c
// 为 Week 3 的 Zig FFI 准备
#ifdef __cplusplus
extern "C" {
#endif

// 导出给 Zig 的接口
struct ghostty_backend_ops {
    void (*init)(void);
    void (*shutdown)(void);
    struct ui_backend* (*get_backend)(void);
};

__attribute__((visibility("default")))
struct ghostty_backend_ops* ghostty_backend_get_ops(void);

#ifdef __cplusplus
}
#endif
```

#### 风险点 (Risk Points)
1. **风险**：回调频率过高
   - **缓解**：批量处理和合并
2. **风险**：内存碎片
   - **缓解**：使用内存池
3. **风险**：线程安全问题
   - **缓解**：明确单线程或加锁

#### 测试计划 (Test Plan)
1. **单元测试**
   - Span 合并算法
   - 批处理逻辑
   - 内存管理
2. **集成测试**
   - 与路由器集成
   - 基本渲染
3. **性能测试**
   - 回调频率测量
   - 内存使用监控

#### 每日汇报 (Daily Report)
```
STATUS [INTG-001] [日期]
任务：T-104 Ghostty Backend Stub
完成：[已实现回调]
进行中：[当前工作]
性能：[回调频率，内存使用]
阻塞：[如有]
明日：[计划]
```