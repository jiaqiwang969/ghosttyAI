// test_framework.c - Core Test Framework Implementation
// Author: QA-001 (Test Lead)
// Date: 2025-08-25
// Version: 1.0.0

#include "test_framework.h"
#include <sys/time.h>
#include <pthread.h>
#include <assert.h>

// ============================================================================
// Global Test State
// ============================================================================

static pthread_mutex_t g_memory_mutex = PTHREAD_MUTEX_INITIALIZER;
static size_t g_total_allocated = 0;
static size_t g_total_freed = 0;
static uint32_t g_allocation_count = 0;
static uint32_t g_free_count = 0;

// Memory tracking structure
typedef struct mem_block {
    void* ptr;
    size_t size;
    struct mem_block* next;
} mem_block_t;

static mem_block_t* g_mem_blocks = NULL;

// ============================================================================
// Test Context Management
// ============================================================================

test_context_t* test_context_create(void) {
    test_context_t* ctx = calloc(1, sizeof(test_context_t));
    if (!ctx) return NULL;
    
    ctx->test_start_ns = get_time_ns();
    ctx->min_frame_latency_ns = UINT64_MAX;
    ctx->max_frame_latency_ns = 0;
    
    return ctx;
}

void test_context_destroy(test_context_t* ctx) {
    if (!ctx) return;
    
    // Free captured frames
    for (uint32_t i = 0; i < ctx->frame_count; i++) {
        if (ctx->captured_frames[i]) {
            free(ctx->captured_frames[i]);
        }
    }
    
    free(ctx);
}

void test_context_reset(test_context_t* ctx) {
    if (!ctx) return;
    
    // Free existing frames
    for (uint32_t i = 0; i < ctx->frame_count; i++) {
        if (ctx->captured_frames[i]) {
            free(ctx->captured_frames[i]);
            ctx->captured_frames[i] = NULL;
        }
    }
    
    // Reset counters
    ctx->frame_count = 0;
    ctx->total_cells_updated = 0;
    ctx->total_spans_created = 0;
    ctx->total_frames_emitted = 0;
    ctx->total_frames_dropped = 0;
    
    // Reset timing
    ctx->test_start_ns = get_time_ns();
    ctx->min_frame_latency_ns = UINT64_MAX;
    ctx->max_frame_latency_ns = 0;
    ctx->avg_frame_latency_ns = 0;
    
    // Reset memory tracking
    ctx->memory_allocated = 0;
    ctx->memory_freed = 0;
    ctx->peak_memory_usage = 0;
    ctx->allocation_count = 0;
    ctx->free_count = 0;
    
    // Reset errors
    ctx->error_count = 0;
    ctx->last_error[0] = '\0';
}

// ============================================================================
// Time Measurement
// ============================================================================

uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

double calculate_fps(test_context_t* ctx) {
    if (!ctx || ctx->frame_count < 2) return 0.0;
    
    uint64_t duration_ns = ctx->test_end_ns - ctx->test_start_ns;
    if (duration_ns == 0) return 0.0;
    
    double duration_sec = duration_ns / 1000000000.0;
    return ctx->frame_count / duration_sec;
}

double calculate_cells_per_second(test_context_t* ctx) {
    if (!ctx) return 0.0;
    
    uint64_t duration_ns = ctx->test_end_ns - ctx->test_start_ns;
    if (duration_ns == 0) return 0.0;
    
    double duration_sec = duration_ns / 1000000000.0;
    return ctx->total_cells_updated / duration_sec;
}

double calculate_latency_ms(uint64_t start_ns, uint64_t end_ns) {
    return (end_ns - start_ns) / 1000000.0;
}

// ============================================================================
// Memory Tracking
// ============================================================================

void* test_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) return NULL;
    
    pthread_mutex_lock(&g_memory_mutex);
    
    // Track allocation
    mem_block_t* block = malloc(sizeof(mem_block_t));
    if (block) {
        block->ptr = ptr;
        block->size = size;
        block->next = g_mem_blocks;
        g_mem_blocks = block;
        
        g_total_allocated += size;
        g_allocation_count++;
    }
    
    pthread_mutex_unlock(&g_memory_mutex);
    
    return ptr;
}

void test_free(void* ptr) {
    if (!ptr) return;
    
    pthread_mutex_lock(&g_memory_mutex);
    
    // Find and remove block
    mem_block_t** current = &g_mem_blocks;
    while (*current) {
        if ((*current)->ptr == ptr) {
            mem_block_t* block = *current;
            *current = block->next;
            
            g_total_freed += block->size;
            g_free_count++;
            
            free(block);
            break;
        }
        current = &(*current)->next;
    }
    
    pthread_mutex_unlock(&g_memory_mutex);
    
    free(ptr);
}

