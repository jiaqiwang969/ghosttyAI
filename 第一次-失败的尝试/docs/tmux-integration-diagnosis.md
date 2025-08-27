# Ghostty × tmux 集成问题诊断报告

## 🔍 核心问题

**根本原因**：Ghostty编译时没有链接libtmuxcore.dylib

### 证据链：

1. **libtmuxcore.dylib存在且包含所需符号**
   - `tmc_input_process_key` ✅ (地址: 0x20b4)
   - `tmc_init` ✅
   - `tmc_session_new` ✅
   - 等等...

2. **Ghostty代码已正确修改**
   - `Termio.zig` 拦截键盘输入 ✅
   - `tmux_integration.zig` 调用tmc函数 ✅
   - 回调注册代码 ✅

3. **但是：Ghostty二进制没有链接libtmuxcore**
   - `otool -L ghostty` 没有显示libtmuxcore.dylib ❌
   - 这意味着运行时找不到tmc_input_process_key函数

## 🎯 真正的解决方案

### 方案A：静态链接（推荐）
将libtmuxcore编译为.a静态库，直接嵌入Ghostty

### 方案B：动态链接
修改Ghostty的build.zig，正确链接libtmuxcore.dylib

## 📋 需要执行的步骤

### 1. 修改Ghostty的build.zig
```zig
// 添加链接指令
exe.addLibraryPath(.{ .path = "../tmux" });
exe.linkSystemLibrary("tmuxcore");
// 或者直接链接
exe.addObjectFile(.{ .path = "../tmux/libtmuxcore.a" });
```

### 2. 重新编译libtmuxcore为静态库
```bash
# 修改Makefile.libtmuxcore
ar rcs libtmuxcore.a *.o
```

### 3. 确保符号可见性
```c
// 在所有tmc_函数前加上
__attribute__((visibility("default")))
```

## 🚫 为什么当前不工作

当Ghostty运行时：
1. 键盘输入被`queueWrite`捕获 ✅
2. 调用`handleKeyInput` ✅
3. 尝试调用`tmc_input_process_key` ❌
4. **失败**：因为符号未定义（库未链接）

## ✅ 验证方法

修复后，应该能：
```bash
# 1. 看到链接
otool -L ghostty | grep tmux

# 2. 找到符号
nm ghostty | grep tmc_input_process_key

# 3. 实际测试
./ghostty
# 按Ctrl-B %应该分屏
```