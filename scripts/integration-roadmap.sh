#!/bin/bash
# Ghostty终端通信功能集成路线图
# 用于指导ghostty-terminal-comm-integrator agent的具体工作

echo "================================================"
echo "  Ghostty Terminal Communication Integration"
echo "           Roadmap & Execution Plan"
echo "================================================"
echo ""

# 前置检查
echo "📋 Phase 0: Pre-Integration Checks (30 min)"
echo "------------------------------------------------"
cat << 'EOF'
# 1. 确认Ghostty编译环境
cd /Users/jqwang/98-ghosttyAI/ghostty
zig build

# 2. 运行现有测试
zig build test

# 3. 创建工作分支
git checkout -b feature/terminal-communication

# 4. 确认关键文件存在
ls -la src/App.zig
ls -la src/Surface.zig
ls -la src/apprt/ipc.zig
ls -la src/termio/Termio.zig

EOF

# Phase 1: 核心集成
echo "🔧 Phase 1: Core Integration (Day 1)"
echo "------------------------------------------------"
cat << 'EOF'
Task 1.1: 准备SessionManager (1小时)
  □ 将SessionManager.zig复制到src/terminal/
  □ 修复任何import路径问题
  □ 确保独立编译通过
  
Task 1.2: 集成到App.zig (2小时)
  □ 添加import语句
  □ 添加session_manager字段
  □ 在init中初始化
  □ 在deinit中清理
  □ 编译测试
  
Task 1.3: 扩展Surface (2小时)
  □ 添加session_id字段
  □ 在init中生成ID
  □ 注册到SessionManager
  □ 在deinit中注销
  □ 编译测试

验证检查点:
  ✓ zig build通过
  ✓ 现有功能未受影响
  ✓ 可以启动Ghostty

EOF

# Phase 2: IPC集成
echo "🔌 Phase 2: IPC Integration (Day 2)"
echo "------------------------------------------------"
cat << 'EOF'
Task 2.1: 扩展IPC Actions (1小时)
  □ 在apprt/ipc.zig添加SendToSession
  □ 定义消息结构
  □ 更新Key枚举
  □ 编译测试
  
Task 2.2: 实现IPC处理 (2小时)
  □ 在App.zig添加处理逻辑
  □ 路由到SessionManager
  □ 错误处理
  □ 日志记录
  
Task 2.3: D-Bus集成 (2小时)
  □ 更新gtk/ipc处理
  □ 添加新的D-Bus方法
  □ 测试IPC调用

验证检查点:
  ✓ IPC消息可以发送
  ✓ SessionManager收到消息
  ✓ 基本路由工作

EOF

# Phase 3: 命令实现
echo "🎮 Phase 3: Command Implementation (Day 3)"
echo "------------------------------------------------"
cat << 'EOF'
Task 3.1: 内部命令解析 (2小时)
  □ 在Terminal添加@send命令识别
  □ 在Terminal添加@link命令识别
  □ 在Terminal添加@sessions命令识别
  □ 命令参数解析
  
Task 3.2: 命令执行 (2小时)
  □ 实现send功能
  □ 实现link功能
  □ 实现list功能
  □ 错误反馈
  
Task 3.3: 外部CLI (1小时)
  □ 在cli.zig添加子命令
  □ 实现ghostty send
  □ 实现ghostty link
  □ 帮助文档

验证检查点:
  ✓ @send命令工作
  ✓ 消息成功路由
  ✓ 终端B收到命令

EOF

# Phase 4: SSH支持
echo "🌐 Phase 4: SSH Remote Support (Day 4)"
echo "------------------------------------------------"
cat << 'EOF'
Task 4.1: OSC 777协议 (2小时)
  □ 在osc.zig添加777处理
  □ 定义通信协议格式
  □ Base64编解码
  □ 测试OSC序列
  
Task 4.2: Shell Integration (2小时)
  □ 修改shell-integration脚本
  □ 添加GHOSTTY_SESSION_ID环境变量
  □ SSH自动传递
  □ 远程注册逻辑
  