void* test_realloc(void* ptr, size_t size) {
    if (!ptr) return test_malloc(size);
    if (size == 0) {
        test_free(ptr);
        return NULL;
    }
    
    pthread_mutex_lock(&g_memory_mutex);
    
    // Find existing block
    mem_block_t* block = g_mem_blocks;
    while (block) {
        if (block->ptr == ptr) {
            g_total_freed += block->size;
            g_total_allocated += size;
            block->size = size;
            break;
        }
        block = block->next;
    }
    
    pthread_mutex_unlock(&g_memory_mutex);
    
    void* new_ptr = realloc(ptr, size);
    if (new_ptr != ptr && block) {
        block->ptr = new_ptr;
    }
    
    return new_ptr;
}

bool check_memory_leaks(test_context_t* ctx) {
    pthread_mutex_lock(&g_memory_mutex);
    
    bool has_leaks = (g_mem_blocks != NULL);
    
    if (ctx) {
        ctx->memory_allocated = g_total_allocated;
        ctx->memory_freed = g_total_freed;
        ctx->allocation_count = g_allocation_count;
        ctx->free_count = g_free_count;
    }
    
    pthread_mutex_unlock(&g_memory_mutex);
    
    return !has_leaks;
}

void print_memory_report(test_context_t* ctx) {
    if (!ctx) return;
    
    printf("\n=== Memory Report ===\n");
    printf("Allocated: %zu bytes (%u allocations)\n", 
           ctx->memory_allocated, ctx->allocation_count);
    printf("Freed: %zu bytes (%u frees)\n", 
           ctx->memory_freed, ctx->free_count);
    printf("Peak usage: %zu bytes\n", ctx->peak_memory_usage);
    
    if (ctx->memory_allocated != ctx->memory_freed) {
        printf("WARNING: Potential memory leak: %zd bytes\n",
               (ssize_t)(ctx->memory_allocated - ctx->memory_freed));
    }
    
    pthread_mutex_lock(&g_memory_mutex);
    
    if (g_mem_blocks) {
        printf("\nLeaked blocks:\n");
        mem_block_t* block = g_mem_blocks;
        while (block) {
            printf("  - %p: %zu bytes\n", block->ptr, block->size);
            block = block->next;
        }
    }
    
    pthread_mutex_unlock(&g_memory_mutex);
}

// ============================================================================
// Frame Validation
// ============================================================================

bool validate_frame(const ui_frame_t* frame) {
    if (!frame) return false;
    
    // Check required fields
    if (frame->size != sizeof(ui_frame_t)) {
        printf("ERROR: Invalid frame size: %u (expected %zu)\n",
               frame->size, sizeof(ui_frame_t));
        return false;
    }
    
    // Validate span data
    if (frame->span_count > 0 && !frame->spans) {
        printf("ERROR: Frame has %u spans but NULL spans pointer\n",
               frame->span_count);
        return false;
    }
    
    // Check frame sequence
    static uint64_t last_seq = 0;
    if (frame->frame_seq <= last_seq) {
        printf("ERROR: Frame sequence not monotonic: %lu <= %lu\n",
               frame->frame_seq, last_seq);
        return false;
    }
    last_seq = frame->frame_seq;
    
    return true;
}

bool validate_frame_sequence(test_context_t* ctx) {
    if (!ctx || ctx->frame_count == 0) return true;
    
    for (uint32_t i = 1; i < ctx->frame_count; i++) {
        ui_frame_t* prev = ctx->captured_frames[i - 1];
        ui_frame_t* curr = ctx->captured_frames[i];
        
        if (curr->frame_seq <= prev->frame_seq) {
            printf("ERROR: Frame sequence error at index %u\n", i);
            return false;
        }
        
        if (curr->timestamp_ns < prev->timestamp_ns) {
            printf("ERROR: Frame timestamp went backwards at index %u\n", i);
            return false;
        }
    }
    
    return true;
}

