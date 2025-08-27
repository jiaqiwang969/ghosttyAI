# INTG-001 测试模板和最佳实践指南
# Test Template and Best Practices for INTG-001
# Author: QA-002 (qa-test-engineer)
# Date: 2025-08-25
# Purpose: 协助INTG-001提升测试覆盖率至50%

## 一、测试模板

### 1. 单元测试模板 (test_ghostty_backend_unit.c)

```c
// test_ghostty_backend_unit.c - Ghostty Backend单元测试
// 使用此模板快速创建新的测试用例

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "../backend_ghostty.h"
#include "../tty_ctx_unified.h"

// 测试统计
typedef struct {
    int total;
    int passed;
    int failed;
    char test_name[256];
    clock_t start_time;
} test_stats_t;

static test_stats_t g_stats = {0};

// 测试宏定义
#define TEST_BEGIN(name) do { \
    strcpy(g_stats.test_name, name); \
    printf("[TEST] %s... ", name); \
    g_stats.total++; \
    g_stats.start_time = clock(); \
} while(0)

#define TEST_ASSERT(condition, msg) do { \
    if (!(condition)) { \
        printf("FAIL: %s\n", msg); \
        g_stats.failed++; \
        return; \
    } \
} while(0)

#define TEST_END() do { \
    double elapsed = (double)(clock() - g_stats.start_time) / CLOCKS_PER_SEC * 1000; \
    printf("PASS (%.2fms)\n", elapsed); \
    g_stats.passed++; \
} while(0)

// Mock对象
static int mock_callback_count = 0;
static void mock_ghostty_callback(void* handle, const char* data, size_t len) {
    (void)handle;
    (void)data;
    (void)len;
    mock_callback_count++;
}

// =============================================================================
// 测试用例示例
// =============================================================================

// 测试1: Backend创建和销毁
void test_backend_lifecycle() {
    TEST_BEGIN("Backend Lifecycle");
    
    struct ui_backend* backend = ghostty_backend_create(NULL);
    TEST_ASSERT(backend != NULL, "Failed to create backend");
    
    // 验证初始化
    TEST_ASSERT(backend->type == UI_BACKEND_GHOSTTY, "Wrong backend type");
    TEST_ASSERT(backend->size > 0, "Invalid size");
    
    ghostty_backend_destroy(backend);
    TEST_END();
}

// 测试2: 回调函数注册
void test_callback_registration() {
    TEST_BEGIN("Callback Registration");
    
    struct ui_backend* backend = ghostty_backend_create(NULL);
    TEST_ASSERT(backend != NULL, "Failed to create backend");
    
    // 注册回调
    mock_callback_count = 0;
    ghostty_backend_register_callback(backend, "test", mock_ghostty_callback);
    
    // 触发回调
    ghostty_backend_trigger_callback(backend, "test", "data", 4);
    TEST_ASSERT(mock_callback_count == 1, "Callback not triggered");
    
    ghostty_backend_destroy(backend);
    TEST_END();
}

// 测试3: 命令处理
void test_command_processing() {
    TEST_BEGIN("Command Processing");
    
    struct ui_backend* backend = ghostty_backend_create(NULL);
    TEST_ASSERT(backend != NULL, "Failed to create backend");
    
    struct tty_ctx ctx;
    tty_ctx_init(&ctx);
    ctx.num = 1;
    ctx.ocx = 10;
    ctx.ocy = 20;
    
    // 测试各种命令
    int result = ghostty_backend_cmd_cell(backend, &ctx);
    TEST_ASSERT(result == 0, "cmd_cell failed");
    
    result = ghostty_backend_cmd_clearline(backend, &ctx);
    TEST_ASSERT(result == 0, "cmd_clearline failed");
    
    ghostty_backend_destroy(backend);
    TEST_END();
}

// 测试4: 错误处理
void test_error_handling() {
    TEST_BEGIN("Error Handling");
    
    // NULL参数测试
    struct ui_backend* backend = ghostty_backend_create(NULL);
    TEST_ASSERT(backend != NULL, "Failed to create backend");
    
    // 测试NULL context
    int result = ghostty_backend_cmd_cell(backend, NULL);
    TEST_ASSERT(result == -1, "Should fail with NULL context");
    
    // 测试无效命令
    result = ghostty_backend_process_invalid_cmd(backend, 9999);
    TEST_ASSERT(result == -1, "Should fail with invalid command");
    
    ghostty_backend_destroy(backend);
    
    // 测试NULL backend销毁
    ghostty_backend_destroy(NULL); // 不应崩溃
    
    TEST_END();
}

// 测试5: 性能测试
void test_performance() {
    TEST_BEGIN("Performance Benchmark");
    
    struct ui_backend* backend = ghostty_backend_create(NULL);
    TEST_ASSERT(backend != NULL, "Failed to create backend");
    
    struct tty_ctx ctx;
    tty_ctx_init(&ctx);
    
    const int iterations = 10000;
    clock_t start = clock();
    
    for (int i = 0; i < iterations; i++) {
        ghostty_backend_cmd_cell(backend, &ctx);
    }
    
    double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
    double ops_per_sec = iterations / elapsed;
    
    printf("\n  Performance: %.0f ops/sec", ops_per_sec);
    TEST_ASSERT(ops_per_sec > 50000, "Performance too low");
    
    ghostty_backend_destroy(backend);
    TEST_END();
}

// 测试6: 内存泄漏检查
void test_memory_leak() {
    TEST_BEGIN("Memory Leak Check");
    
    // 多次创建销毁，检查内存泄漏
    for (int i = 0; i < 100; i++) {
        struct ui_backend* backend = ghostty_backend_create(NULL);
        TEST_ASSERT(backend != NULL, "Failed to create backend");
        
        struct tty_ctx ctx;
        tty_ctx_init(&ctx);
        ghostty_backend_cmd_cell(backend, &ctx);
        
        ghostty_backend_destroy(backend);
    }
    
    // 使用valgrind验证: valgrind --leak-check=full ./test
    TEST_END();
}

// 主测试函数
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("=============================================================\n");
    printf("INTG-001 Ghostty Backend Unit Tests\n");
    printf("=============================================================\n\n");
    
    // 运行所有测试
    test_backend_lifecycle();
    test_callback_registration();
    test_command_processing();
    test_error_handling();
    test_performance();
    test_memory_leak();
    
    // 打印统计
    printf("\n=============================================================\n");
    printf("Test Results:\n");
    printf("  Total:  %d\n", g_stats.total);
    printf("  Passed: %d\n", g_stats.passed);
    printf("  Failed: %d\n", g_stats.failed);
    printf("  Coverage Target: 50%%\n");
    printf("=============================================================\n");
    
    return g_stats.failed > 0 ? 1 : 0;
}
```

