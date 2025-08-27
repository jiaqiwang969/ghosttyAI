# CLAUDE.md - Ghostty × tmux 深度集成项目

## 🎯 项目目标

将 tmux 的**会话编排能力**嵌入到 Ghostty 中，实现可编程的终端会话控制，同时保持 Ghostty 的现代 GPU 渲染界面。

### 核心需求
**不是**简单地在 Ghostty 中运行 tmux 命令（那样会让 tmux 接管整个 UI）  
**而是**将 tmux 编译为库（libtmuxcore），深度集成其会话管理逻辑，UI 完全由 Ghostty 控制

## 📌 你真正想要的 tmux 功能

基于你的实际使用场景（自动化脚本、Claude Agent 编排），tmux 的核心价值在于：

### 1. **可编程的会话控制 API**
```bash
# 你的典型使用场景
tmux send-keys -t "session:window.pane" "command" Enter  # 发送命令到特定面板
tmux capture-pane -t "session:0" -p                      # 捕获面板输出
tmux list-sessions                                       # 查询会话状态
```

### 2. **会话持久性**
- 断开连接后会话继续运行
- 重新连接后恢复工作状态
- 多个独立的工作环境

### 3. **面板/窗口管理**
- 智能分屏布局算法
- 独立的 PTY 进程管理
- 窗口/面板的灵活切换

## 🏗️ 技术架构

```
┌─────────────────────────────────────────────────┐
│           Ghostty Application (Zig)              │
│                                                  │
│  ┌────────────────────────────────────────┐    │
│  │     Ghostty UI Layer (GPU Rendering)    │    │
│  │  • 窗口管理 • 字体渲染 • 颜色主题        │    │
│  │  • 所有可见的 UI 元素                    │    │
│  └────────────────┬───────────────────────┘    │
│                   │ 回调接口                    │
│  ┌────────────────▼───────────────────────┐    │
│  │        libtmuxcore (C Library)          │    │
│  │  • 会话/窗口/面板管理逻辑                │    │
│  │  • send-keys, capture-pane API         │    │
│  │  • 布局算法 • PTY 管理                  │    │
│  │  • 不包含任何 TTY 输出代码               │    │
│  └────────────────────────────────────────┘    │
└─────────────────────────────────────────────────┘
```

## 📁 项目结构

```
/Users/jqwang/98-ghosttyAI/
├── ghostty/                # Ghostty 源代码（Zig）
├── tmux/                    # tmux 源代码（C）
├── tmux-only/               # 提取的 tmux 独有功能
│   ├── session/            # 会话管理
│   ├── window/             # 窗口管理
│   ├── pane/               # 面板管理
│   ├── layout/             # 布局算法
│   ├── control/            # 控制 API (send-keys, capture-pane)
│   └── support/            # 支持功能
└── docs/                    # 文档

```

## 🔑 关键实现点

### 1. 提取 tmux 核心功能
从 tmux 源代码中提取：
- ✅ 会话/窗口/面板管理（session.c, window.c）
- ✅ 控制命令（cmd-send-keys.c, cmd-capture-pane.c）
- ✅ 布局算法（layout.c）
- ❌ 排除：TTY 输出（tty.c）、客户端-服务器架构（client.c, server.c）

### 2. 创建回调接口
将 tmux 的所有输出改为回调：
```c
// 不再直接输出到终端
// tty_write(...) → ui_backend_vtable->on_grid_update(...)
```

### 3. Ghostty 集成
在 Ghostty 中实现：
- 多面板的 Terminal 实例管理
- tmux 命令的 FFI 桥接
- 会话寻址系统（"session:window.pane"格式）

## 🚀 预期成果

### 用户体验
```bash
# 在 Ghostty 中直接使用你现有的脚本
./schedule_with_note.sh 3 "Check progress" "ghostty:0"
./send-claude-message.sh "ghostty:agent-1" "Start task"

# Ghostty 提供 tmux 兼容的命令
ghostty send-keys -t "dev:0" "npm test" Enter
ghostty capture-pane -t "dev:1" | grep "PASS"
```

### 技术优势
- **保留 Ghostty 的所有 UI 优势**：GPU 渲染、平滑滚动、现代界面
- **获得 tmux 的编排能力**：会话管理、命令注入、输出捕获
- **真正的代码级集成**：不是运行 tmux 进程，而是库级别的嵌入

## ⚡ 实施步骤

### Phase 1: 核心提取（当前阶段）
- [x] 分析 tmux 源代码结构
- [x] 识别需要的核心模块
- [x] 创建 tmux-only 目录
- [ ] 编译 libtmuxcore.dylib

### Phase 2: 接口设计
- [ ] 定义 C-to-Zig FFI 接口
- [ ] 创建回调系统
- [ ] 实现 UI 后端适配器

### Phase 3: Ghostty 集成
- [ ] 实现会话管理器
- [ ] 添加命令处理器
- [ ] 创建兼容层

### Phase 4: 测试验证
- [ ] 基础功能测试
- [ ] 性能验证
- [ ] 脚本兼容性测试

## 📝 重要约束

1. **保持 tmux 语义**：所有 tmux 命令行为保持一致
2. **零 TTY 输出**：tmux 不能直接输出任何终端序列
3. **性能要求**：不能降低 Ghostty 的渲染性能
4. **向后兼容**：支持现有的 tmux 自动化脚本

## 🎨 最终效果

你将获得一个强大的终端环境：
- **外观**：完全是 Ghostty 的现代 UI
- **内核**：拥有 tmux 的会话编排能力
- **使用**：支持你所有的自动化脚本
- **性能**：GPU 加速渲染 + 高效会话管理

---

**项目哲学**：tmux 的大脑 + Ghostty 的外表 = 完美的终端体验