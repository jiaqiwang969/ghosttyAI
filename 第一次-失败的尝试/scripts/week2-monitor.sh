#!/bin/bash
# 第二周实时监控脚本

echo "=== 第二周任务执行监控 ==="
echo "时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo ""

# T-201 状态 (CORE-001)
echo "📍 T-201 事件循环vtable (c-tmux-specialist):"
tmux capture-pane -t week-2:2 -p | grep -E "(完成|进度|ERROR|WARNING)" | tail -3

# T-301 状态 (INTG-001)
echo ""
echo "📍 T-301 FFI类型映射 (zig-ghostty-integration):"
tmux capture-pane -t week-2:4 -p | grep -E "(完成|进度|ERROR|WARNING)" | tail -3

# 检查关键交付物
echo ""
echo "📊 关键交付物检查:"
if [ -f "cache/week2/CORE-001/event_loop_backend.h" ]; then
  echo "✅ event_loop_backend.h 已创建"
else
  echo "⏳ event_loop_backend.h 待创建"
fi

if [ -f "cache/week2/INTG-001/ffi/c_types.zig" ]; then
  echo "✅ c_types.zig 已创建"
else
  echo "⏳ c_types.zig 待创建"
fi

echo ""
echo "⏰ 距离M2.1截止时间 (周三10:00) 还有: $(( ($(date -d "Wednesday 10:00" +%s) - $(date +%s)) / 3600 )) 小时"
