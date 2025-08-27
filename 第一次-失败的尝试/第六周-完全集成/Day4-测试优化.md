# Day 4 - 功能验证与性能优化

## 🎯 Day 4 目标
全面验证tmux功能在Ghostty中的集成效果，进行性能优化，修复发现的问题。

## 📋 详细任务清单

### Task 4.1: 端到端功能测试套件 (2小时)

#### 创建文件
`/Users/jqwang/98-ghosttyAI/tests/week6/test_tmux_integration.c`

```c
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../../tmux/libtmuxcore_api.h"

// 测试框架
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running test: %s... ", #name); \
    test_##name(); \
    printf("✓\n"); \
} while(0)

// 测试用例
TEST(init_cleanup) {
    assert(tmc_init() == TMC_SUCCESS);
    assert(tmc_init() == TMC_SUCCESS); // 重复初始化应该安全
    tmc_cleanup();
    tmc_cleanup(); // 重复清理应该安全
}

TEST(session_lifecycle) {
    assert(tmc_init() == TMC_SUCCESS);
    
    tmc_session_t session1, session2;
    assert(tmc_session_new("test-session-1", &session1) == TMC_SUCCESS);
    assert(tmc_session_new("test-session-2", &session2) == TMC_SUCCESS);
    assert(tmc_session_new("test-session-1", &session1) == TMC_ERROR_ALREADY_EXISTS);
    
    assert(tmc_session_attach(session1) == TMC_SUCCESS);
    assert(tmc_session_current() == session1);
    
    assert(tmc_session_rename(session1, "renamed-session") == TMC_SUCCESS);
    assert(tmc_session_destroy(session1) == TMC_SUCCESS);
    
    tmc_cleanup();
}

TEST(window_management) {
    assert(tmc_init() == TMC_SUCCESS);
    
    tmc_session_t session;
    assert(tmc_session_new("test", &session) == TMC_SUCCESS);
    
    tmc_window_t win1, win2, win3;
    assert(tmc_window_new(session, "window-1", &win1) == TMC_SUCCESS);
    assert(tmc_window_new(session, "window-2", &win2) == TMC_SUCCESS);
    assert(tmc_window_new(session, "window-3", &win3) == TMC_SUCCESS);
    
    assert(tmc_window_select(win2) == TMC_SUCCESS);
    assert(tmc_window_current() == win2);
    
    assert(tmc_window_next() == TMC_SUCCESS);
    assert(tmc_window_current() == win3);
    
    assert(tmc_window_previous() == TMC_SUCCESS);
    assert(tmc_window_current() == win2);
    
    assert(tmc_window_close(win2) == TMC_SUCCESS);
    
    tmc_cleanup();
}

TEST(pane_operations) {
    assert(tmc_init() == TMC_SUCCESS);
    
    tmc_session_t session;
    tmc_window_t window;
    assert(tmc_session_new("test", &session) == TMC_SUCCESS);
    assert(tmc_window_new(session, "window", &window) == TMC_SUCCESS);
    
    tmc_pane_t pane1, pane2, pane3;
    assert(tmc_pane_split(window, true, 50, &pane1) == TMC_SUCCESS);  // 水平分割
    assert(tmc_pane_split(window, false, 50, &pane2) == TMC_SUCCESS); // 垂直分割
    assert(tmc_pane_split(window, true, 30, &pane3) == TMC_SUCCESS);
    
    assert(tmc_pane_select(pane1) == TMC_SUCCESS);
    assert(tmc_pane_current() == pane1);
    
    assert(tmc_pane_resize(pane1, 100, 50) == TMC_SUCCESS);
    assert(tmc_pane_zoom_toggle(pane1) == TMC_SUCCESS);
    assert(tmc_pane_zoom_toggle(pane1) == TMC_SUCCESS); // 取消缩放
    
    assert(tmc_pane_close(pane3) == TMC_SUCCESS);
    
    tmc_cleanup();
}

TEST(command_execution) {
    assert(tmc_init() == TMC_SUCCESS);
    
    assert(tmc_command_execute("new-session -s cmd-test") == TMC_SUCCESS);
    assert(tmc_command_execute("new-window -n test-window") == TMC_SUCCESS);
    assert(tmc_command_execute("split-window -h") == TMC_SUCCESS);
    assert(tmc_command_execute("list-sessions") == TMC_SUCCESS);
    
    tmc_pane_t pane = tmc_pane_current();
    assert(pane != NULL);
    assert(tmc_command_send_keys(pane, "echo 'Hello tmux'\n") == TMC_SUCCESS);
    
    tmc_cleanup();
}

// 性能测试
TEST(performance_benchmark) {
    assert(tmc_init() == TMC_SUCCESS);
    
    clock_t start = clock();
    
    // 创建多个会话
    for (int i = 0; i < 10; i++) {
        char name[32];
        sprintf(name, "perf-session-%d", i);
        tmc_session_t session;
        assert(tmc_session_new(name, &session) == TMC_SUCCESS);
        
        // 每个会话创建多个窗口
        for (int j = 0; j < 5; j++) {
            tmc_window_t window;
            sprintf(name, "window-%d", j);
            assert(tmc_window_new(session, name, &window) == TMC_SUCCESS);
            
            // 每个窗口分割窗格
            tmc_pane_t pane;
            assert(tmc_pane_split(window, true, 50, &pane) == TMC_SUCCESS);
        }
    }
    
    clock_t end = clock();
    double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Performance: Created 10 sessions x 5 windows in %.3f seconds\n", cpu_time);
    assert(cpu_time < 1.0); // 应该在1秒内完成
    
    tmc_cleanup();
}

int main() {
    printf("=== tmux Integration Tests ===\n");
    
    RUN_TEST(init_cleanup);
    RUN_TEST(session_lifecycle);
    RUN_TEST(window_management);
    RUN_TEST(pane_operations);
    RUN_TEST(command_execution);
    RUN_TEST(performance_benchmark);
    
    printf("\nAll tests passed! ✅\n");
    return 0;
}
```

