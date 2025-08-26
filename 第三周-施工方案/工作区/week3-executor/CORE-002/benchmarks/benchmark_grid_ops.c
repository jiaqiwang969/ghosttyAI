// benchmark_grid_ops.c - Performance Benchmarks for Grid Operations
// Purpose: Validate 10x performance improvement and P99 <0.3ms latency
// Author: CORE-002 (libtmux-core-developer)
// Date: 2025-08-26
// Task: T-202 - Grid Operations Batch Optimization

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include "../include/grid_callbacks.h"

// Benchmark configuration
#define GRID_WIDTH 200
#define GRID_HEIGHT 60
#define HISTORY_LIMIT 10000
#define WARMUP_ITERATIONS 1000
#define BENCHMARK_ITERATIONS 100000
#define BATCH_SIZES_COUNT 6
#define THREAD_COUNT 4

// Latency tracking
typedef struct {
    uint64_t* samples;
    size_t count;
    size_t capacity;
} latency_tracker_t;

// Benchmark results
typedef struct {
    double mean_latency_us;
    double p50_latency_us;
    double p95_latency_us;
    double p99_latency_us;
    double max_latency_us;
    double throughput_ops_per_sec;
    double speedup_factor;
    size_t memory_bytes_before;
    size_t memory_bytes_after;
    double memory_overhead_percent;
} benchmark_result_t;

// Test patterns for realistic workloads
typedef enum {
    PATTERN_SEQUENTIAL,    // Sequential writes (scrolling text)
    PATTERN_RANDOM,       // Random cell updates
    PATTERN_BLOCK,        // Block updates (window refresh)
    PATTERN_SCROLL,       // Scrolling operations
    PATTERN_MIXED         // Mixed workload
} test_pattern_t;

// ============================================================================
// Utility Functions
// ============================================================================

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static size_t get_memory_usage(void) {
    FILE* file = fopen("/proc/self/status", "r");
    if (!file) return 0;
    
    char line[256];
    size_t vmrss = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %zu kB", &vmrss);
            break;
        }
    }
    
    fclose(file);
    return vmrss * 1024;  // Convert to bytes
}

static latency_tracker_t* latency_tracker_create(size_t capacity) {
    latency_tracker_t* tracker = malloc(sizeof(latency_tracker_t));
    tracker->samples = calloc(capacity, sizeof(uint64_t));
    tracker->capacity = capacity;
    tracker->count = 0;
    return tracker;
}

static void latency_tracker_add(latency_tracker_t* tracker, uint64_t latency_ns) {
    if (tracker->count < tracker->capacity) {
        tracker->samples[tracker->count++] = latency_ns;
    }
}

static int compare_uint64(const void* a, const void* b) {
    uint64_t va = *(const uint64_t*)a;
    uint64_t vb = *(const uint64_t*)b;
    return (va > vb) - (va < vb);
}

static void calculate_percentiles(latency_tracker_t* tracker, benchmark_result_t* result) {
    if (tracker->count == 0) return;
    
    // Sort samples
    qsort(tracker->samples, tracker->count, sizeof(uint64_t), compare_uint64);
    
    // Calculate mean
    uint64_t sum = 0;
    for (size_t i = 0; i < tracker->count; i++) {
        sum += tracker->samples[i];
    }
    result->mean_latency_us = (double)sum / tracker->count / 1000.0;
    
    // Calculate percentiles
    size_t p50_idx = tracker->count * 50 / 100;
    size_t p95_idx = tracker->count * 95 / 100;
    size_t p99_idx = tracker->count * 99 / 100;
    
    result->p50_latency_us = tracker->samples[p50_idx] / 1000.0;
    result->p95_latency_us = tracker->samples[p95_idx] / 1000.0;
    result->p99_latency_us = tracker->samples[p99_idx] / 1000.0;
    result->max_latency_us = tracker->samples[tracker->count - 1] / 1000.0;
}

static void latency_tracker_destroy(latency_tracker_t* tracker) {
    free(tracker->samples);
    free(tracker);
}

// Generate test data
static struct grid_cell* generate_test_cells(size_t count) {
    struct grid_cell* cells = calloc(count, sizeof(struct grid_cell));
    
    const char* test_text = "The quick brown fox jumps over the lazy dog. ";
    size_t text_len = strlen(test_text);
    
    for (size_t i = 0; i < count; i++) {
        cells[i].data.data = test_text[i % text_len];
        cells[i].data.size = 1;
        cells[i].data.width = 1;
        cells[i].fg = 7;
        cells[i].bg = 0;
        cells[i].attr = (i % 10 == 0) ? GRID_ATTR_BRIGHT : 0;
        cells[i].flags = 0;
    }
    
    return cells;
}

// ============================================================================
// Single Cell Benchmark
// ============================================================================

