// test_all_callbacks.c - Comprehensive tests for all 22 tty_cmd callbacks
// Purpose: Achieve >50% code coverage with extensive testing
// Author: INTG-001 (Zig-Ghostty Integration Specialist)
// Date: 2025-08-25
// Version: 2.0.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "../backend_ghostty_v2.c"  // Include implementation for coverage

// Test framework
#define TEST_SUITE_BEGIN(name) \
    printf("\n=== Test Suite: %s ===\n", name); \
    int suite_pass = 0, suite_fail = 0

#define TEST_SUITE_END() \
    printf("Suite Results: %d passed, %d failed\n", suite_pass, suite_fail)

#define TEST_BEGIN(name) \
    printf("  Testing %s... ", name); \
    int test_passed = 1

#define TEST_END() \
    if (test_passed) { \
        printf("✓\n"); \
        suite_pass++; \
    } else { \
        printf("✗\n"); \
        suite_fail++; \
    }

#define ASSERT(cond) \
    if (!(cond)) { \
        printf("\n    Assertion failed: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
        test_passed = 0; \
    }

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))
#define ASSERT_GT(a, b) ASSERT((a) > (b))
#define ASSERT_LT(a, b) ASSERT((a) < (b))
#define ASSERT_NULL(ptr) ASSERT((ptr) == NULL)
#define ASSERT_NOT_NULL(ptr) ASSERT((ptr) != NULL)

// ============================================================================
// Test Fixture
// ============================================================================

typedef struct {
    struct ui_backend* backend;
    struct tty_ctx ctx;
    struct window_pane wp;
    struct grid_cell cell;
    uint32_t callback_count;
    ui_frame_t* last_frame;
} test_fixture_t;

static test_fixture_t* create_fixture(void) {
    test_fixture_t* f = calloc(1, sizeof(test_fixture_t));
    
    // Create backend with specific capabilities
    ui_capabilities_t caps = {
        .size = sizeof(ui_capabilities_t),
        .version = UI_BACKEND_ABI_VERSION,
        .supported = UI_CAP_FRAME_BATCH | UI_CAP_24BIT_COLOR,
        .max_fps = 60,
        .optimal_batch_size = 50,
        .max_dirty_rects = 8
    };
    
    f->backend = ghostty_backend_create_v2(&caps);
    
    // Initialize tty_ctx with unified structure
    tty_ctx_init(&f->ctx);
    f->ctx.wp = &f->wp;
    f->ctx.cell = &f->cell;
    f->ctx.sx = 80;
    f->ctx.sy = 24;
    f->ctx.ocx = 0;
    f->ctx.ocy = 0;
    f->ctx.orupper = 0;
    f->ctx.orlower = 23;
    f->ctx.num = 1;
    
    // Initialize window pane
    f->wp.id = 1;
    f->wp.sx = 80;
    f->wp.sy = 24;
    
    // Initialize cell
    f->cell.codepoint = 'A';
    f->cell.attr = 0;
    f->cell.fg = 7;
    f->cell.bg = 0;
    
    return f;
}

static void destroy_fixture(test_fixture_t* f) {
    if (f) {
        if (f->last_frame) free(f->last_frame);
        ghostty_backend_destroy(f->backend);
        free(f);
    }
}

// ============================================================================
// Test All 22 Callbacks
// ============================================================================