Task 4.3: 远程通信测试 (1小时)
  □ 本地SSH测试
  □ 实际远程测试
  □ 错误处理
  □ 性能测试

验证检查点:
  ✓ OSC序列正确解析
  ✓ 远程会话可注册
  ✓ 双向通信工作

EOF

# Phase 5: 测试与完善
echo "✅ Phase 5: Testing & Polish (Day 5)"
echo "------------------------------------------------"
cat << 'EOF'
Task 5.1: 单元测试 (2小时)
  □ SessionManager测试
  □ IPC测试
  □ 命令解析测试
  □ 集成测试
  
Task 5.2: 手动测试场景 (2小时)
  □ 本地双终端通信
  □ 多终端链接
  □ SSH远程通信
  □ 错误恢复
  
Task 5.3: 文档更新 (1小时)
  □ 用户使用文档
  □ 配置选项说明
  □ 示例脚本
  □ README更新

最终验证:
  ✓ 所有测试通过
  ✓ 无性能退化
  ✓ 文档完整
  ✓ 代码审查通过

EOF

# 关键文件修改清单
echo ""
echo "📁 Key Files to Modify"
echo "------------------------------------------------"
cat << 'EOF'
必须修改 (5个文件):
  1. src/App.zig            (+30 lines)
  2. src/Surface.zig        (+20 lines)
  3. src/apprt/ipc.zig      (+15 lines)
  4. src/terminal/Terminal.zig (+40 lines)
  5. src/terminal/osc.zig   (+30 lines)

新增文件 (1个):
  1. src/terminal/SessionManager.zig (300 lines)

可选修改 (3个文件):
  1. src/cli.zig            (+50 lines)
  2. shell-integration/zsh/ghostty-integration (+10 lines)
  3. src/config/Config.zig  (+5 lines)

总计: 6个必要文件修改, 约500行代码变更

EOF

# 测试命令
echo ""
echo "🧪 Test Commands"
echo "------------------------------------------------"
cat << 'EOF'
# 编译测试
zig build

# 运行单元测试
zig build test

# 运行特定测试
zig test src/terminal/SessionManager.zig

# 手动测试
./zig-out/bin/ghostty --session-id test-a &
./zig-out/bin/ghostty --session-id test-b &

# 在test-a中执行
@send test-b "echo 'Hello from A'"
@link test-b
@sessions

# 清理
killall ghostty

EOF

# Git提交策略
echo ""
echo "📝 Git Commit Strategy"
echo "------------------------------------------------"
cat << 'EOF'
# 每完成一个Phase提交一次
git add -A
git commit -m "feat(terminal-comm): Phase 1 - Core SessionManager integration"

git add -A  
git commit -m "feat(terminal-comm): Phase 2 - IPC Action extensions"

git add -A
git commit -m "feat(terminal-comm): Phase 3 - Command implementation"

git add -A
git commit -m "feat(terminal-comm): Phase 4 - SSH remote support"

git add -A
git commit -m "feat(terminal-comm): Phase 5 - Tests and documentation"

# 最终squash（可选）
git rebase -i HEAD~5
# 合并为一个commit: "feat: Add terminal-to-terminal communication"

EOF

# 成功标准
echo ""
echo "🎯 Success Criteria"
echo "------------------------------------------------"
cat << 'EOF'
功能要求:
  ✓ 终端A可以发送命令到终端B
  ✓ 支持双向通信
  ✓ SSH远程场景工作
  ✓ 命令简洁易用

技术要求:
  ✓ 代码修改 < 10个文件
  ✓ 总代码量 < 500行
  ✓ 无破坏性变更
  ✓ 测试覆盖 > 80%
  ✓ 性能影响 < 1%

用户体验:
  ✓ @send命令响应 < 50ms
  ✓ 错误信息清晰
  ✓ 帮助文档完整

EOF

echo ""
echo "================================================"
echo "         Ready to Start Integration!"
echo "================================================"
echo ""
echo "Next Step: Run Phase 0 checks, then begin Task 1.1"