static benchmark_result_t benchmark_single_cells(grid_router_t* router, 
                                                 test_pattern_t pattern,
                                                 size_t iterations) {
    printf("Benchmarking single cell operations (pattern: %d)...\n", pattern);
    
    latency_tracker_t* tracker = latency_tracker_create(iterations);
    struct grid_cell* test_cells = generate_test_cells(100);
    
    // Warmup
    for (size_t i = 0; i < WARMUP_ITERATIONS; i++) {
        uint32_t x = rand() % GRID_WIDTH;
        uint32_t y = rand() % GRID_HEIGHT;
        grid_router_set_cell(router, x, y, &test_cells[i % 100]);
    }
    
    size_t mem_before = get_memory_usage();
    uint64_t start_time = get_time_ns();
    
    // Benchmark
    for (size_t i = 0; i < iterations; i++) {
        uint32_t x, y;
        
        switch (pattern) {
            case PATTERN_SEQUENTIAL:
                x = i % GRID_WIDTH;
                y = (i / GRID_WIDTH) % GRID_HEIGHT;
                break;
            case PATTERN_RANDOM:
                x = rand() % GRID_WIDTH;
                y = rand() % GRID_HEIGHT;
                break;
            case PATTERN_BLOCK:
                x = (i % 10) + 10;
                y = (i % 10) + 10;
                break;
            default:
                x = i % GRID_WIDTH;
                y = i % GRID_HEIGHT;
        }
        
        uint64_t op_start = get_time_ns();
        grid_router_set_cell(router, x, y, &test_cells[i % 100]);
        uint64_t op_end = get_time_ns();
        
        latency_tracker_add(tracker, op_end - op_start);
    }
    
    uint64_t end_time = get_time_ns();
    size_t mem_after = get_memory_usage();
    
    benchmark_result_t result;
    calculate_percentiles(tracker, &result);
    
    double total_time_sec = (double)(end_time - start_time) / 1e9;
    result.throughput_ops_per_sec = iterations / total_time_sec;
    result.memory_bytes_before = mem_before;
    result.memory_bytes_after = mem_after;
    result.memory_overhead_percent = 
        ((double)(mem_after - mem_before) / mem_before) * 100.0;
    
    free(test_cells);
    latency_tracker_destroy(tracker);
    
    return result;
}

// ============================================================================
// Batch Operations Benchmark
// ============================================================================

static benchmark_result_t benchmark_batch_operations(grid_router_t* router,
                                                     test_pattern_t pattern,
                                                     size_t batch_size,
                                                     size_t iterations) {
    printf("Benchmarking batch operations (size: %zu, pattern: %d)...\n", 
           batch_size, pattern);
    
    latency_tracker_t* tracker = latency_tracker_create(iterations);
    struct grid_cell* test_cells = generate_test_cells(batch_size);
    
    // Warmup
    for (size_t i = 0; i < WARMUP_ITERATIONS / batch_size; i++) {
        grid_router_set_cells(router, 0, i % GRID_HEIGHT, test_cells, batch_size);
    }
    
    size_t mem_before = get_memory_usage();
    uint64_t start_time = get_time_ns();
    
    // Benchmark
    for (size_t i = 0; i < iterations; i++) {
        uint32_t x = 0, y = 0;
        
        switch (pattern) {
            case PATTERN_SEQUENTIAL:
                x = 0;
                y = i % GRID_HEIGHT;
                break;
            case PATTERN_RANDOM:
                x = rand() % (GRID_WIDTH - batch_size);
                y = rand() % GRID_HEIGHT;
                break;
            case PATTERN_BLOCK:
                x = (i * 10) % (GRID_WIDTH - batch_size);
                y = (i * 5) % GRID_HEIGHT;
                break;
            default:
                x = 0;
                y = i % GRID_HEIGHT;
        }
        
        uint64_t op_start = get_time_ns();
        
        // Use batch mode
        grid_router_batch_begin(router);
        grid_router_set_cells(router, x, y, test_cells, batch_size);
        grid_router_batch_end(router);
        
        uint64_t op_end = get_time_ns();
        
        latency_tracker_add(tracker, op_end - op_start);
    }
    
    uint64_t end_time = get_time_ns();
    size_t mem_after = get_memory_usage();
    
    benchmark_result_t result;
    calculate_percentiles(tracker, &result);
    
    double total_time_sec = (double)(end_time - start_time) / 1e9;
    result.throughput_ops_per_sec = (iterations * batch_size) / total_time_sec;
    result.memory_bytes_before = mem_before;
    result.memory_bytes_after = mem_after;
    result.memory_overhead_percent = 
        ((double)(mem_after - mem_before) / mem_before) * 100.0;
    
    free(test_cells);
    latency_tracker_destroy(tracker);
    
    return result;
}

// ============================================================================
// Mixed Workload Benchmark
// ============================================================================

