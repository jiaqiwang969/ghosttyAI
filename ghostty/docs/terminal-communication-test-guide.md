# Ghostty终端通信功能测试指南

## 🎯 测试目标
验证Ghostty是否能实现终端A向终端B发送命令的功能（类似tmux）

## 📍 当前问题
- **症状**：输入`@session`或`@send`时显示`zsh: command not found`
- **原因**：命令被发送给shell而不是被Ghostty内部处理
- **解决方案**：需要在输入层拦截这些内部命令

## ✅ 测试方法1：基础功能验证

### 步骤1：编译并启动
```bash
# 使用新的Makefile编译（自动修复Sparkle签名问题）
make build-ghostty

# 启动Ghostty
make run
```

### 步骤2：测试内部命令识别
在Ghostty终端中输入：
```bash
@session
```

**期望结果**：
- ✅ 显示当前终端的session ID，如：`Session ID: surface-1054d6000`
- ❌ 如果显示`zsh: command not found: @session`，说明命令未被拦截

### 步骤3：测试终端间通信
1. 打开第二个终端标签页（Cmd+T）
2. 在第二个终端输入`@session`获取其ID
3. 在第一个终端输入：
```bash
@send surface-<第二个终端的ID> echo "Hello from Terminal 1"
```

**期望结果**：
- ✅ 第二个终端执行命令并显示：`Hello from Terminal 1`
- ❌ 如果显示`zsh: command not found: @send`，说明命令未被拦截

## ✅ 测试方法2：使用测试脚本

```bash
# 运行自动化测试
make test-comm
```

该命令会：
1. 自动编译Ghostty
2. 启动测试脚本
3. 显示详细的测试说明

## 🔧 调试方法

### 启用日志查看内部处理
```bash
# 编译调试版本
make build-ghostty-debug

# 带调试日志启动
RUST_LOG=debug zig-out/Ghostty.app/Contents/MacOS/Ghostty 2>&1 | grep -E "@send|@session|Intercepted"
```

### 检查消息路由
在日志中查找：
- `Intercepted @send command` - 命令被成功拦截
- `Routing to session` - 命令被路由到目标终端
- `Session not found` - 目标终端不存在

## 🚨 已知问题及解决状态

| 问题 | 状态 | 解决方案 |
|------|------|----------|
| Sparkle框架签名错误 | ✅ 已修复 | `make build-ghostty`自动处理 |
| @session命令未被识别 | ⚠️ 待修复 | 需要在输入层添加拦截 |
| @send命令未被识别 | ⚠️ 待修复 | 需要在输入层添加拦截 |

## 📊 测试检查清单

- [ ] Ghostty成功编译
- [ ] Ghostty能正常启动
- [ ] 输入普通命令（如ls）正常工作
- [ ] @session命令不会报`command not found`
- [ ] @session显示当前终端ID
- [ ] @send命令不会报`command not found`
- [ ] @send能发送命令到其他终端
- [ ] 多个终端之间能互相通信

## 💡 设计原理

根据`CLAUDE.md`的设计目标，我们要实现的是：
1. **最小化修改**：只修改必要的文件（<10个文件）
2. **内部命令**：@send和@session是Ghostty内部命令，不应到达shell
3. **消息路由**：通过App层的SessionManager路由消息到目标Surface

当前的实现架构：
```
用户输入 → Surface.keyCallback → 命令检测 → 
  ├─ 普通输入 → 发送到PTY → Shell
  └─ @命令 → SessionManager处理 → 路由到目标终端
```

## 🔍 下一步

如果测试失败（命令未被识别），需要：
1. 在Surface.zig的keyCallback中添加输入缓冲
2. 检测完整的@send或@session命令
3. 拦截并处理，不发送给PTY

这样能确保内部命令在Ghostty内部处理，而不会到达shell。