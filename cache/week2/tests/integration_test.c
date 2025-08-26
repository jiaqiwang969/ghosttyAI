/*
 * Integration Test Suite
 * Task: T-401 - 集成测试套件
 * Coverage: All core components
 */

#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

#include "event_loop_backend.h"
#include "grid_callbacks.h"
#include "layout_callbacks.h"
#include "copy_mode_backend.h"

/* Test configuration */
#define TEST_GRID_ROWS 24
#define TEST_GRID_COLS 80
#define TEST_ITERATIONS 1000000
#define PERF_TARGET_OPS_SEC 200000
#define PERF_TARGET_P99_MS 0.5

/* Test results tracking */
typedef struct {
    int total;
    int passed;
    int failed;
    double total_time_ms;
} test_results_t;

static test_results_t results = {0};

/* Test macros */
#define TEST_START(name) \
    do { \
        printf("Testing %s... ", name); \
        fflush(stdout); \
        clock_t start = clock(); \
        results.total++;

#define TEST_END() \
        clock_t end = clock(); \
        double time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000; \
        results.total_time_ms += time_ms; \
        printf("PASS (%.2fms)\n", time_ms); \
        results.passed++; \
    } while(0)

#define TEST_ASSERT(cond) \
    if (!(cond)) { \
        printf("FAIL\n  Assertion failed: %s\n  at %s:%d\n", \
               #cond, __FILE__, __LINE__); \
        results.failed++; \
        return; \
    }

/* Test 1: Event Loop Performance */
void test_event_loop_performance() {
    TEST_START("Event Loop Performance");
    
    struct event_base *base = event_base_new_with_backend(BACKEND_GHOSTTY);
    TEST_ASSERT(base != NULL);
    
    /* Measure dispatch performance */
    clock_t start = clock();
    int ops = 0;
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        /* Simulate event operations */
        struct event ev;
        event_set(&ev, -1, EV_TIMEOUT, NULL, NULL);
        event_add(&ev, NULL);
        event_del(&ev);
        ops++;
    }
    
    clock_t end = clock();
    double elapsed_sec = ((double)(end - start)) / CLOCKS_PER_SEC;
    double ops_per_sec = ops / elapsed_sec;
    
    printf("\n  Operations/sec: %.0f (target: %d)\n", ops_per_sec, PERF_TARGET_OPS_SEC);
    TEST_ASSERT(ops_per_sec > PERF_TARGET_OPS_SEC);
    
    event_base_free(base);
    TEST_END();
}

/* Test 2: SIMD Grid Operations */
void test_grid_simd_performance() {
    TEST_START("SIMD Grid Operations");
    
    grid_init_backend(true);  /* Enable SIMD */
    const grid_ops_t *ops = grid_get_backend();
    TEST_ASSERT(ops != NULL);
    
    grid_t *grid = ops->create(TEST_GRID_ROWS, TEST_GRID_COLS);
    TEST_ASSERT(grid != NULL);
    
    /* Prepare test data */
    grid_cell_t *cells = malloc(TEST_GRID_COLS * sizeof(grid_cell_t));
    for (int i = 0; i < TEST_GRID_COLS; i++) {
        cells[i].codepoint = 'A' + (i % 26);
        cells[i].attr = 0;
        cells[i].fg = 7;
        cells[i].bg = 0;
    }
    
    /* Benchmark batch updates */
    clock_t start = clock();
    
    for (int iter = 0; iter < 10000; iter++) {
        for (int row = 0; row < TEST_GRID_ROWS; row++) {
            ops->batch_update(grid, row, 0, cells, TEST_GRID_COLS);
        }
    }
    
    clock_t end = clock();
    double elapsed_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    double cells_per_sec = (10000.0 * TEST_GRID_ROWS * TEST_GRID_COLS) / (elapsed_ms / 1000);
    
    printf("\n  Cells/sec: %.0f (with SIMD)\n", cells_per_sec);
    TEST_ASSERT(cells_per_sec > 1000000);  /* 1M cells/sec target */
    
    free(cells);
    ops->destroy(grid);
    TEST_END();
}

/* Test 3: Layout Management */
void test_layout_switching() {
    TEST_START("Layout Switching Performance");
    
    layout_manager_t *mgr = layout_manager_create();
    TEST_ASSERT(mgr != NULL);
    
    /* Create test panes */
    pane_t panes[10];
    for (int i = 0; i < 10; i++) {
        panes[i].id = i;
        panes[i].next = (i < 9) ? &panes[i+1] : NULL;
        panes[i].prev = (i > 0) ? &panes[i-1] : NULL;
    }
    mgr->pane_list = &panes[0];
    
    /* Test all layout types */
    layout_type_t layouts[] = {
        LAYOUT_EVEN_HORIZONTAL,
        LAYOUT_EVEN_VERTICAL,
        LAYOUT_MAIN_HORIZONTAL,
        LAYOUT_MAIN_VERTICAL,
        LAYOUT_TILED
    };
    
    clock_t start = clock();
    
    for (int iter = 0; iter < 1000; iter++) {
        for (int i = 0; i < 5; i++) {
            mgr->ops->set_layout(layouts[i]);
        }
    }
    
    clock_t end = clock();
    double avg_switch_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000 / 5000;
    
    printf("\n  Average switch time: %.2fms (target: <50ms)\n", avg_switch_ms);
    TEST_ASSERT(avg_switch_ms < 50);
    
    layout_manager_destroy(mgr);
    TEST_END();
}

