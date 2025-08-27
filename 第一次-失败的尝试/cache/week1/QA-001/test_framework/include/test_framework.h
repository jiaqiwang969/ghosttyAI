// test_framework.h - Core Test Framework Header
// Author: QA-001 (Test Lead)
// Date: 2025-08-25
// Version: 1.0.0

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// Include the UI backend header
#include "ui_backend.h"

// Unity test framework
#include "unity.h"

// ============================================================================
// Test Configuration
// ============================================================================

#define TEST_MAX_FRAMES 1000
#define TEST_MAX_SPANS 10000
#define TEST_MAX_CELLS 1000000
#define TEST_FRAME_INTERVAL_NS 16666667  // 60 FPS
#define TEST_MAX_LATENCY_MS 20

// Coverage requirements
#define COVERAGE_MIN_OVERALL 65
#define COVERAGE_MIN_CRITICAL 80

// Performance targets
#define PERF_TARGET_CELLS_PER_SEC 10000000
#define PERF_TARGET_FPS 60
#define PERF_TARGET_LATENCY_MS 16.67

// ============================================================================
// Test Context Structure
// ============================================================================

typedef struct test_context {
    struct ui_backend* backend;
    ui_frame_t* captured_frames[TEST_MAX_FRAMES];
    uint32_t frame_count;
    uint64_t frame_timestamps[TEST_MAX_FRAMES];
    
    // Statistics
    uint64_t total_cells_updated;
    uint64_t total_spans_created;
    uint64_t total_frames_emitted;
    uint64_t total_frames_dropped;
    
    // Timing
    uint64_t test_start_ns;
    uint64_t test_end_ns;
    uint64_t min_frame_latency_ns;
    uint64_t max_frame_latency_ns;
    uint64_t avg_frame_latency_ns;
    
    // Memory tracking
    size_t memory_allocated;
    size_t memory_freed;
    size_t peak_memory_usage;
    uint32_t allocation_count;
    uint32_t free_count;
    
    // Error tracking
    uint32_t error_count;
    char last_error[256];
} test_context_t;

// ============================================================================
// Mock Backend Implementation
// ============================================================================

typedef struct mock_backend {
    struct ui_backend base;
    test_context_t* test_ctx;
    
    // Mock-specific tracking
    uint32_t cmd_cell_calls;
    uint32_t cmd_cells_calls;
    uint32_t cmd_clearline_calls;
    uint32_t cmd_clearscreen_calls;
    uint32_t cmd_scrollup_calls;
    uint32_t cmd_scrolldown_calls;
    uint32_t cmd_insertline_calls;
    uint32_t cmd_deleteline_calls;
    uint32_t cmd_insertcharacter_calls;
    uint32_t cmd_deletecharacter_calls;
    uint32_t cmd_clearcharacter_calls;
    uint32_t cmd_clearendofline_calls;
    uint32_t cmd_clearstartofline_calls;
    uint32_t cmd_clearendofscreen_calls;
    uint32_t cmd_clearstartofscreen_calls;
    uint32_t cmd_reverseindex_calls;
    uint32_t cmd_linefeed_calls;
    uint32_t cmd_alignmenttest_calls;
    uint32_t cmd_setselection_calls;
    uint32_t cmd_rawstring_calls;
    uint32_t cmd_sixelimage_calls;
    uint32_t cmd_syncstart_calls;
    
    // Validation flags
    bool validate_frame_order;
    bool validate_span_merging;
    bool validate_memory_usage;
    bool validate_performance;
} mock_backend_t;

// ============================================================================
// Test Framework Functions
// ============================================================================

// Initialize test context
test_context_t* test_context_create(void);
void test_context_destroy(test_context_t* ctx);
void test_context_reset(test_context_t* ctx);

// Create mock backend
mock_backend_t* mock_backend_create(test_context_t* ctx);
void mock_backend_destroy(mock_backend_t* backend);
void mock_backend_reset_counters(mock_backend_t* backend);

// Frame validation helpers
bool validate_frame(const ui_frame_t* frame);
bool validate_frame_sequence(test_context_t* ctx);
bool validate_frame_timing(test_context_t* ctx, uint64_t expected_interval_ns);
bool validate_span_merging(const ui_frame_t* frame);
bool validate_dirty_rect(const ui_frame_t* frame);

