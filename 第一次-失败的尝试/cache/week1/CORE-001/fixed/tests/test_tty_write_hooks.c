// test_tty_write_hooks.c - Unit tests for TTY write hook implementations
// Purpose: Validate all 22 tty_cmd_* hook functions
// Author: CORE-001 (c-tmux-specialist)
// Date: 2025-08-25
// Version: 1.1.0 - Fixed interface naming (DEFECT-002)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

// Mock structures for testing
struct tty {
    int fd;
    int flags;
};

struct tty_ctx {
    int pane_id;
    int row;
    int col;
    int num;
    void* data;
};

struct grid_cell {
    char ch;
    int attrs;
};

// Include our implementation
#include "../ARCH-001/ui_backend.h"
#include "tty_write_hooks.h"

// ============================================================================
// Test Infrastructure
// ============================================================================

typedef struct {
    int tests_run;
    int tests_passed;
    int tests_failed;
    char last_error[256];
} test_context_t;

static test_context_t g_test_ctx = {0};

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        snprintf(g_test_ctx.last_error, sizeof(g_test_ctx.last_error), \
                 "%s:%d: %s", __FILE__, __LINE__, msg); \
        return false; \
    } \
} while(0)

#define RUN_TEST(func) do { \
    printf("Running %s... ", #func); \
    g_test_ctx.tests_run++; \
    if (func()) { \
        printf("PASS\n"); \
        g_test_ctx.tests_passed++; \
    } else { \
        printf("FAIL: %s\n", g_test_ctx.last_error); \
        g_test_ctx.tests_failed++; \
    } \
} while(0)

// ============================================================================
// Mock Backend Implementation
// ============================================================================

typedef struct {
    int call_counts[22];
    const struct tty_ctx* last_ctx;
    bool enabled;
} mock_backend_data_t;

static mock_backend_data_t g_mock_data = {0};

