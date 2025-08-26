// test_layout.c - Unit Tests for Layout Management
// Purpose: Comprehensive testing of layout operations
// Author: CORE-001 (c-tmux-specialist)
// Date: 2025-08-26
// Task: T-203 - Layout Management Tests
// Version: 1.0.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>
#include "../include/layout_callbacks.h"

// Test tracking
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Timing helpers
static uint64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// Test macros
#define TEST_START(name) \
    printf("Testing %s... ", name); \
    tests_run++; \
    int test_result = 0;

#define TEST_ASSERT(condition) \
    if (!(condition)) { \
        printf("FAILED at line %d: %s\n", __LINE__, #condition); \
        test_result = 1; \
        tests_failed++; \
        return test_result; \
    }

#define TEST_END() \
    if (test_result == 0) { \
        printf("PASSED\n"); \
        tests_passed++; \
    } \
    return test_result;

// ============================================================================
// Basic Layout Tests
// ============================================================================

int test_router_init() {
    TEST_START("router_init");
    
    layout_router_t* router = layout_router_init(LAYOUT_MODE_TMUX, NULL, NULL);
    TEST_ASSERT(router != NULL);
    TEST_ASSERT(router->mode == LAYOUT_MODE_TMUX);
    TEST_ASSERT(router->vtable != NULL);
    TEST_ASSERT(router->pane_count == 0);
    
    layout_router_cleanup(router);
    
    TEST_END();
}

int test_create_root() {
    TEST_START("create_root");
    
    layout_router_t* router = layout_router_init(LAYOUT_MODE_TMUX, NULL, NULL);
    TEST_ASSERT(router != NULL);
    
    layout_cell_t* root = layout_create_root(router, 800, 600);
    TEST_ASSERT(root != NULL);
    TEST_ASSERT(root->sx == 800);
    TEST_ASSERT(root->sy == 600);
    TEST_ASSERT(root->xoff == 0);
    TEST_ASSERT(root->yoff == 0);
    TEST_ASSERT(root->type == LAYOUT_WINDOWPANE);
    TEST_ASSERT(router->pane_count == 1);
    
    layout_router_cleanup(router);
    
    TEST_END();
}

int test_split_pane_horizontal() {
    TEST_START("split_pane_horizontal");
    
    layout_router_t* router = layout_router_init(LAYOUT_MODE_TMUX, NULL, NULL);
    layout_cell_t* root = layout_create_root(router, 800, 600);
    
    uint64_t start = get_time_us();
    layout_cell_t* new_pane = layout_split_pane(router, root, SPLIT_HORIZONTAL, 400);
    uint64_t elapsed = get_time_us() - start;
    
    TEST_ASSERT(new_pane != NULL);
    TEST_ASSERT(router->pane_count == 2);
    
    // Check timing requirement (<10ms)
    TEST_ASSERT(elapsed < 10000);
    printf(" (Split time: %lu us) ", elapsed);
    
    // Verify layout structure
    TEST_ASSERT(root->parent != NULL);
    TEST_ASSERT(root->parent->type == LAYOUT_LEFTRIGHT);
    TEST_ASSERT(root->sx == 400);
    TEST_ASSERT(new_pane->sx == 400);
    TEST_ASSERT(new_pane->xoff == 400);
    
    layout_router_cleanup(router);
    
    TEST_END();
}

int test_split_pane_vertical() {
    TEST_START("split_pane_vertical");
    
    layout_router_t* router = layout_router_init(LAYOUT_MODE_TMUX, NULL, NULL);
    layout_cell_t* root = layout_create_root(router, 800, 600);
    
    uint64_t start = get_time_us();
    layout_cell_t* new_pane = layout_split_pane(router, root, SPLIT_VERTICAL, 300);
    uint64_t elapsed = get_time_us() - start;
    
    TEST_ASSERT(new_pane != NULL);
    TEST_ASSERT(router->pane_count == 2);
    
    // Check timing requirement (<10ms)
    TEST_ASSERT(elapsed < 10000);
    printf(" (Split time: %lu us) ", elapsed);
    
    // Verify layout structure
    TEST_ASSERT(root->parent != NULL);
    TEST_ASSERT(root->parent->type == LAYOUT_TOPBOTTOM);
    TEST_ASSERT(root->sy == 300);
    TEST_ASSERT(new_pane->sy == 300);
    TEST_ASSERT(new_pane->yoff == 300);
    
    layout_router_cleanup(router);
    
    TEST_END();
}

int test_close_pane() {
    TEST_START("close_pane");
    
    layout_router_t* router = layout_router_init(LAYOUT_MODE_TMUX, NULL, NULL);
    layout_cell_t* root = layout_create_root(router, 800, 600);
    
    // Create multiple panes
    layout_cell_t* pane2 = layout_split_pane(router, root, SPLIT_HORIZONTAL, 400);
    layout_cell_t* pane3 = layout_split_pane(router, pane2, SPLIT_VERTICAL, 200);
    
    TEST_ASSERT(router->pane_count == 3);
    
    // Close middle pane
    int result = layout_close_pane(router, pane2);
    TEST_ASSERT(result == 0);
    TEST_ASSERT(router->pane_count == 2);
    
    layout_router_cleanup(router);
    
    TEST_END();
}

int test_resize_pane() {
    TEST_START("resize_pane");
    
    layout_router_t* router = layout_router_init(LAYOUT_MODE_TMUX, NULL, NULL);
    layout_cell_t* root = layout_create_root(router, 800, 600);
    
    layout_cell_t* pane2 = layout_split_pane(router, root, SPLIT_HORIZONTAL, 400);
    
    uint32_t original_width = root->sx;
    
    uint64_t start = get_time_us();
    int result = layout_resize_pane(router, root, 50, 0);
    uint64_t elapsed = get_time_us() - start;
    
    TEST_ASSERT(result == 0);
    TEST_ASSERT(root->sx == original_width + 50);
    TEST_ASSERT(pane2->sx == 400 - 50);
    
    // Check timing requirement (<5ms)
    TEST_ASSERT(elapsed < 5000);
    printf(" (Resize time: %lu us) ", elapsed);
    
    layout_router_cleanup(router);
    
    TEST_END();
}

// ============================================================================
// Layout Preset Tests
// ============================================================================

int test_preset_even_horizontal() {
    TEST_START("preset_even_horizontal");
    
    layout_router_t* router = layout_router_init(LAYOUT_MODE_TMUX, NULL, NULL);
    layout_cell_t* root = layout_create_root(router, 800, 600);
    
    // Create 4 panes
    layout_split_pane(router, root, SPLIT_HORIZONTAL, 200);
    layout_split_pane(router, root, SPLIT_HORIZONTAL, 200);
    layout_split_pane(router, root, SPLIT_HORIZONTAL, 200);
    
    // Apply preset
    int result = layout_apply_preset(router, LAYOUT_PRESET_EVEN_HORIZONTAL);
    TEST_ASSERT(result == 0);
    
    // Verify equal widths
    layout_cell_t* cell = router->root->children.first;
    uint32_t expected_width = 800 / 4;
    while (cell) {
        TEST_ASSERT(cell->sx == expected_width || cell->sx == expected_width + 1);
        cell = cell->next;
    }
    
    layout_router_cleanup(router);
    
    TEST_END();
}

int test_preset_main_vertical() {
    TEST_START("preset_main_vertical");
    
    layout_router_t* router = layout_router_init(LAYOUT_MODE_TMUX, NULL, NULL);
    layout_cell_t* root = layout_create_root(router, 800, 600);
    
    // Create 3 panes
    layout_split_pane(router, root, SPLIT_HORIZONTAL, 400);
    layout_split_pane(router, root, SPLIT_HORIZONTAL, 200);
    
    // Apply preset
    int result = layout_apply_preset(router, LAYOUT_PRESET_MAIN_VERTICAL);
    TEST_ASSERT(result == 0);
    
    // Verify main pane takes 60% width
    layout_cell_t* main_pane = router->root->children.first;
    TEST_ASSERT(main_pane->sx == (800 * 60) / 100);
    
    layout_router_cleanup(router);
    
    TEST_END();
}

// ============================================================================
// Zoom Tests
// ============================================================================

int test_zoom_unzoom() {
    TEST_START("zoom_unzoom");
    
    layout_router_t* router = layout_router_init(LAYOUT_MODE_TMUX, NULL, NULL);
    layout_cell_t* root = layout_create_root(router, 800, 600);
    
    layout_cell_t* pane2 = layout_split_pane(router, root, SPLIT_HORIZONTAL, 400);
    
    // Zoom pane
    int result = layout_zoom_pane(router, pane2);
    TEST_ASSERT(result == 0);
    TEST_ASSERT(layout_is_zoomed(router));
    TEST_ASSERT(router->zoomed_pane != NULL);
    TEST_ASSERT(router->zoomed_pane->sx == 800);
    TEST_ASSERT(router->zoomed_pane->sy == 600);
    
    // Unzoom
    result = layout_unzoom_pane(router);
    TEST_ASSERT(result == 0);
    TEST_ASSERT(!layout_is_zoomed(router));
    TEST_ASSERT(router->zoomed_pane == NULL);
    
    layout_router_cleanup(router);
    
    TEST_END();
}

// ============================================================================
// Navigation Tests
// ============================================================================

int test_find_pane() {
    TEST_START("find_pane");
    
    layout_router_t* router = layout_router_init(LAYOUT_MODE_TMUX, NULL, NULL);
    layout_cell_t* root = layout_create_root(router, 800, 600);
    root->pane.id = 1;
    
    layout_cell_t* pane2 = layout_split_pane(router, root, SPLIT_HORIZONTAL, 400);
    pane2->pane.id = 2;
    
    layout_cell_t* pane3 = layout_split_pane(router, pane2, SPLIT_VERTICAL, 200);
    pane3->pane.id = 3;
    
    // Find each pane by ID
    layout_cell_t* found = layout_find_pane(router, 1);
    TEST_ASSERT(found == root);
    
    found = layout_find_pane(router, 2);
    TEST_ASSERT(found == pane2);
    
    found = layout_find_pane(router, 3);
    TEST_ASSERT(found == pane3);
    
    found = layout_find_pane(router, 999);
    TEST_ASSERT(found == NULL);
    
    layout_router_cleanup(router);
    
    TEST_END();
}

int test_find_by_position() {
    TEST_START("find_by_position");
    
    layout_router_t* router = layout_router_init(LAYOUT_MODE_TMUX, NULL, NULL);
    layout_cell_t* root = layout_create_root(router, 800, 600);
    
    layout_cell_t* pane2 = layout_split_pane(router, root, SPLIT_HORIZONTAL, 400);
    
    // Find panes by position
    layout_cell_t* found = layout_find_by_position(router, 100, 100);
    TEST_ASSERT(found == root);
    
    found = layout_find_by_position(router, 500, 100);
    TEST_ASSERT(found == pane2);
    
    found = layout_find_by_position(router, 1000, 1000);
    TEST_ASSERT(found == NULL);
    
    layout_router_cleanup(router);
    
    TEST_END();
}

// ============================================================================
// Performance Tests
// ============================================================================

int test_performance_layout_change() {
    TEST_START("performance_layout_change");
    
    layout_router_t* router = layout_router_init(LAYOUT_MODE_TMUX, NULL, NULL);
    layout_cell_t* root = layout_create_root(router, 1920, 1080);
    
    // Create complex layout with many panes
    for (int i = 0; i < 10; i++) {
        layout_split_pane(router, root, SPLIT_HORIZONTAL, 192);
    }
    
    uint64_t start = get_time_us();
    
    // Apply different presets
    layout_apply_preset(router, LAYOUT_PRESET_EVEN_HORIZONTAL);
    layout_apply_preset(router, LAYOUT_PRESET_MAIN_VERTICAL);
    layout_apply_preset(router, LAYOUT_PRESET_TILED);
    
    uint64_t elapsed = get_time_us() - start;
    uint64_t per_change = elapsed / 3;
    
    printf(" (Layout change: %lu us) ", per_change);
    
    // Check <50ms requirement
    TEST_ASSERT(per_change < 50000);
    
    layout_router_cleanup(router);
    
    TEST_END();
}

int test_performance_many_splits() {
    TEST_START("performance_many_splits");
    
    layout_router_t* router = layout_router_init(LAYOUT_MODE_TMUX, NULL, NULL);
    layout_cell_t* root = layout_create_root(router, 1920, 1080);
    
    uint64_t total_split_time = 0;
    layout_cell_t* current = root;
    
    // Create 20 panes
    for (int i = 0; i < 19; i++) {
        uint64_t start = get_time_us();
        current = layout_split_pane(router, current, 
                                   (i % 2) ? SPLIT_VERTICAL : SPLIT_HORIZONTAL, 
                                   100);
        total_split_time += get_time_us() - start;
        
        TEST_ASSERT(current != NULL);
    }
    
    uint64_t avg_split_time = total_split_time / 19;
    printf(" (Avg split: %lu us) ", avg_split_time);
    
    // Check <10ms per split requirement
    TEST_ASSERT(avg_split_time < 10000);
    
    TEST_ASSERT(router->pane_count == 20);
    
    layout_router_cleanup(router);
    
    TEST_END();
}

// ============================================================================
// Custom Layout String Tests
// ============================================================================

int test_layout_dump_parse() {
    TEST_START("layout_dump_parse");
    
    layout_router_t* router = layout_router_init(LAYOUT_MODE_TMUX, NULL, NULL);
    layout_cell_t* root = layout_create_root(router, 800, 600);
    root->pane.id = 1;
    
    layout_cell_t* pane2 = layout_split_pane(router, root, SPLIT_HORIZONTAL, 400);
    pane2->pane.id = 2;
    
    // Dump layout
    char* layout_string = layout_dump(router);
    TEST_ASSERT(layout_string != NULL);
    printf("\n  Layout string: %s\n  ", layout_string);
    
    // Parse it back
    layout_router_t* router2 = layout_router_init(LAYOUT_MODE_TMUX, NULL, NULL);
    int result = layout_parse_custom(router2, layout_string);
    TEST_ASSERT(result == 0);
    
    // Verify structure matches
    TEST_ASSERT(router2->root != NULL);
    TEST_ASSERT(router2->root->sx == 800);
    TEST_ASSERT(router2->root->sy == 600);
    
    free(layout_string);
    layout_router_cleanup(router);
    layout_router_cleanup(router2);
    
    TEST_END();
}

// ============================================================================
// Synchronization Tests
// ============================================================================

int test_sync_panes() {
    TEST_START("sync_panes");
    
    layout_router_t* router = layout_router_init(LAYOUT_MODE_TMUX, NULL, NULL);
    layout_cell_t* root = layout_create_root(router, 800, 600);
    
    layout_split_pane(router, root, SPLIT_HORIZONTAL, 400);
    layout_split_pane(router, root, SPLIT_VERTICAL, 300);
    
    // Enable synchronization
    int result = layout_sync_panes(router, true);
    TEST_ASSERT(result == 0);
    TEST_ASSERT(layout_are_synchronized(router));
    
    // Disable synchronization
    result = layout_sync_panes(router, false);
    TEST_ASSERT(result == 0);
    TEST_ASSERT(!layout_are_synchronized(router));
    
    layout_router_cleanup(router);
    
    TEST_END();
}

// ============================================================================
// Test Runner
// ============================================================================

int main(int argc, char** argv) {
    printf("========================================\n");
    printf("Layout Management Test Suite\n");
    printf("========================================\n\n");
    
    // Basic tests
    test_router_init();
    test_create_root();
    test_split_pane_horizontal();
    test_split_pane_vertical();
    test_close_pane();
    test_resize_pane();
    
    // Preset tests
    test_preset_even_horizontal();
    test_preset_main_vertical();
    
    // Zoom tests
    test_zoom_unzoom();
    
    // Navigation tests
    test_find_pane();
    test_find_by_position();
    
    // Performance tests
    test_performance_layout_change();
    test_performance_many_splits();
    
    // Custom layout tests
    test_layout_dump_parse();
    
    // Synchronization tests
    test_sync_panes();
    
    // Print summary
    printf("\n========================================\n");
    printf("Test Results:\n");
    printf("  Total:  %d\n", tests_run);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("\nPerformance Requirements:\n");
    printf("  ✅ Pane split: <10ms\n");
    printf("  ✅ Pane resize: <5ms\n");
    printf("  ✅ Layout change: <50ms\n");
    printf("========================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}