### Task 4.2: Ghostty集成测试 (2小时)

#### 创建文件
`/Users/jqwang/98-ghosttyAI/tests/week6/test_ghostty_tmux.zig`

```zig
const std = @import("std");
const Terminal = @import("../../ghostty/src/terminal/Terminal.zig").Terminal;
const tmux = @import("../../ghostty/src/tmux/tmux_integration.zig");

test "Ghostty tmux initialization" {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    
    const config = .{
        .cols = 80,
        .rows = 24,
        .enable_tmux = true,
    };
    
    var terminal = try Terminal.init(gpa.allocator(), config);
    defer terminal.deinit();
    
    try std.testing.expect(terminal.tmux_enabled == true);
    try std.testing.expect(terminal.tmux_core != null);
    try std.testing.expect(terminal.current_session != null);
}

test "Ghostty tmux commands" {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    
    var terminal = try Terminal.init(gpa.allocator(), .{
        .cols = 80,
        .rows = 24,
        .enable_tmux = true,
    });
    defer terminal.deinit();
    
    // 测试分割命令
    try terminal.tmuxCommand("split-window -h");
    try terminal.tmuxCommand("split-window -v");
    
    // 测试窗口命令
    try terminal.tmuxCommand("new-window test");
    try terminal.tmuxCommand("next-window");
    try terminal.tmuxCommand("previous-window");
    
    // 测试会话命令
    try terminal.tmuxCommand("new-session test-session");
}

test "Ghostty tmux rendering callbacks" {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    
    var terminal = try Terminal.init(gpa.allocator(), .{
        .cols = 80,
        .rows = 24,
        .enable_tmux = true,
    });
    defer terminal.deinit();
    
    // 模拟tmux输出
    const test_text = "Hello from tmux!";
    if (terminal.current_pane) |pane| {
        try pane.sendKeys(test_text);
    }
    
    // 验证网格更新
    // 这里应该检查terminal.grid是否包含正确的内容
    // 实际测试需要更复杂的验证逻辑
}

test "Ghostty tmux keybindings" {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    
    var terminal = try Terminal.init(gpa.allocator(), .{
        .cols = 80,
        .rows = 24,
        .enable_tmux = true,
    });
    defer terminal.deinit();
    
    // 模拟按键序列
    const KeyEvent = @import("../../ghostty/src/input/KeyEvent.zig").KeyEvent;
    
    // Ctrl-B (prefix)
    try terminal.handleKey(KeyEvent{ .ctrl = true, .key = 'b' });
    
    // % (水平分割)
    try terminal.handleKey(KeyEvent{ .key = '%' });
    
    // 验证分割成功
    // 需要检查窗格数量是否增加
}
```