// Memory tracking helpers
void* test_malloc(size_t size);
void test_free(void* ptr);
void* test_realloc(void* ptr, size_t size);
bool check_memory_leaks(test_context_t* ctx);
void print_memory_report(test_context_t* ctx);

// Performance measurement helpers
uint64_t get_time_ns(void);
double calculate_fps(test_context_t* ctx);
double calculate_cells_per_second(test_context_t* ctx);
double calculate_latency_ms(uint64_t start_ns, uint64_t end_ns);
void print_performance_report(test_context_t* ctx);

// Test data generation
ui_cell_t* generate_random_cells(uint32_t count);
ui_span_t* generate_test_spans(uint32_t count, uint32_t width, uint32_t height);
struct tty_ctx* generate_test_context(uint32_t row, uint32_t col, uint32_t num_cells);
void fill_screen_with_pattern(mock_backend_t* backend, char pattern);

// Assertion helpers
#define TEST_ASSERT_FRAME_COUNT(ctx, expected) \
    TEST_ASSERT_EQUAL_UINT32(expected, (ctx)->frame_count)

#define TEST_ASSERT_NO_MEMORY_LEAKS(ctx) \
    TEST_ASSERT_TRUE_MESSAGE(check_memory_leaks(ctx), "Memory leak detected")

#define TEST_ASSERT_FPS_ABOVE(ctx, min_fps) \
    TEST_ASSERT_TRUE_MESSAGE(calculate_fps(ctx) >= min_fps, "FPS below target")

#define TEST_ASSERT_LATENCY_BELOW(ctx, max_ms) \
    TEST_ASSERT_TRUE_MESSAGE((ctx)->max_frame_latency_ns / 1000000.0 <= max_ms, "Latency too high")

#define TEST_ASSERT_CELLS_PER_SEC_ABOVE(ctx, min_cells) \
    TEST_ASSERT_TRUE_MESSAGE(calculate_cells_per_second(ctx) >= min_cells, "Throughput too low")

// ============================================================================
// Mock tty_ctx Structure
// ============================================================================

struct tty_ctx {
    uint32_t ocx, ocy;      // Cursor position
    uint32_t num;           // Number of cells/lines
    uint32_t orlower;       // Region lower bound
    uint32_t orupper;       // Region upper bound
    void* cell_data;        // Mock cell data
    uint32_t bg;            // Background color
    uint32_t fg;            // Foreground color
    uint32_t attr;          // Attributes
};

// ============================================================================
// Test Fixture Macros
// ============================================================================

#define TEST_SETUP() \
    test_context_t* ctx = test_context_create(); \
    mock_backend_t* backend = mock_backend_create(ctx); \
    TEST_ASSERT_NOT_NULL(ctx); \
    TEST_ASSERT_NOT_NULL(backend)

#define TEST_TEARDOWN() \
    if (backend) mock_backend_destroy(backend); \
    if (ctx) { \
        print_memory_report(ctx); \
        print_performance_report(ctx); \
        test_context_destroy(ctx); \
    }

// ============================================================================
// Coverage Tracking
// ============================================================================

typedef struct coverage_stats {
    uint32_t functions_total;
    uint32_t functions_covered;
    uint32_t lines_total;
    uint32_t lines_covered;
    uint32_t branches_total;
    uint32_t branches_covered;
} coverage_stats_t;

coverage_stats_t* get_coverage_stats(void);
void print_coverage_report(coverage_stats_t* stats);
bool check_coverage_requirements(coverage_stats_t* stats);

// ============================================================================
// Test Categories
// ============================================================================

// Unit test runners
void run_unit_tests_commands(void);
void run_unit_tests_frame_aggregation(void);
void run_unit_tests_memory(void);
void run_unit_tests_performance(void);

// Integration test runners
void run_integration_tests_vim(void);
void run_integration_tests_htop(void);
void run_integration_tests_tmux_nested(void);

// Stress test runners
void run_stress_tests_panes(void);
void run_stress_tests_scrolling(void);
void run_stress_tests_unicode(void);

// Performance benchmark runners
void run_benchmark_cell_updates(void);
void run_benchmark_frame_aggregation(void);
void run_benchmark_span_merging(void);
void run_benchmark_60fps(void);

#endif /* TEST_FRAMEWORK_H */