# Ghostty Terminal Communication Project - CLAUDE.md

## 🎯 项目使命
在Ghostty中实现类似tmux的终端间通信能力，允许终端A向终端B发送命令并接收响应，包括SSH远程场景。

## 📖 项目背景

### 前两次失败的教训
1. **第一次失败**：试图提取tmux为libtmuxcore，创建175个文件，9+个agents，过度组织化
2. **第二次失败**：构建916KB的libtmuxcore.dylib，导出999个符号，过度工程化

### 第三次成功的关键
1. **源码验证架构**：通过阅读实际代码理解tmux和Ghostty的架构
2. **找准层次对应**：tmux server ↔ Ghostty App层的准确映射
3. **最小化实现**：聚焦核心需求 - 终端间发送命令

## 🏗️ 核心架构理解

### tmux架构（已验证）
```
tmux client → tmux server → session → window → pane → PTY
                  ↑
            核心路由层
    (bufferevent_write直接写入PTY)
```

### Ghostty架构（已验证）
```
IPC → App → Surface → Termio → Terminal → PTY
       ↑
   对应tmux server层
   (管理所有Surface实例)
```

### 关键代码路径
- **tmux**: `cmd-send-keys.c` → `window_pane_key()` → `input_key_pane()` → `bufferevent_write()`
- **Ghostty**: `App.zig` → `Surface.zig` → `Termio.zig` → `backend.write()`

## 🎯 最小化实现方案

### Phase 1: 核心功能（MVP）
1. 在App层添加SessionManager
2. 扩展IPC支持send_to_session
3. Surface添加session_id
4. 实现基本的消息路由

### Phase 2: SSH支持
1. 利用现有shell integration
2. 通过OSC 777建立通信通道
3. 环境变量传递session信息

### Phase 3: 用户体验
1. 实现@send, @link命令
2. 添加会话管理命令
3. 可视化状态显示

## ⚙️ 技术实现细节

### 关键文件修改
```
src/App.zig                    # 添加SessionManager
src/Surface.zig                # 添加session_id字段
src/apprt/ipc.zig             # 扩展Action枚举
src/terminal/SessionManager.zig # 新增核心管理器
src/termio/Termio.zig         # 消息拦截点
```

### 通信协议
- **本地**: D-Bus IPC + 直接PTY写入
- **远程**: OSC 777序列 + Shell Integration

### 命令设计
```bash
# 内部命令（终端内）
@send <session-id> <command>
@link <session-id>
@sessions

# 外部命令（命令行）
ghostty send <session-id> <command>
ghostty link <session-id>
```

## 📋 代码集成检查清单

### 必须完成
- [ ] App.zig集成SessionManager
- [ ] IPC Action扩展
- [ ] Surface session_id实现
- [ ] 基本send功能
- [ ] 单元测试

### 应该完成
- [ ] Shell integration集成
- [ ] OSC 777协议
- [ ] 会话持久化
- [ ] 错误处理

### 可以完成
- [ ] GUI管理界面
- [ ] 高级过滤器
- [ ] 性能监控

## 🚫 避免的陷阱

1. **不要**试图提取tmux代码
2. **不要**创建独立的库
3. **不要**过度抽象和设计
4. **不要**偏离核心需求
5. **不要**引入不必要的依赖

## 📊 成功标准

### 功能标准
- ✅ 终端A能发送命令到终端B
- ✅ 支持双向通信
- ✅ SSH远程场景可用
- ✅ 命令简洁直观

### 技术标准
- ✅ 代码修改最小化（<10个文件）
- ✅ 不影响现有功能
- ✅ 性能开销可忽略
- ✅ 易于测试和调试

## 🔧 开发原则

1. **KISS原则**：保持简单直接
2. **最小侵入**：尽量少修改现有代码
3. **增量开发**：先实现核心，再添加功能
4. **代码优先**：用代码验证想法
5. **用户体验**：命令要直观易用

## 📝 项目状态

### 已完成
- ✅ 架构分析和验证
- ✅ 实现方案设计
- ✅ SessionManager原型
- ✅ 演示脚本
- ✅ Phase 1: 基础终端间通信功能（2024-12）
- ✅ Phase 2: 会话管理命令扩展（2024-12-28）
- ✅ **关键修复**: @ghostty send命令执行问题（2024-12-29）

### 重要修复记录（2024-12-29）

#### @ghostty send命令执行修复
**问题描述**：通过`@ghostty send`发送的命令在接收终端仅显示为文本，而不执行命令。

**根本原因**：
1. 消息直接通过`termio.Message.writeReq`写入PTY
2. 绕过了Surface层的`@ghostty`命令检测机制
3. 键盘事件处理顺序问题：keybinding handler在@检测之前消费了事件

**解决方案**：
1. 在Surface.zig添加`processReceivedMessage`公共方法
2. 模拟完整的键盘输入流程，每个字符都通过`keyCallback`处理
3. 调整事件处理顺序，将@检测移到keybinding handler之前（行2229）
4. SessionManager使用新方法确保命令正确触发

**代码变更**：
- `Surface.zig`: 添加processReceivedMessage方法，实现键盘输入模拟
- `SessionManager.zig`: 修改deliverMessage使用Surface的新方法
- 修复了`self.renderer_state.terminal`应为`self.io.terminal`的引用问题

**影响**：
- `@ghostty send agent "@ghostty help"`现在会正确执行help命令
- 所有通过send发送的@ghostty命令都能正确触发和执行
- 保持了命令处理的一致性

### 进行中
- 🔄 Phase 3: SessionCore架构升级
- 🔄 Terminal内容提取功能

### 待开始
- ⏳ 内容同步实现
- ⏳ attach/detach命令
- ⏳ 完整测试套件

## 🤖 AI助手指导

### 对话原则
1. 始终基于源码讨论，不要想象
2. 优先考虑最小化实现
3. 每个决策都要有代码支撑
4. 保持聚焦在核心需求

### 代码原则
1. 修改前先读取现有代码
2. 每次修改控制在最小范围
3. 保持代码风格一致
4. 添加必要的注释

### 测试原则
1. 先写测试，后写代码
2. 测试覆盖核心路径
3. 包含错误场景测试

## 📚 参考资源

### 关键文档
- `docs/tmux-ghostty-架构对比文档.md`
- `docs/ghostty-terminal-communication-design.md`
- `scripts/demo-ghostty-terminal-comm.sh`

### 核心代码
- `src/terminal/SessionManager.zig`
- `src/terminal/demo_terminal_communication.zig`

---

**记住**：成功的关键是**简单、直接、最小化**。不要重复前两次的错误！