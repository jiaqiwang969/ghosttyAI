#!/bin/bash
# 项目经理实时监控脚本 - P0缺陷修复进度
# 运行方式: bash monitor-fixes.sh

echo "====================================="
echo "P0缺陷修复进度监控面板"
echo "开始时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo "====================================="

while true; do
  clear
  echo "====================================="
  echo "P0缺陷修复进度监控 - $(date '+%H:%M:%S')"
  echo "====================================="
  echo ""
  
  # 检查ARCH-001进度
  echo "📐 ARCH-001 (架构师) - DEFECT-001修复:"
  echo "目标: 统一struct tty_ctx定义"
  if [ -f "/Users/jqwang/98-ghosttyAI/cache/week1/ARCH-001/tty_ctx_unified.h" ]; then
    echo "✅ tty_ctx_unified.h 已创建"
  else
    echo "⏳ 等待创建..."
  fi
  tmux capture-pane -t my-project:1 -p | tail -3 | sed 's/^/  /'
  echo ""
  
  # 检查CORE-001进度
  echo "🔧 CORE-001 (C专家) - DEFECT-002修复:"
  echo "目标: 修复函数接口不匹配"
  if [ -d "/Users/jqwang/98-ghosttyAI/cache/week1/CORE-001/fixed" ]; then
    echo "✅ 修复目录已创建"
    ls -la /Users/jqwang/98-ghosttyAI/cache/week1/CORE-001/fixed/ 2>/dev/null | tail -3
  else
    echo "⏳ 等待修复..."
  fi
  tmux capture-pane -t my-project:3 -p | tail -3 | sed 's/^/  /'
  echo ""
  
  # 检查CORE-002进度
  echo "🔨 CORE-002 (库开发) - DEFECT-002修复:"
  echo "目标: Router接口统一"
  if [ -d "/Users/jqwang/98-ghosttyAI/cache/week1/CORE-002/fixed" ]; then
    echo "✅ 修复目录已创建"
  else
    echo "⏳ 等待修复..."
  fi
  tmux capture-pane -t my-project:4 -p | tail -3 | sed 's/^/  /'
  echo ""
  
  # 检查INTG-001进度
  echo "🌉 INTG-001 (Zig集成) - 测试覆盖率提升:"
  echo "目标: 30% → 50%"
  tmux capture-pane -t my-project:5 -p | grep -i "coverage\|test" | tail -2 | sed 's/^/  /'
  echo ""
  
  # 检查QA-001准备
  echo "🧪 QA-001 (测试主管) - 验证准备:"
  echo "目标: 验证测试框架就绪"
  tmux capture-pane -t my-project:6 -p | tail -3 | sed 's/^/  /'
  echo ""
  
  # 统计进度
  echo "====================================="
  echo "📊 整体进度统计:"
  FIXED_COUNT=0
  [ -f "/Users/jqwang/98-ghosttyAI/cache/week1/ARCH-001/tty_ctx_unified.h" ] && ((FIXED_COUNT++))
  [ -d "/Users/jqwang/98-ghosttyAI/cache/week1/CORE-001/fixed" ] && ((FIXED_COUNT++))
  [ -d "/Users/jqwang/98-ghosttyAI/cache/week1/CORE-002/fixed" ] && ((FIXED_COUNT++))
  
  echo "P0缺陷修复: $FIXED_COUNT/3 完成"
  echo "时间进度: $(date '+%H:%M') / 23:00 截止"
  echo ""
  
  # 风险警告
  CURRENT_HOUR=$(date +%H)
  if [ $CURRENT_HOUR -ge 22 ] && [ $FIXED_COUNT -lt 2 ]; then
    echo "⚠️  警告: 时间紧迫，请加速修复！"
  fi
  
  echo "====================================="
  echo "按 Ctrl+C 退出监控"
  
  sleep 60  # 每分钟刷新
done