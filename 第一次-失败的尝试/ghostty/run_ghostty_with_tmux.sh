#!/bin/bash
# 确保libtmuxcore可以被找到
export DYLD_LIBRARY_PATH="/Users/jqwang/98-ghosttyAI/tmux:$DYLD_LIBRARY_PATH"
export GHOSTTY_TMUX_ENABLED=1

# 移除有问题的Sparkle框架
GHOSTTY_APP="/Users/jqwang/98-ghosttyAI/build/ghostty/Ghostty.app"
if [ -d "$GHOSTTY_APP/Contents/Frameworks/Sparkle.framework" ]; then
    rm -rf "$GHOSTTY_APP/Contents/Frameworks/Sparkle.framework"
fi

# 复制libtmuxcore到app bundle
cp -f /Users/jqwang/98-ghosttyAI/tmux/libtmuxcore.dylib "$GHOSTTY_APP/Contents/Frameworks/" 2>/dev/null

echo "正在启动Ghostty with tmux..."
echo ""
echo "使用方法:"
echo "  Ctrl-B %   - 垂直分屏"
echo "  Ctrl-B \"  - 水平分屏"
echo "  Ctrl-B c   - 新窗口"
echo "  Ctrl-B 方向键 - 切换焦点"
echo ""

# 启动Ghostty
"$GHOSTTY_APP/Contents/MacOS/ghostty"
