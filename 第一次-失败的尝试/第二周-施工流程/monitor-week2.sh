#!/bin/bash
# 第二周进展监控脚本
# 用于实时监控任务执行状态

echo "=== 📊 第二周实时进展监控 ==="
echo "更新时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo ""

# 统计完成情况
completed_count=0
in_progress_count=0

# 检查完成的任务
echo "✅ 已完成任务："
for task in T-201 T-202 T-301 T-302; do
    case $task in
        T-201) 
            if [ -f "/Users/jqwang/98-ghosttyAI/cache/week2/CORE-001/src/event_loop_backend.h" ]; then
                echo "  $task 事件循环vtable ✅"
                ((completed_count++))
            fi
            ;;
        T-202)
            if [ -f "/Users/jqwang/98-ghosttyAI/cache/week2/CORE-002/include/grid_callbacks.h" ]; then
                echo "  $task 网格操作callbacks ✅"
                ((completed_count++))
            fi
            ;;
        T-301)
            if [ -f "/Users/jqwang/98-ghosttyAI/cache/week2/INTG-001/ffi/c_types.zig" ]; then
                echo "  $task FFI类型映射 ✅"
                ((completed_count++))
            fi
            ;;
        T-302)
            if [ -f "/Users/jqwang/98-ghosttyAI/cache/week2/INTG-001/integration.zig" ]; then
                echo "  $task Ghostty集成层 ✅"
                ((completed_count++))
            fi
            ;;
    esac
done

echo ""
echo "🔄 执行中任务："
# 检查各窗口状态
for window in 2 3 4 5 6 7; do
    case $window in
        2) task="T-203 布局管理" ;;
        3) task="T-202 网格操作" ;;
        4) task="T-302 集成层" ;;
        5) task="T-204 Copy模式" ;;
        6) task="T-303 内存验证" ;;
        7) task="T-401 集成测试" ;;
    esac
    
    status=$(tmux capture-pane -t week-2:$window -p 2>/dev/null | grep -E "Working|Musing|Beaming" | tail -1)
    if [ -n "$status" ]; then
        echo "  Window $window: $task 🔄"
        ((in_progress_count++))
    fi
done

echo ""
echo "📈 进度统计："
total_tasks=12
completion_rate=$((completed_count * 100 / total_tasks))
echo "  完成: $completed_count/$total_tasks ($completion_rate%)"
echo "  执行中: $in_progress_count"
echo "  待启动: $((total_tasks - completed_count - in_progress_count))"

echo ""
echo "⏰ 关键截止时间："
echo "  T-203 布局管理: 周四 17:00"
echo "  T-204 Copy模式: 周四 17:00"
echo "  T-401 集成测试: 周五 14:00"
echo "  M2.3 Demo展示: 周五 14:00"

echo ""
echo "🎯 下一个关键检查点："
if [ $completion_rate -lt 50 ]; then
    echo "  优先完成T-203布局管理，这是Demo的关键组件"
else
    echo "  准备Demo集成，确保所有组件协同工作"
fi