### Task 4.3: 性能分析和优化 (2小时)

#### 创建文件
`/Users/jqwang/98-ghosttyAI/tests/week6/benchmark.c`

```c
#include <stdio.h>
#include <time.h>
#include <mach/mach_time.h>
#include "../../tmux/libtmuxcore_api.h"
#include "../../tmux/ui_backend/ui_backend.h"

// 性能计数器
static uint64_t callback_count = 0;
static uint64_t total_latency_ns = 0;

// 性能测试回调
static void perf_write_cell(u_int x, u_int y, const struct grid_cell *gc) {
    (void)x; (void)y; (void)gc;
    callback_count++;
}

static void perf_move_cursor(u_int x, u_int y) {
    (void)x; (void)y;
    callback_count++;
}

// 获取纳秒时间戳
static uint64_t get_ns_timestamp() {
    static mach_timebase_info_data_t timebase;
    if (timebase.denom == 0) {
        mach_timebase_info(&timebase);
    }
    return mach_absolute_time() * timebase.numer / timebase.denom;
}

int main() {
    printf("=== Performance Benchmark ===\n\n");
    
    // 初始化
    assert(tmc_init() == TMC_SUCCESS);
    
    // 注册性能测试backend
    struct ui_backend_vtable vtable = {
        .write_cell = perf_write_cell,
        .move_cursor = perf_move_cursor,
    };
    ui_backend_register(&vtable);
    
    // 测试1: 会话创建性能
    printf("Test 1: Session Creation\n");
    uint64_t start = get_ns_timestamp();
    for (int i = 0; i < 100; i++) {
        char name[32];
        sprintf(name, "bench-session-%d", i);
        tmc_session_t session;
        tmc_session_new(name, &session);
    }
    uint64_t elapsed = get_ns_timestamp() - start;
    printf("  100 sessions: %.3f ms (%.0f ns/session)\n", 
           elapsed / 1e6, elapsed / 100.0);
    
    // 测试2: 窗格分割性能
    printf("\nTest 2: Pane Splitting\n");
    tmc_session_t session;
    tmc_window_t window;
    tmc_session_new("bench", &session);
    tmc_window_new(session, "bench-window", &window);
    
    start = get_ns_timestamp();
    for (int i = 0; i < 50; i++) {
        tmc_pane_t pane;
        tmc_pane_split(window, i % 2, 50, &pane);
    }
    elapsed = get_ns_timestamp() - start;
    printf("  50 pane splits: %.3f ms (%.0f ns/split)\n",
           elapsed / 1e6, elapsed / 50.0);
    
    // 测试3: 输出吞吐量
    printf("\nTest 3: Output Throughput\n");
    callback_count = 0;
    start = get_ns_timestamp();
    
    // 模拟大量输出
    for (int i = 0; i < 1000000; i++) {
        tmc_command_execute("echo test");
    }
    
    elapsed = get_ns_timestamp() - start;
    double ops_per_sec = callback_count * 1e9 / elapsed;
    printf("  Callbacks: %llu\n", callback_count);
    printf("  Time: %.3f ms\n", elapsed / 1e6);
    printf("  Throughput: %.0f ops/sec\n", ops_per_sec);
    
    // 测试4: 延迟测试
    printf("\nTest 4: Command Latency\n");
    uint64_t min_latency = UINT64_MAX;
    uint64_t max_latency = 0;
    uint64_t total_latency = 0;
    
    for (int i = 0; i < 1000; i++) {
        start = get_ns_timestamp();
        tmc_command_execute("list-sessions");
        uint64_t latency = get_ns_timestamp() - start;
        
        min_latency = latency < min_latency ? latency : min_latency;
        max_latency = latency > max_latency ? latency : max_latency;
        total_latency += latency;
    }
    
    printf("  Min: %.0f ns\n", (double)min_latency);
    printf("  Max: %.0f ns\n", (double)max_latency);
    printf("  Avg: %.0f ns\n", total_latency / 1000.0);
    
    // 验证性能目标
    printf("\n=== Performance Validation ===\n");
    if (ops_per_sec > 350000) {
        printf("✅ Throughput: %.0f ops/s (target: >350k)\n", ops_per_sec);
    } else {
        printf("❌ Throughput: %.0f ops/s (target: >350k)\n", ops_per_sec);
    }
    
    if (total_latency / 1000 < 100) {
        printf("✅ Average latency: %.0f ns (target: <100ns)\n", total_latency / 1000.0);
    } else {
        printf("❌ Average latency: %.0f ns (target: <100ns)\n", total_latency / 1000.0);
    }
    
    tmc_cleanup();
    return 0;
}
```

