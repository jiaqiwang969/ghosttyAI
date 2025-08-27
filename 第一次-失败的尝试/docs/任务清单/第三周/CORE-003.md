# CORE-003 任务清单 - 生产化部署

**角色代码**: CORE-003  
**负责人**: libtmux-core-developer, c-tmux-specialist  
**任务数量**: 2个  
**重要性**: 关键路径

---

## T-501: 动态库打包与符号导出

### 目标
将libtmuxcore从静态库转换为可分发的动态库，实现符号版本控制。

### 输入
- cache/week2/所有已完成的源代码
- project_spec.md中的ABI要求

### 交付物
1. **libtmuxcore.so.1.0.0** (Linux)
2. **libtmuxcore.1.dylib** (macOS)  
3. **libtmuxcore.sym** - 符号版本脚本
4. **libtmuxcore.pc** - pkg-config文件
5. **install.sh** - 安装脚本

### 技术要求
```bash
# 符号版本控制
LIBTMUXCORE_1.0 {
    global:
        tmc_init;
        tmc_create_session;
        tmc_loop_vtable_*;
        event_loop_*;
        grid_callbacks_*;
    local: *;
};
```

### 验收标准
- [ ] 动态库可加载
- [ ] 符号正确导出
- [ ] pkg-config可用
- [ ] 向后兼容性保证

---

## T-502: 传统tmux兼容层

### 目标
保持与传统tmux的100%兼容性，支持渐进式迁移。

### 输入
- 原始tmux代码
- 第二周的vtable抽象

### 交付物
1. **legacy_tty_backend.c** - 传统TTY后端
2. **mode_switch.c** - 运行时模式切换
3. **compat_api.h** - 兼容性API层
4. **migration_guide.md** - 迁移指南

### 技术实现
```c
// 运行时后端切换
typedef enum {
    BACKEND_LEGACY_TTY,
    BACKEND_GHOSTTY,
    BACKEND_GENERIC
} backend_mode_t;

int tmc_set_backend(backend_mode_t mode);
```

### 验收标准
- [ ] 所有tmux命令兼容
- [ ] 配置文件100%兼容
- [ ] 平滑切换机制
- [ ] 性能无退化

---

## 执行计划

### 周一
- 上午: 搭建动态库构建系统
- 下午: 实现符号版本控制

### 周二  
- 上午: 完成pkg-config和安装脚本
- 下午: 实现兼容层基础架构

### 周三
- 测试和优化
- 文档编写

---

## 风险点
1. ABI稳定性维护
2. 符号冲突处理
3. 多平台兼容性

---

## 成功指标
- 可通过标准方式安装和链接
- 零破坏性变更
- 为包管理器集成做好准备