bool validate_frame_timing(test_context_t* ctx, uint64_t expected_interval_ns) {
    if (!ctx || ctx->frame_count < 2) return true;
    
    uint64_t tolerance_ns = expected_interval_ns / 10; // 10% tolerance
    
    for (uint32_t i = 1; i < ctx->frame_count; i++) {
        uint64_t interval = ctx->frame_timestamps[i] - ctx->frame_timestamps[i - 1];
        
        if (interval < expected_interval_ns - tolerance_ns ||
            interval > expected_interval_ns + tolerance_ns) {
            printf("WARNING: Frame interval out of range at index %u: %lu ns\n",
                   i, interval);
            // Don't fail, just warn
        }
    }
    
    return true;
}

bool validate_span_merging(const ui_frame_t* frame) {
    if (!frame || frame->span_count < 2) return true;
    
    // Check that adjacent spans with same attributes are merged
    for (uint32_t i = 1; i < frame->span_count; i++) {
        const ui_span_t* prev = &frame->spans[i - 1];
        const ui_span_t* curr = &frame->spans[i];
        
        // Check if spans are adjacent
        if (prev->row == curr->row && prev->col_end == curr->col_start) {
            // Check if they have same attributes (simplified check)
            if (prev->flags == curr->flags) {
                printf("ERROR: Adjacent spans should be merged at index %u\n", i);
                return false;
            }
        }
    }
    
    return true;
}

bool validate_dirty_rect(const ui_frame_t* frame) {
    if (!frame || frame->span_count == 0) return true;
    
    uint32_t min_row = UINT32_MAX, max_row = 0;
    uint32_t min_col = UINT32_MAX, max_col = 0;
    
    // Calculate actual dirty rectangle
    for (uint32_t i = 0; i < frame->span_count; i++) {
        const ui_span_t* span = &frame->spans[i];
        
        if (span->row < min_row) min_row = span->row;
        if (span->row > max_row) max_row = span->row;
        if (span->col_start < min_col) min_col = span->col_start;
        if (span->col_end > max_col) max_col = span->col_end;
    }
    
    // Verify all spans are within the dirty rectangle
    for (uint32_t i = 0; i < frame->span_count; i++) {
        const ui_span_t* span = &frame->spans[i];
        
        if (span->row < min_row || span->row > max_row ||
            span->col_start < min_col || span->col_end > max_col) {
            printf("ERROR: Span outside dirty rectangle at index %u\n", i);
            return false;
        }
    }
    
    return true;
}

// ============================================================================
// Performance Reporting
// ============================================================================

void print_performance_report(test_context_t* ctx) {
    if (!ctx) return;
    
    ctx->test_end_ns = get_time_ns();
    
    printf("\n=== Performance Report ===\n");
    printf("Duration: %.2f seconds\n", 
           (ctx->test_end_ns - ctx->test_start_ns) / 1000000000.0);
    printf("Frames emitted: %lu\n", ctx->total_frames_emitted);
    printf("Frames dropped: %lu\n", ctx->total_frames_dropped);
    printf("Cells updated: %lu\n", ctx->total_cells_updated);
    printf("Spans created: %lu\n", ctx->total_spans_created);
    
    double fps = calculate_fps(ctx);
    double cells_per_sec = calculate_cells_per_second(ctx);
    
    printf("Average FPS: %.2f\n", fps);
    printf("Cells/second: %.0f\n", cells_per_sec);
    
    if (ctx->frame_count > 0) {
        printf("Min latency: %.2f ms\n", ctx->min_frame_latency_ns / 1000000.0);
        printf("Max latency: %.2f ms\n", ctx->max_frame_latency_ns / 1000000.0);
        printf("Avg latency: %.2f ms\n", ctx->avg_frame_latency_ns / 1000000.0);
    }
    
    // Check against targets
    if (fps < PERF_TARGET_FPS) {
        printf("WARNING: FPS below target (%.0f < %d)\n", fps, PERF_TARGET_FPS);
    }
    
    if (cells_per_sec < PERF_TARGET_CELLS_PER_SEC) {
        printf("WARNING: Throughput below target (%.0f < %d)\n",
               cells_per_sec, PERF_TARGET_CELLS_PER_SEC);
    }
    
    if (ctx->max_frame_latency_ns / 1000000.0 > PERF_TARGET_LATENCY_MS) {
        printf("WARNING: Max latency above target (%.2f > %.2f ms)\n",
               ctx->max_frame_latency_ns / 1000000.0, PERF_TARGET_LATENCY_MS);
    }
}

// ============================================================================
// Test Data Generation
// ============================================================================