// Mock backend operation functions
static void mock_cmd_cell(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[0]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_cells(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[1]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_insertcharacter(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[2]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_deletecharacter(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[3]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_clearcharacter(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[4]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_insertline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[5]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_deleteline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[6]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_clearline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[7]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_clearendofline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[8]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_clearstartofline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[9]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_clearscreen(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[10]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_clearendofscreen(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[11]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_clearstartofscreen(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[12]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_alignmenttest(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[13]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_reverseindex(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[14]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_linefeed(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[15]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_scrollup(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[16]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_scrolldown(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[17]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_setselection(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[18]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_rawstring(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[19]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_sixelimage(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[20]++;
    g_mock_data.last_ctx = ctx;
}

static void mock_cmd_syncstart(struct ui_backend* backend, const struct tty_ctx* ctx) {
    g_mock_data.call_counts[21]++;
    g_mock_data.last_ctx = ctx;
}

// Mock backend operations table
static ui_backend_ops_t mock_ops = {
    .size = sizeof(ui_backend_ops_t),
    .version = UI_BACKEND_ABI_VERSION,
    .cmd_cell = mock_cmd_cell,
    .cmd_cells = mock_cmd_cells,
    .cmd_insertcharacter = mock_cmd_insertcharacter,
    .cmd_deletecharacter = mock_cmd_deletecharacter,
    .cmd_clearcharacter = mock_cmd_clearcharacter,
    .cmd_insertline = mock_cmd_insertline,
    .cmd_deleteline = mock_cmd_deleteline,
    .cmd_clearline = mock_cmd_clearline,
    .cmd_clearendofline = mock_cmd_clearendofline,
    .cmd_clearstartofline = mock_cmd_clearstartofline,
    .cmd_clearscreen = mock_cmd_clearscreen,
    .cmd_clearendofscreen = mock_cmd_clearendofscreen,
    .cmd_clearstartofscreen = mock_cmd_clearstartofscreen,
    .cmd_alignmenttest = mock_cmd_alignmenttest,
    .cmd_reverseindex = mock_cmd_reverseindex,
    .cmd_linefeed = mock_cmd_linefeed,
    .cmd_scrollup = mock_cmd_scrollup,
    .cmd_scrolldown = mock_cmd_scrolldown,
    .cmd_setselection = mock_cmd_setselection,
    .cmd_rawstring = mock_cmd_rawstring,
    .cmd_sixelimage = mock_cmd_sixelimage,
    .cmd_syncstart = mock_cmd_syncstart,
};

// Mock backend instance
static struct ui_backend mock_backend = {
    .size = sizeof(struct ui_backend),
    .version = UI_BACKEND_ABI_VERSION,
    .type = UI_BACKEND_TEST,
    .ops = &mock_ops,
    .aggregator = NULL,
    .capabilities = {
        .size = sizeof(ui_capabilities_t),
        .version = UI_BACKEND_ABI_VERSION,
        .supported = UI_CAP_FRAME_BATCH | UI_CAP_24BIT_COLOR,
    },
    .user_data = &g_mock_data,
    .priv = NULL,
};

// ============================================================================
// Test Functions
// ============================================================================

bool test_hook_initialization(void) {
    tty_hooks_init();
    
    int count = tty_hooks_get_count();
    TEST_ASSERT(count == 22, "Should have exactly 22 hooks");
    
    // Verify all function names are accessible
    for (int i = 0; i < count; i++) {
        const char* name = tty_hooks_get_function_name(i);
        TEST_ASSERT(name != NULL, "Function name should not be NULL");
        TEST_ASSERT(strstr(name, "tty_cmd_") == name, "Function should start with tty_cmd_");
    }
    
    return true;
}

bool test_hook_installation(void) {
    tty_hooks_init();
    
    // Install hooks with mock backend
    int ret = tty_hooks_install(&mock_backend);
    TEST_ASSERT(ret == 0, "Hook installation should succeed");
    
    // Uninstall hooks
    ret = tty_hooks_uninstall();
    TEST_ASSERT(ret == 0, "Hook uninstallation should succeed");
    
    // Test NULL backend
    ret = tty_hooks_install(NULL);
    TEST_ASSERT(ret == -1, "Should fail with NULL backend");
    
    return true;
}

bool test_hook_routing_cell(void) {
    // Reset mock data
    memset(&g_mock_data, 0, sizeof(g_mock_data));
    
    // Initialize and install hooks
    tty_hooks_init();
    tty_hooks_install(&mock_backend);
    
    // Create test context
    struct tty tty = {.fd = 1, .flags = 0};
    struct tty_ctx ctx = {.pane_id = 1, .row = 10, .col = 20};
    
    // Call the hooked function
    tty_cmd_cell(&tty, &ctx);
    
    // Verify the mock backend was called
    TEST_ASSERT(g_mock_data.call_counts[0] == 1, "cmd_cell should be called once");
    TEST_ASSERT(g_mock_data.last_ctx == &ctx, "Context should be passed correctly");
    
    tty_hooks_uninstall();
    return true;
}

bool test_hook_routing_all_functions(void) {
    // Reset mock data
    memset(&g_mock_data, 0, sizeof(g_mock_data));
    
    // Initialize and install hooks
    tty_hooks_init();
    tty_hooks_install(&mock_backend);
    
    struct tty tty = {.fd = 1, .flags = 0};
    struct tty_ctx ctx = {.pane_id = 1, .row = 10, .col = 20};
    
    // Call all 22 functions
    tty_cmd_cell(&tty, &ctx);
    tty_cmd_cells(&tty, &ctx);
    tty_cmd_insertcharacter(&tty, &ctx);
    tty_cmd_deletecharacter(&tty, &ctx);
    tty_cmd_clearcharacter(&tty, &ctx);
    tty_cmd_insertline(&tty, &ctx);
    tty_cmd_deleteline(&tty, &ctx);
    tty_cmd_clearline(&tty, &ctx);
    tty_cmd_clearendofline(&tty, &ctx);
    tty_cmd_clearstartofline(&tty, &ctx);
    tty_cmd_clearscreen(&tty, &ctx);
    tty_cmd_clearendofscreen(&tty, &ctx);
    tty_cmd_clearstartofscreen(&tty, &ctx);
    tty_cmd_alignmenttest(&tty, &ctx);
    tty_cmd_reverseindex(&tty, &ctx);
    tty_cmd_linefeed(&tty, &ctx);
    tty_cmd_scrollup(&tty, &ctx);
    tty_cmd_scrolldown(&tty, &ctx);
    tty_cmd_setselection(&tty, &ctx);
    tty_cmd_rawstring(&tty, &ctx);
    tty_cmd_sixelimage(&tty, &ctx);
    tty_cmd_syncstart(&tty, &ctx);
    
    // Verify all functions were called
    for (int i = 0; i < 22; i++) {
        TEST_ASSERT(g_mock_data.call_counts[i] == 1, 
                   "Each function should be called exactly once");
    }
    
    tty_hooks_uninstall();
    return true;
}

bool test_hook_statistics(void) {
    tty_hooks_init();
    tty_hooks_reset_stats();
    
    tty_hook_stats_t stats;
    tty_hooks_get_stats(&stats);
    
    TEST_ASSERT(stats.total_calls == 0, "Initial total calls should be 0");
    TEST_ASSERT(stats.intercepted_calls == 0, "Initial intercepted calls should be 0");
    
    // Note: Full statistics tracking would be implemented in production version
    
    return true;
}

bool test_function_name_lookup(void) {
    tty_hooks_init();
    
    // Test valid indices
    const char* name0 = tty_hooks_get_function_name(0);
    TEST_ASSERT(strcmp(name0, "tty_cmd_cell") == 0, "First function should be tty_cmd_cell");
    
    const char* name21 = tty_hooks_get_function_name(21);
    TEST_ASSERT(strcmp(name21, "tty_cmd_syncstart") == 0, "Last function should be tty_cmd_syncstart");
    
    // Test invalid indices
    const char* invalid1 = tty_hooks_get_function_name(-1);
    TEST_ASSERT(invalid1 == NULL, "Negative index should return NULL");
    
    const char* invalid2 = tty_hooks_get_function_name(22);
    TEST_ASSERT(invalid2 == NULL, "Out of bounds index should return NULL");
    
    return true;
}

bool test_backend_null_safety(void) {
    // Create backend with NULL ops
    struct ui_backend null_ops_backend = {
        .size = sizeof(struct ui_backend),
        .version = UI_BACKEND_ABI_VERSION,
        .type = UI_BACKEND_TEST,
        .ops = NULL,  // NULL operations table
    };
    
    tty_hooks_init();
    tty_hooks_install(&null_ops_backend);
    
    struct tty tty = {.fd = 1, .flags = 0};
    struct tty_ctx ctx = {.pane_id = 1, .row = 10, .col = 20};
    
    // These should not crash even with NULL ops
    tty_cmd_cell(&tty, &ctx);
    tty_cmd_clearscreen(&tty, &ctx);
    
    tty_hooks_uninstall();
    return true;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char** argv) {
    printf("=============================================================\n");
    printf("TTY Write Hooks Test Suite\n");
    printf("Testing all 22 tty_cmd_* hook functions\n");
    printf("=============================================================\n\n");
    
    // Run all tests
    RUN_TEST(test_hook_initialization);
    RUN_TEST(test_hook_installation);
    RUN_TEST(test_hook_routing_cell);
    RUN_TEST(test_hook_routing_all_functions);
    RUN_TEST(test_hook_statistics);
    RUN_TEST(test_function_name_lookup);
    RUN_TEST(test_backend_null_safety);
    
    printf("\n=============================================================\n");
    printf("Test Results:\n");
    printf("  Tests Run:    %d\n", g_test_ctx.tests_run);
    printf("  Tests Passed: %d\n", g_test_ctx.tests_passed);
    printf("  Tests Failed: %d\n", g_test_ctx.tests_failed);
    printf("  Success Rate: %.1f%%\n", 
           g_test_ctx.tests_run > 0 ? 
           (100.0 * g_test_ctx.tests_passed / g_test_ctx.tests_run) : 0.0);
    printf("=============================================================\n");
    
    return g_test_ctx.tests_failed > 0 ? 1 : 0;
}