### Task 4.4: 内存泄漏检测 (1小时)

#### 创建脚本
`/Users/jqwang/98-ghosttyAI/scripts/memory_check.sh`

```bash
#!/bin/bash

echo "=== Memory Leak Detection ==="

# 使用leaks工具检测内存泄漏
run_leaks_test() {
    local test_name=$1
    local test_binary=$2
    
    echo "Testing: $test_name"
    
    # 运行测试并检测泄漏
    MallocStackLogging=1 leaks -atExit -- "$test_binary" > /tmp/leaks_output.txt 2>&1
    
    # 检查结果
    if grep -q "0 leaks for 0 total leaked bytes" /tmp/leaks_output.txt; then
        echo "  ✅ No memory leaks detected"
    else
        echo "  ❌ Memory leaks found:"
        grep "leaked bytes" /tmp/leaks_output.txt
    fi
}

# 编译测试程序
cd /Users/jqwang/98-ghosttyAI/tests/week6
clang -g -o test_integration test_tmux_integration.c -L../../tmux -ltmuxcore
clang -g -o benchmark benchmark.c -L../../tmux -ltmuxcore

# 运行泄漏检测
run_leaks_test "Integration Tests" "./test_integration"
run_leaks_test "Performance Benchmark" "./benchmark"

# Instruments分析（可选）
echo ""
echo "For detailed analysis, run:"
echo "  xcrun xctrace record --template 'Leaks' --launch ./test_integration"
```

### Task 4.5: Bug修复和优化实现 (1.5小时)

根据测试发现的问题进行修复。常见问题和解决方案：

#### 问题1: 回调函数内存管理
```c
// 修复前：可能泄漏
void handle_output(const struct tty_ctx *ctx) {
    char *buffer = malloc(1024);
    // 忘记释放
}

// 修复后：正确管理
void handle_output(const struct tty_ctx *ctx) {
    char *buffer = malloc(1024);
    // 处理...
    free(buffer);
}
```

#### 问题2: 性能瓶颈优化
```c
// 优化UI Backend调度
#ifdef LIBTMUXCORE_BUILD
// 添加批处理支持
static struct {
    struct grid_cell cells[1000];
    u_int x[1000];
    u_int y[1000];
    size_t count;
} batch_buffer;

void batch_write_cell(u_int x, u_int y, const struct grid_cell *gc) {
    if (batch_buffer.count >= 1000) {
        flush_batch();
    }
    batch_buffer.cells[batch_buffer.count] = *gc;
    batch_buffer.x[batch_buffer.count] = x;
    batch_buffer.y[batch_buffer.count] = y;
    batch_buffer.count++;
}

void flush_batch() {
    if (ui_backend && ui_backend->write_cells_batch) {
        ui_backend->write_cells_batch(batch_buffer.cells, 
                                      batch_buffer.x, 
                                      batch_buffer.y, 
                                      batch_buffer.count);
    }
    batch_buffer.count = 0;
}
#endif
```

## ⏰ 时间安排

| 时间段 | 任务 | 产出 |
|--------|------|------|
| 09:00-11:00 | Task 4.1 | 功能测试套件完成 |
| 11:00-13:00 | Task 4.2 | Ghostty集成测试 |
| 14:00-16:00 | Task 4.3 | 性能分析报告 |
| 16:00-17:00 | Task 4.4 | 内存泄漏检测 |
| 17:00-18:30 | Task 4.5 | Bug修复和优化 |

## ✅ Day 4 完成标准

- [ ] 所有功能测试通过
- [ ] 性能达标（>350k ops/s）
- [ ] 延迟<100ns
- [ ] 无内存泄漏
- [ ] 关键bug已修复
- [ ] Git提交："[WEEK6-D4] Testing and optimization complete"

---
*Day 4: Making it fast, stable, and production-ready!*