## 二、集成测试模板

```c
// test_ghostty_integration.c - 集成测试模板

#include <pthread.h>
#include "../backend_ghostty.h"
#include "../backend_router.h"
#include "../tty_write_hooks.h"

// 测试完整的集成流程
void test_full_integration() {
    // 1. 初始化所有组件
    tty_hooks_init();
    backend_router_t* router = backend_router_create(BACKEND_MODE_UI);
    struct ui_backend* backend = ghostty_backend_create(NULL);
    
    // 2. 注册backend
    backend_router_register_ui(router, backend);
    
    // 3. 模拟tmux命令流
    struct tty_ctx ctx = {0};
    tty_ctx_init(&ctx);
    
    // 4. 测试各种场景
    // ...
    
    // 5. 清理
    ghostty_backend_destroy(backend);
    backend_router_destroy(router);
}

// 并发测试
void test_concurrent_operations() {
    // 测试多线程环境下的安全性
    // ...
}
```

## 三、最佳实践

### 1. 测试组织
- 每个功能模块一个测试文件
- 测试函数命名：test_<module>_<functionality>
- 使用统一的测试宏

### 2. 覆盖率目标
- 正常路径：100%覆盖
- 错误处理：>80%覆盖
- 边界条件：全部测试

### 3. 测试数据
```c
// 使用测试fixture
typedef struct {
    struct ui_backend* backend;
    struct tty_ctx* ctx;
    backend_router_t* router;
} test_fixture_t;

void setup_fixture(test_fixture_t* f) {
    f->backend = ghostty_backend_create(NULL);
    f->ctx = malloc(sizeof(struct tty_ctx));
    tty_ctx_init(f->ctx);
    f->router = backend_router_create(BACKEND_MODE_UI);
}

void teardown_fixture(test_fixture_t* f) {
    ghostty_backend_destroy(f->backend);
    free(f->ctx);
    backend_router_destroy(f->router);
}
```

### 4. Mock对象使用
```c
// Mock Ghostty handle
typedef struct mock_ghostty {
    int call_count;
    char last_command[256];
} mock_ghostty_t;

static mock_ghostty_t g_mock = {0};

void* mock_ghostty_create() {
    return &g_mock;
}
```

## 四、覆盖率提升策略

### 当前状态 (30%)
- ✅ 基本创建/销毁
- ✅ 简单命令测试
- ❌ 错误处理路径
- ❌ 并发测试
- ❌ 性能测试
- ❌ 边界条件

### 目标状态 (50%)
需要新增测试：
1. 所有22个命令函数
2. 错误处理（NULL参数、无效状态）
3. 并发安全性
4. 内存管理
5. 性能基准

### 具体任务
- [ ] 为每个cmd_*函数编写测试（22个）
- [ ] 添加负面测试用例（10个）
- [ ] 添加边界条件测试（5个）
- [ ] 添加并发测试（3个）
- [ ] 添加性能测试（2个）

## 五、编译和运行

### Makefile模板
```makefile
CC = gcc
CFLAGS = -Wall -Wextra -g -fprofile-arcs -ftest-coverage
INCLUDES = -I../include -I../../ARCH-001
LIBS = -lpthread

TESTS = test_ghostty_backend_unit \
        test_ghostty_integration \
        test_ghostty_commands

all: $(TESTS)

test_ghostty_backend_unit: test_ghostty_backend_unit.c ../backend_ghostty.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

run-tests: $(TESTS)
	@for test in $(TESTS); do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
	done

coverage:
	gcov *.c
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage_html

clean:
	rm -f $(TESTS) *.gcno *.gcda *.gcov
	rm -rf coverage_html

.PHONY: all run-tests coverage clean
```

## 六、验收标准

### 必须达到的指标
- 代码覆盖率 ≥ 50%
- 所有测试通过
- 无内存泄漏（valgrind验证）
- 性能基准达标

### 提交前检查清单
- [ ] 运行所有测试
- [ ] 生成覆盖率报告
- [ ] 代码review
- [ ] 文档更新

---

**支持联系**：QA-002随时提供协助
**截止时间**：周六 14:00