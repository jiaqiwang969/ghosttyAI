# Ghostty Terminal Session 项目

## 🎯 项目目标
在 Ghostty 终端中实现类似 tmux 的会话管理功能，让用户可以：
- 在不同终端窗口之间切换会话（attach/detach）
- 保持会话持久化（关闭窗口不丢失会话）
- 多个窗口查看同一个会话

## 📊 架构参考（必读）

**⚠️ 重要：实施前必须理解以下架构图**

关键架构图位于 `/Users/jqwang/98-ghosttyAI/diagrams/`：

| 文件名 | 说明 | 重要性 |
|--------|------|--------|
| `tmux-attach-architecture.puml` | tmux 的 Session-Client 分离架构，理解正确的设计模式 | ⭐⭐⭐ |
| `ghostty-tmux-gap-analysis.puml` | 分析当前架构为何 attach 失效，理解问题根源 | ⭐⭐⭐ |
| `new-sessioncore-architecture.puml` | 目标架构设计，明确要实现的结构 | ⭐⭐⭐ |
| `implementation-roadmap.puml` | Phase 4 实施步骤，按步骤执行 | ⭐⭐⭐ |


### 核心洞察（从架构图中学到的）
- **tmux 模式**：Session 拥有一切（Terminal、PTY、进程），Client 只是查看器
- **当前问题**：Ghostty 架构颠倒 - Surface 拥有 Terminal（错误）
- **解决方案**：SessionCore 必须拥有 Terminal，Surface 只能查看

## 📌 当前任务：修复 attach 功能

### 问题诊断（参考 ghostty-tmux-gap-analysis.puml）
当前 `@ghostty attach session-name` 命令不工作，原因是架构设计错误：
- ❌ **现状**：Surface 拥有 Terminal，SessionCore 只有引用
- ✅ **目标**：SessionCore 拥有 Terminal，Surface 只是查看器

### 期望效果
```bash
# Terminal A
@ghostty session alpha
echo "This is session alpha"

# Terminal B  
@ghostty session beta
echo "This is session beta"

# Terminal A 执行
@ghostty attach beta

# 结果：Terminal A 立即显示 "This is session beta"
```

## 🏗️ 实施计划（基于 implementation-roadmap.puml）

### Phase 4.1：SessionCore 拥有 Terminal
修改 `src/terminal/SessionCore.zig`：
```zig
pub const SessionCore = struct {
    owned_terminal: Terminal,  // 不再是引用
    owned_pty: PTY,           // 不再是引用
    shell_process: Process,   // 管理 shell 进程
};
```

### Phase 4.2：Surface 变为查看器
修改 `src/Surface.zig`：
```zig
pub const Surface = struct {
    session_core: *SessionCore,  // 必需，指向查看目标
    // 移除 io: Termio（不再拥有 Terminal）
};
```

### Phase 4.3：实现 attach 切换
```zig
pub fn attachToSession(surface: *Surface, session_core: *SessionCore) {
    surface.session_core = session_core;      // 切换指针
    renderer.switchTerminal(session_core.terminal);  // 更新渲染
    renderer.forceFullRedraw();               // 立即重绘
}
```

## 🔧 如何编译运行

### 构建项目
```bash
cd /Users/jqwang/98-ghosttyAI/ghostty
make run
```

### 调试模式
```bash
cd /Users/jqwang/98-ghosttyAI/ghostty
GHOSTTY_LOG=debug make run 2>debug.log
```

### 测试 attach 功能
```bash
# 运行测试脚本
bash test-phase4-switching.sh

# 或手动测试
# 1. 打开两个 Ghostty 终端
# 2. Terminal 1: @ghostty session alpha
# 3. Terminal 2: @ghostty session beta  
# 4. Terminal 1: @ghostty attach beta
# 5. 验证 Terminal 1 是否切换到 beta 的内容
```

## ✅ 成功标准
- `@ghostty attach` 命令能瞬间切换会话
- 关闭窗口不影响会话持续运行
- 多个窗口可以同时查看同一会话

---

**核心理念**：像 tmux 一样，Session 拥有一切，Surface 只是查看器。

**记住**：实施前务必查看架构图 → `open /Users/jqwang/98-ghosttyAI/diagrams/index.html`