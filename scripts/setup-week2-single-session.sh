#!/bin/bash
# 第二周单一Session设置脚本 - week-2

PROJECT_PATH="/Users/jqwang/98-ghosttyAI"

echo "=== 创建第二周 week-2 Session ==="

# Kill existing week-2 session if exists
tmux kill-session -t week-2 2>/dev/null

# Create week-2 session with all agents as windows
tmux new-session -d -s week-2 -c "$PROJECT_PATH"

# Window 0: Project Manager
tmux rename-window -t week-2:0 "tmux-project-manager"

# Window 1: System Architect
tmux new-window -t week-2 -n "system-architect" -c "$PROJECT_PATH"

# Window 2: C/tmux Specialist (CORE-001)
tmux new-window -t week-2 -n "c-tmux-specialist" -c "$PROJECT_PATH"

# Window 3: Libtmux Core Developer (CORE-002)
tmux new-window -t week-2 -n "libtmux-core-developer" -c "$PROJECT_PATH"

# Window 4: Zig-Ghostty Integration (INTG-001)
tmux new-window -t week-2 -n "zig-ghostty-integration" -c "$PROJECT_PATH"

# Window 5: Integration Dev (INTG-002)
tmux new-window -t week-2 -n "integration-dev" -c "$PROJECT_PATH"

# Window 6: Performance Engineer (INTG-003)
tmux new-window -t week-2 -n "performance-eng" -c "$PROJECT_PATH"

# Window 7: QA Test Lead (QA-001)
tmux new-window -t week-2 -n "qa-test-lead" -c "$PROJECT_PATH"

# Window 8: QA Test Engineer (QA-002)
tmux new-window -t week-2 -n "qa-test-engineer" -c "$PROJECT_PATH"

# Window 9: DevOps Engineer (OPS-001)
tmux new-window -t week-2 -n "devops-engineer-ops001" -c "$PROJECT_PATH"

echo "✅ week-2 session created successfully!"
echo ""
echo "=== week-2 Session 窗口列表 ==="
echo "Window 0: tmux-project-manager      # 项目经理"
echo "Window 1: system-architect           # ARCH-001 系统架构师"
echo "Window 2: c-tmux-specialist          # CORE-001 事件循环(T-201)"
echo "Window 3: libtmux-core-developer     # CORE-002 网格操作(T-202)"
echo "Window 4: zig-ghostty-integration    # INTG-001 FFI+集成(T-301,T-302)"
echo "Window 5: integration-dev            # INTG-002 Copy模式(T-204)"
echo "Window 6: performance-eng            # INTG-003 性能优化(T-303,T-402)"
echo "Window 7: qa-test-lead               # QA-001 测试策略(T-401)"
echo "Window 8: qa-test-engineer           # QA-002 缺陷修复(T-404)"
echo "Window 9: devops-engineer-ops001     # OPS-001 构建系统"
echo ""
echo "=== 启动所有 Agents ==="
echo "运行以下命令启动所有Claude agents:"
echo ""
cat << 'EOF'
# 启动所有agents的脚本
for i in {0..9}; do
  echo "Starting agent in window $i..."
  tmux send-keys -t week-2:$i "claude --dangerously-skip-permissions" Enter
  sleep 3
done
EOF
echo ""
echo "=== 访问命令 ==="
echo "tmux attach -t week-2        # 进入session"
echo "tmux list-windows -t week-2  # 查看所有窗口"