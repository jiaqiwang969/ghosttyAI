# CORE-001 任务清单 - 高级C开发工程师
## 第一周：提取 tty_write Hooks

### 任务概览
**角色**：Senior C Developer (tmux Specialist)  
**任务ID**：T-101  
**预计工期**：2天  
**开始时间**：2025-01-06 (周一) 下午  
**完成时间**：2025-01-07 (周二) 下午  

### 主要任务：提取和重构 tty_write 调用

#### 输入 (Inputs)
1. **架构设计** (来自 ARCH-001)
   - `ui_backend.h` - UI backend 接口定义 (周一下午获得)
   - `ui_backend_design.md` - 设计文档

2. **源代码文件**
   - `/Users/jqwang/98-ghosttyAI/tmux/tty.c` - TTY 输出实现
   - `/Users/jqwang/98-ghosttyAI/tmux/tty-term.c` - 终端能力
   - `/Users/jqwang/98-ghosttyAI/tmux/screen-write.c` - 屏幕写入
   - `/Users/jqwang/98-ghosttyAI/tmux/screen-redraw.c` - 屏幕重绘

3. **参考文档**
   - `/Users/jqwang/98-ghosttyAI/docs/architecture-view/tty-write-interception.md`

#### 输出 (Outputs)
1. **tty_write_hooks.c** - 提取的 hooks 实现
   ```c
   // 所有 tty_cmd_* 函数的 hook 点
   void tty_cmd_insertcharacter_hook(struct tty *tty, const struct tty_ctx *ctx);
   void tty_cmd_deletecharacter_hook(struct tty *tty, const struct tty_ctx *ctx);
   // ... 更多 hooks
   ```

2. **tty_cmd_inventory.md** - 完整的 tty_cmd 函数清单
   - 所有 tty_cmd_* 函数列表
   - 每个函数的参数和用途
   - 调用频率分析
   - 性能影响评估

3. **refactoring_plan.md** - 重构计划
   - 修改点列表
   - 风险评估
   - 回退策略

#### 具体步骤 (Steps)

**周一下午 (获得 ui_backend.h 后)**
1. **14:00-15:00** - 与 ARCH-001 评审 ui_backend.h
2. **15:00-17:00** - 扫描所有 tty_cmd_* 函数
3. **17:00-18:00** - 创建函数清单和分类

**周二全天**
1. **9:00-10:00** - 设计 hook 机制
2. **10:00-12:00** - 实现核心 hooks
   - `tty_cmd_cell` - 单元格绘制
   - `tty_cmd_clearline` - 清除行
   - `tty_cmd_cursormove` - 光标移动
3. **13:00-15:00** - 实现剩余 hooks
4. **15:00-16:00** - 单元测试
5. **16:00-17:00** - 与 CORE-002 交接
6. **17:00-18:00** - 文档整理

#### 代码示例 (Code Examples)
```c
// 原始代码
void tty_cmd_insertcharacter(struct tty *tty, const struct tty_ctx *ctx) {
    // 直接 TTY 输出
    tty_putcode1(tty, TTYC_ICH, ctx->num);
}

// 重构后
void tty_cmd_insertcharacter(struct tty *tty, const struct tty_ctx *ctx) {
    if (tty->backend && tty->backend->insert_char) {
        tty->backend->insert_char(ctx->num, ctx->x, ctx->y);
    } else {
        // 回退到传统 TTY
        tty_putcode1(tty, TTYC_ICH, ctx->num);
    }
}
```

#### 验收标准 (Acceptance Criteria)
- [ ] 识别所有 tty_cmd_* 函数（预计 30-40 个）
- [ ] 每个函数都有对应的 hook 点
- [ ] 保持向后兼容，不破坏现有 TTY 模式
- [ ] 编译通过，无警告
- [ ] 基础单元测试通过
- [ ] 性能开销 <5%

#### 协作要求 (Collaboration)
- **依赖 ARCH-001**：需要 ui_backend.h 接口定义
- **输出给 CORE-002**：hooks 列表和实现，用于 backend router
- **支持 QA-002**：提供测试用例

#### 风险点 (Risk Points)
1. **风险**：遗漏某些 tty_cmd 函数
   - **缓解**：使用 grep 和代码分析工具全面扫描
2. **风险**：破坏现有功能
   - **缓解**：保留原始路径，添加开关控制
3. **风险**：性能退化
   - **缓解**：使用内联函数，减少间接调用

#### 测试计划 (Test Plan)
1. 单元测试每个 hook 函数
2. 集成测试 tmux 基本操作
3. 性能基准测试

#### 每日汇报 (Daily Report)
```
STATUS [CORE-001] [日期]
任务：T-101 提取 tty_write hooks
完成：[已完成项]
进行中：[当前工作]
阻塞：[如有]
明日：[计划]
交付：[可交付项]
```

### 代码审查清单 (Code Review Checklist)
- [ ] 所有 hooks 都有注释
- [ ] 错误处理完善
- [ ] 内存管理正确
- [ ] 线程安全考虑
- [ ] 性能影响可接受