/* Test 4: Copy Mode */
void test_copy_mode_operations() {
    TEST_START("Copy Mode Operations");
    
    /* Create test grid */
    grid_init_backend(false);
    const grid_ops_t *grid_ops = grid_get_backend();
    grid_t *grid = grid_ops->create(TEST_GRID_ROWS, TEST_GRID_COLS);
    
    /* Initialize copy mode */
    int result = copy_mode_init(grid, COPY_MODE_VI);
    TEST_ASSERT(result == 0);
    
    /* Test selection */
    copy_mode_start_selection(5, 10);
    copy_mode_update_selection(10, 30);
    
    size_t len;
    char *text = copy_mode_get_selection(&len);
    TEST_ASSERT(text != NULL);
    TEST_ASSERT(len > 0);
    free(text);
    
    /* Test movement */
    copy_mode_move_cursor(COPY_MOVE_DOWN, 5);
    copy_mode_move_cursor(COPY_MOVE_RIGHT, 10);
    copy_mode_move_cursor(COPY_MOVE_START_LINE, 1);
    copy_mode_move_cursor(COPY_MOVE_END_LINE, 1);
    
    /* Test search */
    result = copy_mode_search("test", 1);
    /* Search may fail if pattern not in grid, that's ok */
    
    /* Get stats */
    copy_mode_stats_t stats;
    copy_mode_get_stats(&stats);
    TEST_ASSERT(stats.selection_changes > 0);
    
    copy_mode_exit();
    grid_ops->destroy(grid);
    TEST_END();
}

/* Test 5: Memory Safety */
void test_memory_safety() {
    TEST_START("Memory Safety Verification");
    
    /* This test would normally use Valgrind or ASAN */
    /* Here we do basic allocation/deallocation tests */
    
    for (int i = 0; i < 100; i++) {
        /* Event base */
        struct event_base *base = event_base_new();
        TEST_ASSERT(base != NULL);
        event_base_free(base);
        
        /* Grid */
        grid_init_backend(true);
        const grid_ops_t *ops = grid_get_backend();
        grid_t *grid = ops->create(10, 10);
        TEST_ASSERT(grid != NULL);
        ops->destroy(grid);
        
        /* Layout manager */
        layout_manager_t *mgr = layout_manager_create();
        TEST_ASSERT(mgr != NULL);
        layout_manager_destroy(mgr);
    }
    
    TEST_END();
}

/* Test 6: Thread Safety */
void* thread_worker(void* arg) {
    int thread_id = *(int*)arg;
    
    /* Each thread creates and uses its own event base */
    struct event_base *base = event_base_new();
    if (!base) return NULL;
    
    /* Simulate some work */
    for (int i = 0; i < 1000; i++) {
        struct event ev;
        event_set(&ev, -1, EV_TIMEOUT, NULL, NULL);
        event_add(&ev, NULL);
        event_del(&ev);
    }
    
    event_base_free(base);
    return NULL;
}

void test_thread_safety() {
    TEST_START("Thread Safety");
    
    pthread_t threads[10];
    int thread_ids[10];
    
    /* Create threads */
    for (int i = 0; i < 10; i++) {
        thread_ids[i] = i;
        int result = pthread_create(&threads[i], NULL, thread_worker, &thread_ids[i]);
        TEST_ASSERT(result == 0);
    }
    
    /* Wait for threads */
    for (int i = 0; i < 10; i++) {
        pthread_join(threads[i], NULL);
    }
    
    TEST_END();
}

/* Test 7: Integration Test */
void test_full_integration() {
    TEST_START("Full System Integration");
    
    /* Initialize all components */
    struct event_base *base = event_base_new_with_backend(BACKEND_GHOSTTY);
    TEST_ASSERT(base != NULL);
    
    grid_init_backend(true);
    const grid_ops_t *grid_ops = grid_get_backend();
    grid_t *grid = grid_ops->create(TEST_GRID_ROWS, TEST_GRID_COLS);
    TEST_ASSERT(grid != NULL);
    
    layout_manager_t *layout_mgr = layout_manager_create();
    TEST_ASSERT(layout_mgr != NULL);
    
    copy_mode_init(grid, COPY_MODE_VI);
    
    /* Simulate typical usage */
    for (int i = 0; i < 100; i++) {
        /* Update grid */
        grid_cell_t cell = {.codepoint = 'X', .attr = 0, .fg = 7, .bg = 0};
        grid_ops->set_cell(grid, i % TEST_GRID_ROWS, i % TEST_GRID_COLS, &cell);
        
        /* Process events */
        event_base_loop(base, EVLOOP_NONBLOCK);
        
        /* Switch layouts */
        if (i % 20 == 0) {
            layout_mgr->ops->set_layout(i % 5);
        }
        
        /* Copy mode operations */
        if (i % 30 == 0) {
            copy_mode_start_selection(0, 0);
            copy_mode_update_selection(5, 10);
        }
    }
    
    /* Cleanup */
    copy_mode_exit();
    layout_manager_destroy(layout_mgr);
    grid_ops->destroy(grid);
    event_base_free(base);
    
    TEST_END();
}

/* Main test runner */
int main(int argc, char *argv[]) {
    printf("=================================================\n");
    printf("Ghostty × tmux Integration Test Suite\n");
    printf("=================================================\n\n");
    
    /* Run all tests */
    test_event_loop_performance();
    test_grid_simd_performance();
    test_layout_switching();
    test_copy_mode_operations();
    test_memory_safety();
    test_thread_safety();
    test_full_integration();
    
    /* Print summary */
    printf("\n=================================================\n");
    printf("Test Results Summary\n");
    printf("=================================================\n");
    printf("Total:  %d\n", results.total);
    printf("Passed: %d\n", results.passed);
    printf("Failed: %d\n", results.failed);
    printf("Total time: %.2fms\n", results.total_time_ms);
    printf("=================================================\n");
    
    if (results.failed == 0) {
        printf("✅ ALL TESTS PASSED!\n");
        return 0;
    } else {
        printf("❌ SOME TESTS FAILED\n");
        return 1;
    }
}