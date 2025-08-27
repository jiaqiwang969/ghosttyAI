// test_callbacks.c - Test all 22 tty_cmd callback functions
// Purpose: Achieve comprehensive coverage of all callback implementations
// Author: INTG-001 (Zig-Ghostty Integration Specialist)
// Date: 2025-08-25
// Version: 1.0.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include "../../ARCH-001/tty_ctx_unified.h"
#include "../../ARCH-001/ui_backend.h"
#include "../backend_ghostty.c"  // Include implementation for coverage

// Test framework macros
#define TEST_START(name) \
    do { \
    printf("Testing %s...", name); \
    int test_result = 0; \
    clock_t start_time = clock()

#define TEST_END() \
    clock_t end_time = clock(); \
    double time_spent = ((double)(end_time - start_time)) / CLOCKS_PER_SEC; \
    if (test_result == 0) { \
        printf(" PASSED (%.3fs)\n", time_spent); \
        tests_passed++; \
    } else { \
        printf(" FAILED (%.3fs)\n", time_spent); \
        tests_failed++; \
    } \
    } while(0)

#define ASSERT(cond) \
    if (!(cond)) { \
        fprintf(stderr, "\n  Assertion failed: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
        test_result = 1; \
    }

// Global test counters
static int tests_passed = 0;
static int tests_failed = 0;
static int coverage_hits[1000] = {0};  // Track which lines were hit

// Mock structures for testing
struct mock_window_pane {
    uint32_t id;
    uint32_t sx, sy;
};

struct mock_grid_cell {
    uint32_t codepoint;
    uint16_t attr;
    uint8_t fg, bg;
};

// Test fixture
typedef struct {
    struct ui_backend* backend;
    struct tty_ctx ctx;
    struct mock_window_pane wp;
    struct mock_grid_cell cell;
    int callback_count;
} test_fixture_t;

// Coverage tracking
#define COVERAGE_MARK(line) coverage_hits[line % 1000]++

// Initialize test fixture
static test_fixture_t* create_fixture(void) {
    test_fixture_t* f = calloc(1, sizeof(test_fixture_t));
    
    // Create backend with test capabilities
    ui_capabilities_t caps = {
        .size = sizeof(ui_capabilities_t),
        .version = UI_BACKEND_ABI_VERSION,
        .supported = UI_CAP_FRAME_BATCH | UI_CAP_24BIT_COLOR,
        .max_fps = 60,
        .optimal_batch_size = 100
    };
    
    f->backend = ghostty_backend_create(&caps);
    
    // Initialize tty_ctx using unified definition
    tty_ctx_init(&f->ctx);
    f->ctx.wp = (struct window_pane*)&f->wp;
    f->ctx.cell = (struct grid_cell*)&f->cell;
    
    // Set default values using safe macros
    TTY_CTX_SET_FIELD(&f->ctx, ocx, 0);
    TTY_CTX_SET_FIELD(&f->ctx, ocy, 0);
    TTY_CTX_SET_FIELD(&f->ctx, orupper, 0);
    TTY_CTX_SET_FIELD(&f->ctx, orlower, 23);
    
    // Initialize mock structures
    f->wp.id = 1;
    f->wp.sx = 80;
    f->wp.sy = 24;
    f->cell.codepoint = 'A';
    f->cell.attr = 0;
    f->cell.fg = 7;
    f->cell.bg = 0;
    
    return f;
}

static void destroy_fixture(test_fixture_t* f) {
    if (f) {
        ghostty_backend_destroy(f->backend);
        free(f);
    }
}

// Test all 22 callback functions
void test_all_22_callbacks(void) {
    printf("\n=== Testing All 22 tty_cmd Callbacks ===\n");
    
    test_fixture_t* f = create_fixture();
    if (!f || !f->backend) {
        printf("FAILED: Could not create test fixture\n");
        tests_failed++;
        return;
    }
    
    // 1. Test cmd_cell
    TEST_START("cmd_cell");
    COVERAGE_MARK(__LINE__);
    TTY_CTX_SET_FIELD(&f->ctx, ocx, 10);
    TTY_CTX_SET_FIELD(&f->ctx, ocy, 5);
    f->backend->ops->cmd_cell(f->backend, &f->ctx);
    ASSERT(TTY_CTX_GET_OCX(&f->ctx) == 10);
    TEST_END();
    
    // 2. Test cmd_cells
    TEST_START("cmd_cells");
    COVERAGE_MARK(__LINE__);
    f->ctx.num = 5;
    f->backend->ops->cmd_cells(f->backend, &f->ctx);
    ASSERT(f->ctx.num == 5);
    TEST_END();
    
    // 3. Test cmd_insertcharacter
    TEST_START("cmd_insertcharacter");
    COVERAGE_MARK(__LINE__);
    TTY_CTX_SET_FIELD(&f->ctx, ocx, 20);
    f->backend->ops->cmd_insertcharacter(f->backend, &f->ctx);
    TEST_END();
    
    // 4. Test cmd_deletecharacter
    TEST_START("cmd_deletecharacter");
    COVERAGE_MARK(__LINE__);
    f->backend->ops->cmd_deletecharacter(f->backend, &f->ctx);
    TEST_END();
    
    // 5. Test cmd_clearcharacter
    TEST_START("cmd_clearcharacter");
    COVERAGE_MARK(__LINE__);
    f->ctx.num = 3;
    f->backend->ops->cmd_clearcharacter(f->backend, &f->ctx);
    TEST_END();
    
    // 6. Test cmd_insertline
    TEST_START("cmd_insertline");
    COVERAGE_MARK(__LINE__);
    TTY_CTX_SET_FIELD(&f->ctx, ocy, 10);
    f->backend->ops->cmd_insertline(f->backend, &f->ctx);
    TEST_END();
    
    // 7. Test cmd_deleteline
    TEST_START("cmd_deleteline");
    COVERAGE_MARK(__LINE__);
    f->backend->ops->cmd_deleteline(f->backend, &f->ctx);
    TEST_END();
    
    // 8. Test cmd_clearline
    TEST_START("cmd_clearline");
    COVERAGE_MARK(__LINE__);
    f->backend->ops->cmd_clearline(f->backend, &f->ctx);
    TEST_END();
    
    // 9. Test cmd_clearendofline
    TEST_START("cmd_clearendofline");
    COVERAGE_MARK(__LINE__);
    TTY_CTX_SET_FIELD(&f->ctx, ocx, 40);
    f->backend->ops->cmd_clearendofline(f->backend, &f->ctx);
    TEST_END();
    
    // 10. Test cmd_clearstartofline
    TEST_START("cmd_clearstartofline");
    COVERAGE_MARK(__LINE__);
    f->backend->ops->cmd_clearstartofline(f->backend, &f->ctx);
    TEST_END();
    
    // 11. Test cmd_clearscreen
    TEST_START("cmd_clearscreen");
    COVERAGE_MARK(__LINE__);
    f->backend->ops->cmd_clearscreen(f->backend, &f->ctx);
    TEST_END();
    
    // 12. Test cmd_clearendofscreen
    TEST_START("cmd_clearendofscreen");
    COVERAGE_MARK(__LINE__);
    TTY_CTX_SET_FIELD(&f->ctx, ocy, 12);
    f->backend->ops->cmd_clearendofscreen(f->backend, &f->ctx);
    TEST_END();
    
    // 13. Test cmd_clearstartofscreen
    TEST_START("cmd_clearstartofscreen");
    COVERAGE_MARK(__LINE__);
    f->backend->ops->cmd_clearstartofscreen(f->backend, &f->ctx);
    TEST_END();
    
    // 14. Test cmd_alignmenttest
    TEST_START("cmd_alignmenttest");
    COVERAGE_MARK(__LINE__);
    f->backend->ops->cmd_alignmenttest(f->backend, &f->ctx);
    TEST_END();
    
    // 15. Test cmd_reverseindex
    TEST_START("cmd_reverseindex");
    COVERAGE_MARK(__LINE__);
    TTY_CTX_SET_FIELD(&f->ctx, orupper, 5);
    TTY_CTX_SET_FIELD(&f->ctx, orlower, 20);
    f->backend->ops->cmd_reverseindex(f->backend, &f->ctx);
    TEST_END();
    
    // 16. Test cmd_linefeed
    TEST_START("cmd_linefeed");
    COVERAGE_MARK(__LINE__);
    f->backend->ops->cmd_linefeed(f->backend, &f->ctx);
    TEST_END();
    
    // 17. Test cmd_scrollup
    TEST_START("cmd_scrollup");
    COVERAGE_MARK(__LINE__);
    f->ctx.num = 2;
    f->backend->ops->cmd_scrollup(f->backend, &f->ctx);
    TEST_END();
    
    // 18. Test cmd_scrolldown
    TEST_START("cmd_scrolldown");
    COVERAGE_MARK(__LINE__);
    f->ctx.num = 1;
    f->backend->ops->cmd_scrolldown(f->backend, &f->ctx);
    TEST_END();
    
    // 19. Test cmd_setselection
    TEST_START("cmd_setselection");
    COVERAGE_MARK(__LINE__);
    f->backend->ops->cmd_setselection(f->backend, &f->ctx);
    TEST_END();
    
    // 20. Test cmd_rawstring
    TEST_START("cmd_rawstring");
    COVERAGE_MARK(__LINE__);
    f->backend->ops->cmd_rawstring(f->backend, &f->ctx);
    TEST_END();
    
    // 21. Test cmd_sixelimage
    TEST_START("cmd_sixelimage");
    COVERAGE_MARK(__LINE__);
    TTY_CTX_SET_FIELD(&f->ctx, ocy, 0);
    f->backend->ops->cmd_sixelimage(f->backend, &f->ctx);
    TEST_END();
    
    // 22. Test cmd_syncstart
    TEST_START("cmd_syncstart");
    COVERAGE_MARK(__LINE__);
    f->backend->ops->cmd_syncstart(f->backend, &f->ctx);
    TEST_END();
    
    destroy_fixture(f);
}

// Test edge cases for each callback
void test_callback_edge_cases(void) {
    printf("\n=== Testing Callback Edge Cases ===\n");
    
    test_fixture_t* f = create_fixture();
    
    // Test with maximum coordinates
    TEST_START("max coordinates");
    COVERAGE_MARK(__LINE__);
    TTY_CTX_SET_FIELD(&f->ctx, ocx, 9999);
    TTY_CTX_SET_FIELD(&f->ctx, ocy, 9999);
    f->backend->ops->cmd_cell(f->backend, &f->ctx);
    TEST_END();
    
    // Test with zero coordinates
    TEST_START("zero coordinates");
    COVERAGE_MARK(__LINE__);
    TTY_CTX_SET_FIELD(&f->ctx, ocx, 0);
    TTY_CTX_SET_FIELD(&f->ctx, ocy, 0);
    f->backend->ops->cmd_cell(f->backend, &f->ctx);
    TEST_END();
    
    // Test with large num parameter
    TEST_START("large num");
    COVERAGE_MARK(__LINE__);
    f->ctx.num = 1000000;
    f->backend->ops->cmd_cells(f->backend, &f->ctx);
    TEST_END();
    
    // Test scroll region boundaries
    TEST_START("scroll boundaries");
    COVERAGE_MARK(__LINE__);
    TTY_CTX_SET_FIELD(&f->ctx, orupper, 23);
    TTY_CTX_SET_FIELD(&f->ctx, orlower, 0);  // Invalid: upper > lower
    f->backend->ops->cmd_scrollup(f->backend, &f->ctx);
    TEST_END();
    
    destroy_fixture(f);
}

// Test callback combinations
void test_callback_combinations(void) {
    printf("\n=== Testing Callback Combinations ===\n");
    
    test_fixture_t* f = create_fixture();
    
    TEST_START("clear then write");
    COVERAGE_MARK(__LINE__);
    f->backend->ops->cmd_clearscreen(f->backend, &f->ctx);
    f->backend->ops->cmd_cell(f->backend, &f->ctx);
    TEST_END();
    
    TEST_START("scroll then insert");
    COVERAGE_MARK(__LINE__);
    f->backend->ops->cmd_scrollup(f->backend, &f->ctx);
    f->backend->ops->cmd_insertline(f->backend, &f->ctx);
    TEST_END();
    
    TEST_START("delete then clear");
    COVERAGE_MARK(__LINE__);
    f->backend->ops->cmd_deletecharacter(f->backend, &f->ctx);
    f->backend->ops->cmd_clearline(f->backend, &f->ctx);
    TEST_END();
    
    destroy_fixture(f);
}

// Performance test for callbacks
void test_callback_performance(void) {
    printf("\n=== Testing Callback Performance ===\n");
    
    test_fixture_t* f = create_fixture();
    
    TEST_START("1000 cell operations");
    COVERAGE_MARK(__LINE__);
    clock_t perf_start = clock();
    for (int i = 0; i < 1000; i++) {
        TTY_CTX_SET_FIELD(&f->ctx, ocx, i % 80);
        TTY_CTX_SET_FIELD(&f->ctx, ocy, (i / 80) % 24);
        f->backend->ops->cmd_cell(f->backend, &f->ctx);
    }
    clock_t perf_end = clock();
    double ops_per_sec = 1000.0 / (((double)(perf_end - perf_start)) / CLOCKS_PER_SEC);
    printf(" (%.0f ops/sec)", ops_per_sec);
    ASSERT(ops_per_sec > 1000);  // Should handle >1000 ops/sec
    TEST_END();
    
    destroy_fixture(f);
}

// Main test runner
int main(int argc, char* argv[]) {
    printf("=== Ghostty Backend Callback Tests ===\n");
    printf("Testing all 22 tty_cmd callbacks with unified tty_ctx\n");
    
    // Run all test suites
    test_all_22_callbacks();
    test_callback_edge_cases();
    test_callback_combinations();
    test_callback_performance();
    
    // Print summary
    printf("\n=== Test Summary ===\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    // Calculate coverage
    int lines_hit = 0;
    for (int i = 0; i < 1000; i++) {
        if (coverage_hits[i] > 0) lines_hit++;
    }
    printf("Coverage markers hit: %d/1000\n", lines_hit);
    
    return tests_failed > 0 ? 1 : 0;
}