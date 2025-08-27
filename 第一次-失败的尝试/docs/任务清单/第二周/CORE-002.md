# CORE-002 第二周任务清单
## libtmux-core-developer Week 2 Tasks

### 👤 角色定位
**专长**: C库开发，内存管理，网格数据结构  
**本周重点**: 网格操作批量化，性能优化

### 📋 任务列表

## T-202: 网格操作callbacks 【高优先级】
**工期**: 2天 (周三-周四)  
**状态**: 待T-201初步完成  
**前置依赖**: T-201事件循环框架

### 输入文件分析
```bash
# 需要深入分析的文件
/Users/jqwang/98-ghosttyAI/tmux/grid.c
/Users/jqwang/98-ghosttyAI/tmux/grid-view.c
/Users/jqwang/98-ghosttyAI/tmux/screen-write.c
/Users/jqwang/98-ghosttyAI/tmux/utf8.c
```

### 输出交付物
```
cache/week2/CORE-002/
├── grid_callbacks.h          # 网格回调接口
├── grid_callbacks.c          # 网格回调实现
├── grid_batch_ops.h         # 批量操作API
├── grid_batch_ops.c         # 批量操作实现
├── dirty_tracking.c         # 脏区域追踪
└── tests/
    ├── test_grid_ops.c      # 网格操作测试
    └── test_unicode.c       # Unicode处理测试
```

### 实现步骤

#### 周三任务
1. **上午 09:30-12:00**: 网格操作分析
   ```c
   // 识别所有网格修改点
   - grid_clear()
   - grid_clear_lines()
   - grid_move_lines()
   - grid_duplicate_lines()
   - grid_reflow()
   ```

2. **下午 14:00-17:00**: 批量API设计
   ```c
   typedef struct grid_batch_update {
       uint32_t start_row;
       uint32_t end_row;
       uint32_t start_col;
       uint32_t end_col;
       enum update_type type;
       void* data;
   } grid_batch_update_t;
   
   typedef struct grid_callbacks {
       void (*on_batch_update)(grid_batch_update_t* updates, size_t count);
       void (*on_clear)(u_int sx, u_int sy, u_int ex, u_int ey);
       void (*on_scroll)(int direction, u_int lines);
   } grid_callbacks_t;
   ```

#### 周四任务
1. **上午 09:30-12:00**: Dirty Region追踪
   ```c
   typedef struct dirty_region {
       u_int min_row, max_row;
       u_int min_col, max_col;
       bool needs_full_redraw;
   } dirty_region_t;
   ```

2. **下午 14:00-17:00**: UTF-8和宽字符处理
   - 确保多字节字符正确处理
   - 处理combining characters
   - emoji支持验证

### 验收标准
- [ ] 批量更新API完整
- [ ] Dirty region最小化
- [ ] UTF-8/宽字符100%正确
- [ ] 内存使用不增加>5%
- [ ] 性能提升>20%

### 性能目标
- 单次更新延迟: <10μs
- 批量更新(1000 cells): <100μs
- 内存占用: 不超过原始grid 110%

---

## 协助任务

### A-201: 协助T-201事件循环
**时间**: 周二下午2小时  
**内容**: Review event_loop设计，提供tmux内部视角

### A-203: 协助T-203布局管理
**时间**: 周四下午1小时  
**内容**: 解释grid与layout的交互关系

---

## 📅 每日计划

### 周一 (8/26)
- 09:00 - 周会参与
- 10:00 - 复习第一周backend_router代码
- 14:00 - 准备grid.c深度分析
- 16:00 - 整理网格操作列表
- 17:00 - 日报

### 周二 (8/27)
- 09:00 - 站会
- 09:30 - 继续grid.c分析
- 14:00 - 协助CORE-001 review T-201
- 16:00 - 设计grid_callbacks接口草案
- 17:00 - 同步进展

### 周三 (8/28)
- 09:00 - 站会
- 09:30 - 正式开始T-202
- 10:00 - 实现grid_callbacks.h
- 14:00 - 实现批量操作API
- 16:00 - 单元测试框架
- 17:00 - 与INTG-001同步接口

### 周四 (8/29)
- 09:00 - 站会
- 09:30 - 实现dirty region tracking
- 11:00 - UTF-8处理验证
- 14:00 - 性能测试
- 15:00 - 协助CORE-001 T-203
- 16:00 - 优化和调试
- 17:00 - 交付T-202

### 周五 (8/30)
- 09:00 - 站会
- 09:30 - Bug修复
- 11:00 - 集成测试支持
- 14:00 - 性能调优
- 16:00 - 文档完善
- 17:00 - Demo支持

## 🔄 协作接口

### 输入依赖
- CORE-001: event_loop_backend.h (周三需要)
- ARCH-001: 性能要求确认

### 输出交付
- INTG-001: grid_callbacks.h (周四17:00)
- INTG-002: 批量操作API (周四17:00)
- QA-002: 测试用例文档

### 关键交接点
| 时间 | 交接内容 | 交给 |
|------|----------|------|
| 周三17:00 | grid_callbacks初版 | INTG-001 |
| 周四12:00 | dirty_tracking.c | INTG-003 |
| 周四17:00 | 完整T-202 | QA-001 |

## 🎯 质量标准

### 代码质量
- [ ] 0编译警告
- [ ] 0内存泄漏(Valgrind验证)
- [ ] 代码覆盖率>80%
- [ ] 所有函数有注释

### 性能指标
- [ ] 批量操作比单个操作快10x
- [ ] 内存增长<10%
- [ ] CPU使用降低20%

## 🚨 风险项

| 风险 | 概率 | 缓解 |
|------|------|------|
| UTF-8处理复杂 | 高 | 提前研究，准备测试用例 |
| 性能不达标 | 中 | 预留周五优化时间 |
| 接口变更 | 低 | 尽早与INTG-001确认 |

## 📝 技术笔记区

### Grid内存布局
```c
struct grid {
    int sx, sy;           // size
    int hsize;            // history size  
    int hlimit;           // history limit
    struct grid_line *linedata;  // line array
};

struct grid_line {
    u_int cellsize;       // allocated cells
    u_int cellused;       // used cells
    struct grid_cell *celldata;  // cell array
    u_int flags;          // line flags
};
```

### 批量优化思路
1. 合并相邻的相同操作
2. 延迟实际内存操作
3. 使用脏标记避免重复更新
4. 预分配缓冲区

---

**角色**: CORE-002 (libtmux-core-developer)  
**更新日期**: 2025-08-25