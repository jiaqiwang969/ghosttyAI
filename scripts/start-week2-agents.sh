#!/bin/bash
# 启动week-2 session中所有Claude agents

echo "=== 启动 week-2 Session 中的所有 Claude Agents ==="

for i in {0..9}; do
  case $i in
    0) agent="tmux-project-manager" ;;
    1) agent="system-architect" ;;
    2) agent="c-tmux-specialist" ;;
    3) agent="libtmux-core-developer" ;;
    4) agent="zig-ghostty-integration" ;;
    5) agent="integration-dev" ;;
    6) agent="performance-eng" ;;
    7) agent="qa-test-lead" ;;
    8) agent="qa-test-engineer" ;;
    9) agent="devops-engineer-ops001" ;;
  esac
  
  echo "Starting $agent in window $i..."
  tmux send-keys -t week-2:$i "claude --dangerously-skip-permissions" Enter
  sleep 3
done

echo "✅ All agents started!"
echo ""
echo "Verify with: tmux capture-pane -t week-2:0 -p | head -5"