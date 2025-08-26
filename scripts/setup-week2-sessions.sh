#!/bin/bash
# 第二周 Tmux Session 设置脚本

PROJECT_PATH="/Users/jqwang/98-ghosttyAI"

echo "=== 创建第二周 Tmux Sessions ==="

# 1. 管理层 Sessions
echo "Creating management sessions..."
tmux new-session -d -s tmux-orchestrator -c "$PROJECT_PATH"
tmux rename-window -t tmux-orchestrator:0 "Orchestrator-PM"
tmux new-window -t tmux-orchestrator -n "Status-Monitor" -c "$PROJECT_PATH"
tmux new-window -t tmux-orchestrator -n "Cross-Team-Comm" -c "$PROJECT_PATH"

tmux new-session -d -s tmux-project-manager -c "$PROJECT_PATH"
tmux rename-window -t tmux-project-manager:0 "PM-Main"
tmux new-window -t tmux-project-manager -n "Task-Deployment" -c "$PROJECT_PATH"
tmux new-window -t tmux-project-manager -n "Risk-Monitor" -c "$PROJECT_PATH"
tmux new-window -t tmux-project-manager -n "Quality-Gate" -c "$PROJECT_PATH"

# 2. 架构师 Session
echo "Creating architect session..."
tmux new-session -d -s ghostty-tmux-architect -c "$PROJECT_PATH"
tmux rename-window -t ghostty-tmux-architect:0 "system-architect"
tmux new-window -t ghostty-tmux-architect -n "Design-Review" -c "$PROJECT_PATH"
tmux new-window -t ghostty-tmux-architect -n "Architecture-Docs" -c "$PROJECT_PATH"

# 3. 核心开发 Session
echo "Creating core development session..."
tmux new-session -d -s ghostty-core -c "$PROJECT_PATH"
tmux rename-window -t ghostty-core:0 "c-tmux-specialist"
tmux new-window -t ghostty-core -n "Build-Test" -c "$PROJECT_PATH"
tmux new-window -t ghostty-core -n "Shell" -c "$PROJECT_PATH"
tmux new-window -t ghostty-core -n "Git-Ops" -c "$PROJECT_PATH"
tmux new-window -t ghostty-core -n "libtmux-core-dev" -c "$PROJECT_PATH"

# 4. 集成开发 Session
echo "Creating integration session..."
tmux new-session -d -s ghostty-integration -c "$PROJECT_PATH"
tmux rename-window -t ghostty-integration:0 "zig-ghostty-integ"
tmux new-window -t ghostty-integration -n "FFI-Test" -c "$PROJECT_PATH"
tmux new-window -t ghostty-integration -n "Memory-Check" -c "$PROJECT_PATH"
tmux new-window -t ghostty-integration -n "Demo-Prep" -c "$PROJECT_PATH"
tmux new-window -t ghostty-integration -n "integration-dev" -c "$PROJECT_PATH"
tmux new-window -t ghostty-integration -n "performance-eng" -c "$PROJECT_PATH"

# 5. 质量保证 Session
echo "Creating quality session..."
tmux new-session -d -s ghostty-quality -c "$PROJECT_PATH"
tmux rename-window -t ghostty-quality:0 "qa-test-lead"
tmux new-window -t ghostty-quality -n "qa-test-engineer" -c "$PROJECT_PATH"
tmux new-window -t ghostty-quality -n "Coverage-Report" -c "$PROJECT_PATH"
tmux new-window -t ghostty-quality -n "Integration-Tests" -c "$PROJECT_PATH"

# 6. DevOps Session
echo "Creating devops session..."
tmux new-session -d -s ghostty-devops -c "$PROJECT_PATH"
tmux rename-window -t ghostty-devops:0 "devops-engineer"
tmux new-window -t ghostty-devops -n "Build-Pipeline" -c "$PROJECT_PATH"
tmux new-window -t ghostty-devops -n "Performance-Monitor" -c "$PROJECT_PATH"
tmux new-window -t ghostty-devops -n "Deploy-Status" -c "$PROJECT_PATH"

echo "✅ All sessions created successfully!"
echo ""
echo "=== Session 列表 ==="
tmux list-sessions

echo ""
echo "=== 快速访问命令 ==="
echo "tmux attach -t tmux-orchestrator      # 总协调器"
echo "tmux attach -t tmux-project-manager   # 项目经理"
echo "tmux attach -t ghostty-tmux-architect # 架构师"
echo "tmux attach -t ghostty-core           # 核心开发"
echo "tmux attach -t ghostty-integration    # 集成团队"
echo "tmux attach -t ghostty-quality        # 质量团队"
echo "tmux attach -t ghostty-devops         # DevOps"