static benchmark_result_t benchmark_mixed_workload(grid_router_t* router) {
    printf("Benchmarking mixed workload...\n");
    
    latency_tracker_t* tracker = latency_tracker_create(BENCHMARK_ITERATIONS);
    struct grid_cell* test_cells = generate_test_cells(1000);
    
    size_t mem_before = get_memory_usage();
    uint64_t start_time = get_time_ns();
    
    for (size_t i = 0; i < BENCHMARK_ITERATIONS; i++) {
        uint64_t op_start = get_time_ns();
        
        // Mix of operations
        switch (i % 5) {
            case 0:  // Single cell
                grid_router_set_cell(router, rand() % GRID_WIDTH, 
                                    rand() % GRID_HEIGHT, &test_cells[0]);
                break;
                
            case 1:  // Small batch
                grid_router_set_cells(router, rand() % (GRID_WIDTH - 10),
                                     rand() % GRID_HEIGHT, test_cells, 10);
                break;
                
            case 2:  // Clear region
                grid_router_clear_region(router, 
                                        rand() % (GRID_WIDTH - 20),
                                        rand() % (GRID_HEIGHT - 5),
                                        20, 5, 0);
                break;
                
            case 3:  // Scroll
                grid_router_scroll(router, 0, GRID_HEIGHT - 1, 1, 0);
                break;
                
            case 4:  // Large batch
                grid_router_batch_begin(router);
                grid_router_set_cells(router, 0, rand() % GRID_HEIGHT,
                                     test_cells, 100);
                grid_router_batch_end(router);
                break;
        }
        
        uint64_t op_end = get_time_ns();
        latency_tracker_add(tracker, op_end - op_start);
    }
    
    uint64_t end_time = get_time_ns();
    size_t mem_after = get_memory_usage();
    
    benchmark_result_t result;
    calculate_percentiles(tracker, &result);
    
    double total_time_sec = (double)(end_time - start_time) / 1e9;
    result.throughput_ops_per_sec = BENCHMARK_ITERATIONS / total_time_sec;
    result.memory_bytes_before = mem_before;
    result.memory_bytes_after = mem_after;
    result.memory_overhead_percent = 
        ((double)(mem_after - mem_before) / mem_before) * 100.0;
    
    free(test_cells);
    latency_tracker_destroy(tracker);
    
    return result;
}

// ============================================================================
// Thread Safety Benchmark
// ============================================================================

typedef struct {
    grid_router_t* router;
    size_t iterations;
    int thread_id;
    benchmark_result_t result;
} thread_args_t;

static void* thread_worker(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;
    
    latency_tracker_t* tracker = latency_tracker_create(args->iterations);
    struct grid_cell* test_cells = generate_test_cells(100);
    
    for (size_t i = 0; i < args->iterations; i++) {
        uint64_t op_start = get_time_ns();
        
        // Each thread works on different region
        uint32_t x = (args->thread_id * 50) % GRID_WIDTH;
        uint32_t y = rand() % GRID_HEIGHT;
        
        grid_router_set_cells(args->router, x, y, test_cells, 10);
        
        uint64_t op_end = get_time_ns();
        latency_tracker_add(tracker, op_end - op_start);
    }
    
    calculate_percentiles(tracker, &args->result);
    
    free(test_cells);
    latency_tracker_destroy(tracker);
    
    return NULL;
}

static benchmark_result_t benchmark_thread_safety(grid_router_t* router) {
    printf("Benchmarking thread safety (%d threads)...\n", THREAD_COUNT);
    
    pthread_t threads[THREAD_COUNT];
    thread_args_t args[THREAD_COUNT];
    
    size_t iterations_per_thread = BENCHMARK_ITERATIONS / THREAD_COUNT;
    
    uint64_t start_time = get_time_ns();
    
    // Start threads
    for (int i = 0; i < THREAD_COUNT; i++) {
        args[i].router = router;
        args[i].iterations = iterations_per_thread;
        args[i].thread_id = i;
        pthread_create(&threads[i], NULL, thread_worker, &args[i]);
    }
    
    // Wait for completion
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }
    
    uint64_t end_time = get_time_ns();
    
    // Aggregate results
    benchmark_result_t result = {0};
    for (int i = 0; i < THREAD_COUNT; i++) {
        result.mean_latency_us += args[i].result.mean_latency_us;
        if (args[i].result.p99_latency_us > result.p99_latency_us) {
            result.p99_latency_us = args[i].result.p99_latency_us;
        }
        if (args[i].result.max_latency_us > result.max_latency_us) {
            result.max_latency_us = args[i].result.max_latency_us;
        }
    }
    result.mean_latency_us /= THREAD_COUNT;
    
    double total_time_sec = (double)(end_time - start_time) / 1e9;
    result.throughput_ops_per_sec = BENCHMARK_ITERATIONS / total_time_sec;
    
    return result;
}

