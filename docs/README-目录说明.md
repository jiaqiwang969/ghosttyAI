# 📚 Docs 目录结构说明 - Ghostty × tmux Integration Project

## 目录概览

docs 目录包含项目的所有架构设计、任务分配、管理视图等关键文档，分为4个主要子目录，每个服务于不同的目的和受众。

---

## 📁 /docs/任务清单/第一周/

**目的**: Week 1 具体任务分配和执行计划  
**受众**: 所有工程师和项目经理  
**更新频率**: 每日更新进度

### 核心文件说明

#### 1. **协作计划.md**
- **作用**: Week 1 整体协作框架和时间表
- **内容**: 
  - 每日交接点 (Daily Handoff Points)
  - 关键依赖关系甘特图
  - 沟通协议和升级机制
  - 风险监控指标
  - 质量检查点
- **使用者**: Project Manager 每日参考

#### 2. **ARCH-001-架构师.md**
- **作用**: 架构师的 Week 1 任务清单
- **内容**:
  - UI Backend 接口设计 (2天)
  - 设计评审和验证
  - 输出: ui_backend.h, backend_vtable_spec.md
- **状态**: 独立顾问角色

#### 3. **CORE-001-高级C开发.md** & **CORE-002-C开发.md**
- **作用**: C语言核心开发任务
- **CORE-001 重点**:
  - T-101: 提取 tty_write hooks
  - T-102: 识别所有 tty_cmd_* 函数
  - 输出: Hook 清单和接口定义
- **CORE-002 重点**:
  - T-103: 实现 backend router
  - T-104: 保持 TTY 兼容性
  - 输出: backend_router.c

#### 4. **INTG-001-高级Zig开发.md**
- **作用**: Zig FFI 集成任务
- **内容**:
  - T-201: Ghostty backend 实现
  - T-202: FFI bridge 开发
  - 输出: backend_ghostty.c, ffi_bridge.zig

#### 5. **QA-001-测试主管.md** & **QA-002-测试工程师.md**
- **作用**: 测试框架和验证任务
- **QA-001**: 测试策略、框架搭建
- **QA-002**: Grid 操作测试、性能验证
- **覆盖率目标**: >65%

#### 6. **OPS-001-DevOps工程师.md**
- **作用**: 构建系统和 CI/CD 任务
- **内容**:
  - Makefile 完善
  - ARM64 优化
  - PlantUML 验证
- **状态**: 独立顾问角色

---

## 📁 /docs/architecture-view/

**目的**: 技术架构设计文档（工程师视角）  
**受众**: 开发团队、架构师  
**更新频率**: 设计阶段更新

### 核心文件说明

#### PlantUML 图表 (.puml)
1. **high-level-integration.puml** - 系统整体架构图
2. **component-interaction.puml** - 组件交互关系
3. **data-flow.puml** - 数据流向图
4. **callback-sequences.puml** - 回调时序图
5. **event-loop-integration.puml** - 事件循环集成
6. **session-lifecycle.puml** - 会话生命周期
7. **class-diagram.puml** - 类结构图
8. **deployment-diagram.puml** - 部署架构

#### Markdown 文档 (.md)
1. **integration-architecture.md** - 集成架构总体设计
2. **tty-write-interception.md** - TTY 写入拦截机制
3. **abi-stability.md** - C ABI 稳定性策略
4. **frame-batching.md** - 帧批处理优化
5. **backpressure-control.md** - 背压控制机制
6. **implementation-roadmap.md** - 实施路线图

**特点**: 深度技术细节，包含代码示例和性能分析

---

## 📁 /docs/new-architecture-施工图/

**目的**: 施工蓝图视角的架构文档（如何构建）  
**受众**: 实施团队、项目经理  
**更新频率**: 每个 Sprint 开始时

### 核心文件说明

#### PlantUML 施工图 (.puml)
1. **task-driven-architecture.puml** - 任务驱动的架构分解
2. **component-ownership-matrix.puml** - 组件负责人矩阵
3. **implementation-flow.puml** - 实施流程图
4. **data-flow-with-tasks.puml** - 带任务标注的数据流
5. **construction-timeline.puml** - 施工时间线

#### 实施指南 (.md)
1. **technical-implementation-guide.md** - 技术实施指南
   - Week-by-week 实施计划
   - 具体代码修改位置
   - 测试验证步骤

**特点**: 强调 "HOW TO BUILD"，包含具体步骤和验收标准

---

## 📁 /docs/project-manager-view/

**目的**: 项目管理视角的文档（谁负责什么）  
**受众**: Project Manager, Team Leads  
**更新频率**: 每日更新任务状态

### 核心文件说明

#### 任务管理图表 (.puml)
1. **task-ownership-week1.puml** - Week 1 任务卡片和负责人
   - 使用颜色编码状态: Todo(红), Doing(黄), Done(绿), Blocked(橙)
   - 每个任务包含: ID, Owner, Est, Inputs, Outputs, AC

2. **component-ownership.puml** - 组件级别的负责人分配
   - 映射每个系统组件到具体工程师

3. **role-swimlanes.puml** - 角色泳道图
   - 显示不同角色的并行工作流

4. **ownership-gantt.puml** - 带负责人的甘特图
   - 时间线 + 任务 + 负责人三维视图

5. **task-complete-ownership.puml** - 完整任务依赖图
   - 显示任务间依赖和关键路径

#### 管理文档 (.md)
1. **team-structure.md** - 团队组织结构
   - RACI 矩阵 (Responsible, Accountable, Consulted, Informed)
   - 11 个角色的职责定义

2. **task-assignment-matrix.md** - 任务分配矩阵
   - 任务 ID → 负责人 → 交付物 → 截止日期

**特点**: 强调 "WHO DOES WHAT"，便于追踪和问责

---

## 🔄 文档间的关系

```
任务清单 (WHAT)
    ↓
    ├→ architecture-view (WHY - 技术原理)
    ├→ new-architecture-施工图 (HOW - 实施步骤)
    └→ project-manager-view (WHO - 责任分配)
```

## 📝 使用指南

### 对于工程师
1. 早上查看 `/任务清单/第一周/[YOUR-ROLE].md` 获取今日任务
2. 参考 `/architecture-view/` 理解技术设计
3. 遵循 `/new-architecture-施工图/` 的实施步骤
4. 每日 17:00 前更新进度到 `cache/week1/[ROLE]/daily-reports/`

### 对于项目经理
1. 每日查看 `/project-manager-view/` 跟踪进度
2. 使用 `/任务清单/第一周/协作计划.md` 协调交接
3. 更新 PlantUML 图表中的任务状态
4. 监控关键路径和阻塞问题

### 对于架构师
1. 维护 `/architecture-view/` 的技术准确性
2. 评审 `/new-architecture-施工图/` 的可行性
3. 咨询时参考具体设计文档

### 对于 DevOps
1. 根据 `/new-architecture-施工图/` 准备构建环境
2. 验证 PlantUML 图表的正确性
3. 确保 CI/CD 支持所有组件

---

## 🎯 关键要点

1. **任务清单** = 执行层面的日常指导
2. **architecture-view** = 深度技术理解
3. **施工图** = 分步实施计划
4. **project-manager-view** = 管理和追踪

每个目录服务不同目的，但共同支撑项目成功交付。所有文档相互关联，形成完整的项目文档体系。

---

*最后更新: 2025-01-06*  
*维护者: Project Manager*