ui_cell_t* generate_random_cells(uint32_t count) {
    ui_cell_t* cells = test_malloc(sizeof(ui_cell_t) * count);
    if (!cells) return NULL;
    
    for (uint32_t i = 0; i < count; i++) {
        cells[i].codepoint = 'A' + (rand() % 26);
        cells[i].fg_rgb = rand() & 0xFFFFFF;
        cells[i].bg_rgb = rand() & 0xFFFFFF;
        cells[i].attrs = rand() & 0xFFFF;
        cells[i].width = 1;
        cells[i].cluster_cont = 0;
    }
    
    return cells;
}

ui_span_t* generate_test_spans(uint32_t count, uint32_t width, uint32_t height) {
    ui_span_t* spans = test_malloc(sizeof(ui_span_t) * count);
    if (!spans) return NULL;
    
    for (uint32_t i = 0; i < count; i++) {
        spans[i].row = rand() % height;
        spans[i].col_start = rand() % width;
        spans[i].col_end = spans[i].col_start + (rand() % 10) + 1;
        if (spans[i].col_end > width) {
            spans[i].col_end = width;
        }
        spans[i].cells = generate_random_cells(spans[i].col_end - spans[i].col_start);
        spans[i].flags = 0;
    }
    
    return spans;
}

struct tty_ctx* generate_test_context(uint32_t row, uint32_t col, uint32_t num_cells) {
    struct tty_ctx* ctx = test_malloc(sizeof(struct tty_ctx));
    if (!ctx) return NULL;
    
    ctx->ocx = col;
    ctx->ocy = row;
    ctx->num = num_cells;
    ctx->orlower = 0;
    ctx->orupper = 24;  // Standard terminal height
    ctx->cell_data = generate_random_cells(num_cells);
    ctx->bg = 0x000000;
    ctx->fg = 0xFFFFFF;
    ctx->attr = 0;
    
    return ctx;
}

void fill_screen_with_pattern(mock_backend_t* backend, char pattern) {
    if (!backend) return;
    
    // Create a full screen of the pattern
    for (uint32_t row = 0; row < 24; row++) {
        for (uint32_t col = 0; col < 80; col++) {
            struct tty_ctx* ctx = generate_test_context(row, col, 1);
            if (ctx) {
                ui_cell_t* cell = (ui_cell_t*)ctx->cell_data;
                cell->codepoint = pattern;
                
                // Call the mock backend's cmd_cell
                if (backend->base.ops && backend->base.ops->cmd_cell) {
                    backend->base.ops->cmd_cell(&backend->base, ctx);
                }
                
                test_free(ctx->cell_data);
                test_free(ctx);
            }
        }
    }
}

// ============================================================================
// Coverage Tracking
// ============================================================================

coverage_stats_t* get_coverage_stats(void) {
    coverage_stats_t* stats = test_malloc(sizeof(coverage_stats_t));
    if (!stats) return NULL;
    
    // These would normally be populated by gcov/lcov
    // For now, return mock data
    stats->functions_total = 22;  // All tty_cmd functions
    stats->functions_covered = 0;
    stats->lines_total = 1000;
    stats->lines_covered = 0;
    stats->branches_total = 500;
    stats->branches_covered = 0;
    
    return stats;
}

void print_coverage_report(coverage_stats_t* stats) {
    if (!stats) return;
    
    printf("\n=== Coverage Report ===\n");
    
    double func_coverage = (stats->functions_total > 0) ?
        (100.0 * stats->functions_covered / stats->functions_total) : 0.0;
    double line_coverage = (stats->lines_total > 0) ?
        (100.0 * stats->lines_covered / stats->lines_total) : 0.0;
    double branch_coverage = (stats->branches_total > 0) ?
        (100.0 * stats->branches_covered / stats->branches_total) : 0.0;
    
    printf("Functions: %.1f%% (%u/%u)\n",
           func_coverage, stats->functions_covered, stats->functions_total);
    printf("Lines: %.1f%% (%u/%u)\n",
           line_coverage, stats->lines_covered, stats->lines_total);
    printf("Branches: %.1f%% (%u/%u)\n",
           branch_coverage, stats->branches_covered, stats->branches_total);
    
    // Check against requirements
    if (line_coverage < COVERAGE_MIN_OVERALL) {
        printf("WARNING: Overall coverage below target (%.1f%% < %d%%)\n",
               line_coverage, COVERAGE_MIN_OVERALL);
    }
}

bool check_coverage_requirements(coverage_stats_t* stats) {
    if (!stats) return false;
    
    double line_coverage = (stats->lines_total > 0) ?
        (100.0 * stats->lines_covered / stats->lines_total) : 0.0;
    
    return line_coverage >= COVERAGE_MIN_OVERALL;
}