// ============================================================================
// Main Benchmark Runner
// ============================================================================

static void print_result(const char* test_name, benchmark_result_t* result) {
    printf("\n=== %s ===\n", test_name);
    printf("Throughput: %.2f ops/sec\n", result->throughput_ops_per_sec);
    printf("Latency (us):\n");
    printf("  Mean: %.2f\n", result->mean_latency_us);
    printf("  P50:  %.2f\n", result->p50_latency_us);
    printf("  P95:  %.2f\n", result->p95_latency_us);
    printf("  P99:  %.2f (Target: <300us)\n", result->p99_latency_us);
    printf("  Max:  %.2f\n", result->max_latency_us);
    
    if (result->memory_bytes_before > 0) {
        printf("Memory:\n");
        printf("  Before: %zu KB\n", result->memory_bytes_before / 1024);
        printf("  After:  %zu KB\n", result->memory_bytes_after / 1024);
        printf("  Overhead: %.1f%% (Target: <10%%)\n", result->memory_overhead_percent);
    }
    
    if (result->speedup_factor > 0) {
        printf("Speedup: %.2fx\n", result->speedup_factor);
    }
    
    // Check if targets are met
    bool p99_ok = result->p99_latency_us < 300.0;  // <0.3ms
    bool memory_ok = result->memory_overhead_percent < 10.0;
    
    printf("Status: %s\n", (p99_ok && memory_ok) ? "✓ PASS" : "✗ FAIL");
}

int main(int argc, char* argv[]) {
    printf("Grid Operations Performance Benchmark\n");
    printf("=====================================\n");
    printf("Configuration:\n");
    printf("  Grid: %dx%d\n", GRID_WIDTH, GRID_HEIGHT);
    printf("  History: %d lines\n", HISTORY_LIMIT);
    printf("  Iterations: %d\n", BENCHMARK_ITERATIONS);
    printf("\n");
    
    // Initialize router in batch mode
    grid_router_t* router = grid_router_init(GRID_MODE_BATCH, 
                                            GRID_WIDTH, GRID_HEIGHT,
                                            HISTORY_LIMIT);
    if (!router) {
        fprintf(stderr, "Failed to initialize grid router\n");
        return 1;
    }
    
    // Configure for optimal performance
    grid_router_set_batch_threshold(router, 10);
    grid_router_set_auto_batch(router, true);
    grid_router_enable_zero_copy(router, true);
    grid_router_enable_dirty_tracking(router, true);
    
    // Run benchmarks
    benchmark_result_t single_result = 
        benchmark_single_cells(router, PATTERN_RANDOM, BENCHMARK_ITERATIONS);
    print_result("Single Cell Operations", &single_result);
    
    // Batch sizes to test
    size_t batch_sizes[] = {10, 50, 100, 500, 1000, 5000};
    
    for (int i = 0; i < BATCH_SIZES_COUNT; i++) {
        char test_name[256];
        snprintf(test_name, sizeof(test_name), 
                "Batch Operations (size=%zu)", batch_sizes[i]);
        
        benchmark_result_t batch_result = 
            benchmark_batch_operations(router, PATTERN_SEQUENTIAL,
                                      batch_sizes[i], 
                                      BENCHMARK_ITERATIONS / batch_sizes[i]);
        
        // Calculate speedup
        batch_result.speedup_factor = 
            batch_result.throughput_ops_per_sec / single_result.throughput_ops_per_sec;
        
        print_result(test_name, &batch_result);
        
        // Check if we achieved 10x speedup
        if (batch_sizes[i] >= 100 && batch_result.speedup_factor >= 10.0) {
            printf("✓ Achieved 10x speedup target!\n");
        }
    }
    
    // Mixed workload
    benchmark_result_t mixed_result = benchmark_mixed_workload(router);
    print_result("Mixed Workload", &mixed_result);
    
    // Thread safety
    benchmark_result_t thread_result = benchmark_thread_safety(router);
    print_result("Multi-threaded Operations", &thread_result);
    
    // Get final statistics
    grid_stats_t stats;
    grid_router_get_stats(router, &stats);
    
    printf("\n=== Overall Statistics ===\n");
    printf("Total cells written: %lu\n", stats.cells_written);
    printf("Total cells cleared: %lu\n", stats.cells_cleared);
    printf("Batch operations: %lu\n", stats.batch_operations);
    printf("Single operations: %lu\n", stats.single_operations);
    printf("Dirty flushes: %lu\n", stats.dirty_flushes);
    printf("Average batch speedup: %.2fx\n", stats.batch_speedup);
    
    // Cleanup
    grid_router_destroy(router);
    
    printf("\n=== BENCHMARK COMPLETE ===\n");
    return 0;
}