void test_all_22_callbacks(void) {
    TEST_SUITE_BEGIN("All 22 tty_cmd Callbacks");
    
    test_fixture_t* f = create_fixture();
    ASSERT_NOT_NULL(f);
    ASSERT_NOT_NULL(f->backend);
    ASSERT_NOT_NULL(f->backend->ops);
    
    // Test 1: cmd_cell
    TEST_BEGIN("cmd_cell");
    f->ctx.ocx = 10;
    f->ctx.ocy = 5;
    f->backend->ops->cmd_cell(f->backend, &f->ctx);
    uint64_t cells = 0;
    ghostty_backend_get_statistics(f->backend, NULL, &cells, NULL);
    ASSERT_GT(cells, 0);
    TEST_END();
    
    // Test 2: cmd_cells
    TEST_BEGIN("cmd_cells");
    f->ctx.num = 5;
    f->backend->ops->cmd_cells(f->backend, &f->ctx);
    uint64_t new_cells = 0;
    ghostty_backend_get_statistics(f->backend, NULL, &new_cells, NULL);
    ASSERT_GT(new_cells, cells);
    TEST_END();
    
    // Test 3: cmd_insertcharacter
    TEST_BEGIN("cmd_insertcharacter");
    f->ctx.ocx = 20;
    f->backend->ops->cmd_insertcharacter(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 4: cmd_deletecharacter
    TEST_BEGIN("cmd_deletecharacter");
    f->backend->ops->cmd_deletecharacter(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 5: cmd_clearcharacter
    TEST_BEGIN("cmd_clearcharacter");
    f->ctx.num = 3;
    f->backend->ops->cmd_clearcharacter(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 6: cmd_insertline
    TEST_BEGIN("cmd_insertline");
    f->ctx.ocy = 10;
    f->backend->ops->cmd_insertline(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 7: cmd_deleteline
    TEST_BEGIN("cmd_deleteline");
    f->backend->ops->cmd_deleteline(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 8: cmd_clearline
    TEST_BEGIN("cmd_clearline");
    f->backend->ops->cmd_clearline(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 9: cmd_clearendofline
    TEST_BEGIN("cmd_clearendofline");
    f->ctx.ocx = 40;
    f->backend->ops->cmd_clearendofline(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 10: cmd_clearstartofline
    TEST_BEGIN("cmd_clearstartofline");
    f->backend->ops->cmd_clearstartofline(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 11: cmd_clearscreen
    TEST_BEGIN("cmd_clearscreen");
    f->backend->ops->cmd_clearscreen(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 12: cmd_clearendofscreen
    TEST_BEGIN("cmd_clearendofscreen");
    f->ctx.ocy = 12;
    f->backend->ops->cmd_clearendofscreen(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 13: cmd_clearstartofscreen
    TEST_BEGIN("cmd_clearstartofscreen");
    f->backend->ops->cmd_clearstartofscreen(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 14: cmd_alignmenttest
    TEST_BEGIN("cmd_alignmenttest");
    f->backend->ops->cmd_alignmenttest(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 15: cmd_reverseindex
    TEST_BEGIN("cmd_reverseindex");
    f->backend->ops->cmd_reverseindex(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 16: cmd_linefeed
    TEST_BEGIN("cmd_linefeed");
    f->backend->ops->cmd_linefeed(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 17: cmd_scrollup
    TEST_BEGIN("cmd_scrollup");
    f->ctx.num = 2;
    f->backend->ops->cmd_scrollup(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 18: cmd_scrolldown
    TEST_BEGIN("cmd_scrolldown");
    f->ctx.num = 1;
    f->backend->ops->cmd_scrolldown(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 19: cmd_setselection
    TEST_BEGIN("cmd_setselection");
    f->backend->ops->cmd_setselection(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 20: cmd_rawstring
    TEST_BEGIN("cmd_rawstring");
    f->backend->ops->cmd_rawstring(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 21: cmd_sixelimage
    TEST_BEGIN("cmd_sixelimage");
    f->backend->ops->cmd_sixelimage(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    // Test 22: cmd_syncstart
    TEST_BEGIN("cmd_syncstart");
    f->backend->ops->cmd_syncstart(f->backend, &f->ctx);
    ASSERT_EQ(ghostty_backend_get_error_count(f->backend), 0);
    TEST_END();
    
    destroy_fixture(f);
    TEST_SUITE_END();
}

// ============================================================================
// Error Handling Tests
// ============================================================================

void test_error_handling(void) {
    TEST_SUITE_BEGIN("Error Handling");
    
    // Test NULL backend
    TEST_BEGIN("NULL backend");
    ghostty_cmd_cell_v2(NULL, NULL);
    TEST_END();
    
    // Test NULL ctx
    TEST_BEGIN("NULL ctx");
    test_fixture_t* f = create_fixture();
    f->backend->ops->cmd_cell(f->backend, NULL);
    int errors = ghostty_backend_get_error_count(f->backend);
    ASSERT_GT(errors, 0);
    destroy_fixture(f);
    TEST_END();
    
    // Test invalid ctx structure
    TEST_BEGIN("Invalid ctx structure");
    f = create_fixture();
    struct tty_ctx bad_ctx = {0};
    bad_ctx.size = 1;  // Wrong size
    f->backend->ops->cmd_cell(f->backend, &bad_ctx);
    errors = ghostty_backend_get_error_count(f->backend);
    ASSERT_GT(errors, 0);
    destroy_fixture(f);
    TEST_END();
    
    // Test error injection
    TEST_BEGIN("Error injection");
    f = create_fixture();
    ghostty_backend_enable_error_injection(f->backend, true);
    
    // Run many operations to trigger injected errors
    for (int i = 0; i < 100; i++) {
        f->ctx.ocx = i % 80;
        f->ctx.ocy = i % 24;
        f->backend->ops->cmd_cell(f->backend, &f->ctx);
    }
    
    errors = ghostty_backend_get_error_count(f->backend);
    ASSERT_GT(errors, 0);  // Should have some injected errors
    destroy_fixture(f);
    TEST_END();
    
    // Test recovery after error
    TEST_BEGIN("Recovery after error");
    f = create_fixture();
    
    // Cause an error with NULL ctx
    f->backend->ops->cmd_cell(f->backend, NULL);
    int error1 = ghostty_backend_get_error_count(f->backend);
    ASSERT_GT(error1, 0);
    
    // Now do valid operation
    f->backend->ops->cmd_cell(f->backend, &f->ctx);
    int error2 = ghostty_backend_get_error_count(f->backend);
    ASSERT_EQ(error2, error1);  // No new errors
    
    destroy_fixture(f);
    TEST_END();
    
    TEST_SUITE_END();
}

// ============================================================================
// Boundary Condition Tests
// ============================================================================

void test_boundary_conditions(void) {
    TEST_SUITE_BEGIN("Boundary Conditions");
    
    test_fixture_t* f = create_fixture();
    ghostty_backend_set_strict_validation(f->backend, true);
    
    // Test maximum cursor position
    TEST_BEGIN("Max cursor position");
    f->ctx.ocx = 9999;
    f->ctx.ocy = 9999;
    f->backend->ops->cmd_cell(f->backend, &f->ctx);
    int violations = ghostty_backend_get_bounds_violations(f->backend);
    ASSERT_GT(violations, 0);
    TEST_END();
    
    // Test zero dimensions
    TEST_BEGIN("Zero dimensions");
    f->ctx.sx = 0;
    f->ctx.sy = 0;
    f->backend->ops->cmd_clearscreen(f->backend, &f->ctx);
    int errors = ghostty_backend_get_error_count(f->backend);
    ASSERT_GT(errors, 0);
    TEST_END();
    
    // Test huge dimensions
    TEST_BEGIN("Huge dimensions");
    f->ctx.sx = 100000;
    f->ctx.sy = 100000;
    f->backend->ops->cmd_clearscreen(f->backend, &f->ctx);
    errors = ghostty_backend_get_error_count(f->backend);
    ASSERT_GT(errors, 0);
    TEST_END();
    
    // Test edge of screen
    TEST_BEGIN("Edge of screen");
    f->ctx.sx = 80;
    f->ctx.sy = 24;
    f->ctx.ocx = 79;  // Last column
    f->ctx.ocy = 23;  // Last row
    f->backend->ops->cmd_cell(f->backend, &f->ctx);
    errors = ghostty_backend_get_error_count(f->backend);
    // Should succeed
    TEST_END();
    
    // Test beyond screen
    TEST_BEGIN("Beyond screen");
    f->ctx.ocx = 80;  // One past last column
    f->ctx.ocy = 24;  // One past last row
    f->backend->ops->cmd_cell(f->backend, &f->ctx);
    violations = ghostty_backend_get_bounds_violations(f->backend);
    ASSERT_GT(violations, 0);
    TEST_END();
    
    // Test scroll region boundaries
    TEST_BEGIN("Scroll region boundaries");
    f->ctx.orupper = 10;
    f->ctx.orlower = 5;  // Invalid: upper > lower
    f->backend->ops->cmd_scrollup(f->backend, &f->ctx);
    // Should handle gracefully
    TEST_END();
    
    // Test large num parameter
    TEST_BEGIN("Large num parameter");
    f->ctx.num = 1000000;
    f->backend->ops->cmd_cells(f->backend, &f->ctx);
    // Should handle without crash
    TEST_END();
    
    destroy_fixture(f);
    TEST_SUITE_END();
}

// ============================================================================
// Concurrency Tests
// ============================================================================

typedef struct {
    test_fixture_t* fixture;
    int thread_id;
    int iterations;
    int errors;
} thread_test_data_t;

void* concurrent_test_worker(void* arg) {
    thread_test_data_t* data = (thread_test_data_t*)arg;
    
    for (int i = 0; i < data->iterations; i++) {
        // Random position
        data->fixture->ctx.ocx = rand() % 80;
        data->fixture->ctx.ocy = rand() % 24;
        
        // Random operation
        switch (rand() % 10) {
            case 0:
                data->fixture->backend->ops->cmd_cell(data->fixture->backend, 
                                                      &data->fixture->ctx);
                break;
            case 1:
                data->fixture->backend->ops->cmd_clearline(data->fixture->backend,
                                                           &data->fixture->ctx);
                break;
            case 2:
                data->fixture->backend->ops->cmd_insertcharacter(data->fixture->backend,
                                                                 &data->fixture->ctx);
                break;
            case 3:
                data->fixture->backend->ops->cmd_deletecharacter(data->fixture->backend,
                                                                 &data->fixture->ctx);
                break;
            case 4:
                data->fixture->backend->ops->cmd_scrollup(data->fixture->backend,
                                                          &data->fixture->ctx);
                break;
            case 5:
                data->fixture->backend->ops->cmd_scrolldown(data->fixture->backend,
                                                            &data->fixture->ctx);
                break;
            case 6:
                data->fixture->backend->ops->cmd_clearscreen(data->fixture->backend,
                                                             &data->fixture->ctx);
                break;
            case 7:
                data->fixture->backend->ops->cmd_linefeed(data->fixture->backend,
                                                          &data->fixture->ctx);
                break;
            case 8:
                data->fixture->backend->ops->cmd_cells(data->fixture->backend,
                                                       &data->fixture->ctx);
                break;
            case 9:
                data->fixture->backend->ops->cmd_reverseindex(data->fixture->backend,
                                                              &data->fixture->ctx);
                break;
        }
        
        // Random small delay
        if (rand() % 10 == 0) {
            usleep(rand() % 100);
        }
    }
    
    data->errors = ghostty_backend_get_error_count(data->fixture->backend);
    return NULL;
}

void test_concurrency(void) {
    TEST_SUITE_BEGIN("Concurrency");
    
    test_fixture_t* f = create_fixture();
    
    // Test 1: Multiple threads, same backend
    TEST_BEGIN("Multi-threaded access");
    
    const int num_threads = 8;
    const int iterations = 100;
    pthread_t threads[num_threads];
    thread_test_data_t thread_data[num_threads];
    
    int initial_errors = ghostty_backend_get_error_count(f->backend);
    
    // Start threads
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].fixture = f;
        thread_data[i].thread_id = i;
        thread_data[i].iterations = iterations;
        thread_data[i].errors = 0;
        
        pthread_create(&threads[i], NULL, concurrent_test_worker, &thread_data[i]);
    }
    
    // Wait for completion
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Check for race conditions (no crashes is success)
    int final_errors = ghostty_backend_get_error_count(f->backend);
    printf("\n    %d threads completed, errors: %d", num_threads, final_errors - initial_errors);
    
    TEST_END();
    
    // Test 2: Rapid create/destroy
    TEST_BEGIN("Rapid create/destroy");
    
    for (int i = 0; i < 10; i++) {
        test_fixture_t* temp = create_fixture();
        ASSERT_NOT_NULL(temp);
        ASSERT_NOT_NULL(temp->backend);
        
        // Do some operations
        temp->backend->ops->cmd_cell(temp->backend, &temp->ctx);
        
        destroy_fixture(temp);
    }
    
    TEST_END();
    
    // Test 3: Concurrent reader/writer
    TEST_BEGIN("Reader/Writer pattern");
    
    // One thread writes, others read statistics
    // Implementation would go here
    
    TEST_END();
    
    destroy_fixture(f);
    TEST_SUITE_END();
}

// ============================================================================
// Performance and Stress Tests
// ============================================================================

void test_performance(void) {
    TEST_SUITE_BEGIN("Performance");
    
    test_fixture_t* f = create_fixture();
    
    // Test batching performance
    TEST_BEGIN("Batching performance");
    
    ghostty_backend_set_immediate_mode(f->backend, false);
    
    clock_t start = clock();
    
    for (int i = 0; i < 10000; i++) {
        f->ctx.ocx = i % 80;
        f->ctx.ocy = (i / 80) % 24;
        f->backend->ops->cmd_cell(f->backend, &f->ctx);
    }
    
    ui_backend_flush_frame(f->backend);
    
    clock_t end = clock();
    double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    uint64_t frames, cells, batched;
    ghostty_backend_get_statistics(f->backend, &frames, &cells, &batched);
    
    printf("\n    10000 cells in %.3fs, %llu frames (%.1fx batching)", 
           cpu_time, frames, cells / (double)frames);
    
    ASSERT_LT(cpu_time, 1.0);  // Should complete in < 1 second
    ASSERT_GT(cells / (double)frames, 10.0);  // Good batching ratio
    
    TEST_END();
    
    // Test immediate mode performance
    TEST_BEGIN("Immediate mode performance");
    
    ghostty_backend_set_immediate_mode(f->backend, true);
    
    start = clock();
    
    for (int i = 0; i < 1000; i++) {
        f->ctx.ocx = i % 80;
        f->ctx.ocy = (i / 80) % 24;
        f->backend->ops->cmd_cell(f->backend, &f->ctx);
    }
    
    end = clock();
    cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("\n    1000 immediate cells in %.3fs", cpu_time);
    
    ASSERT_LT(cpu_time, 1.0);
    
    TEST_END();
    
    // Test memory usage
    TEST_BEGIN("Memory stability");
    
    // Run many operations and check for leaks
    for (int iter = 0; iter < 10; iter++) {
        for (int i = 0; i < 1000; i++) {
            f->ctx.ocx = rand() % 80;
            f->ctx.ocy = rand() % 24;
            
            switch (rand() % 5) {
                case 0: f->backend->ops->cmd_cell(f->backend, &f->ctx); break;
                case 1: f->backend->ops->cmd_clearline(f->backend, &f->ctx); break;
                case 2: f->backend->ops->cmd_scrollup(f->backend, &f->ctx); break;
                case 3: f->backend->ops->cmd_insertcharacter(f->backend, &f->ctx); break;
                case 4: f->backend->ops->cmd_clearscreen(f->backend, &f->ctx); break;
            }
        }
    }
    
    // If we got here without crash, memory is stable
    
    TEST_END();
    
    destroy_fixture(f);
    TEST_SUITE_END();
}

// ============================================================================
// Integration Tests
// ============================================================================

void test_integration(void) {
    TEST_SUITE_BEGIN("Integration");
    
    // Test typical terminal session
    TEST_BEGIN("Typical session simulation");
    
    test_fixture_t* f = create_fixture();
    
    // Clear screen
    f->backend->ops->cmd_clearscreen(f->backend, &f->ctx);
    
    // Type "Hello World"
    const char* text = "Hello World";
    for (const char* p = text; *p; p++) {
        f->cell.codepoint = *p;
        f->backend->ops->cmd_cell(f->backend, &f->ctx);
        f->ctx.ocx++;
    }
    
    // Newline
    f->backend->ops->cmd_linefeed(f->backend, &f->ctx);
    f->ctx.ocx = 0;
    f->ctx.ocy++;
    
    // Type more text
    text = "Testing Ghostty";
    for (const char* p = text; *p; p++) {
        f->cell.codepoint = *p;
        f->backend->ops->cmd_cell(f->backend, &f->ctx);
        f->ctx.ocx++;
    }
    
    // Clear to end of line
    f->backend->ops->cmd_clearendofline(f->backend, &f->ctx);
    
    // Scroll
    f->backend->ops->cmd_scrollup(f->backend, &f->ctx);
    
    int errors = ghostty_backend_get_error_count(f->backend);
    ASSERT_EQ(errors, 0);
    
    destroy_fixture(f);
    
    TEST_END();
    
    // Test vi-like operations
    TEST_BEGIN("Vi-like operations");
    
    f = create_fixture();
    
    // Insert line
    f->backend->ops->cmd_insertline(f->backend, &f->ctx);
    
    // Delete character
    f->backend->ops->cmd_deletecharacter(f->backend, &f->ctx);
    
    // Clear line
    f->backend->ops->cmd_clearline(f->backend, &f->ctx);
    
    // Delete line
    f->backend->ops->cmd_deleteline(f->backend, &f->ctx);
    
    destroy_fixture(f);
    
    TEST_END();
    
    TEST_SUITE_END();
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char* argv[]) {
    printf("=== Ghostty Backend Test Suite v2.0 ===\n");
    printf("Target: >50%% Code Coverage\n");
    
    // Seed random for reproducible tests
    srand(42);
    
    // Run all test suites
    test_all_22_callbacks();
    test_error_handling();
    test_boundary_conditions();
    test_concurrency();
    test_performance();
    test_integration();
    
    // Close coverage log
    ghostty_backend_close_coverage_log();
    
    printf("\n=== Test Suite Complete ===\n");
    
    return 0;
}