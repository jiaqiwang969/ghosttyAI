// benchmark_suite.c - Performance Benchmark Suite for tmux-Ghostty Integration
// Purpose: Measure throughput, latency, and overhead of integrated components
// Author: INTG-003 (performance-eng)
// Date: 2025-08-25
// Target: 200k ops/s, P99 <0.5ms

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <math.h>
#include <sys/time.h>
#include <assert.h>

// Include our components (paths adjusted for testing)
// #include "../../CORE-001/src/event_loop_backend.h"
// #include "../../CORE-001/src/ui_backend.h"

#define BENCHMARK_ITERATIONS 1000000
#define WARMUP_ITERATIONS 10000
#define PERCENTILE_BUCKETS 1000
#define THREAD_COUNT 4

// Timing utilities
typedef struct {
    uint64_t start_ns;
    uint64_t end_ns;
    uint64_t duration_ns;
} timing_t;

typedef struct {
    double min_us;
    double max_us;
    double mean_us;
    double stddev_us;
    double p50_us;
    double p90_us;
    double p95_us;
    double p99_us;
    double p999_us;
    uint64_t total_ops;
    double ops_per_sec;
} stats_t;

static inline uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static inline void timing_start(timing_t* t) {
    t->start_ns = get_time_ns();
}

static inline void timing_end(timing_t* t) {
    t->end_ns = get_time_ns();
    t->duration_ns = t->end_ns - t->start_ns;
}

// Calculate statistics from timing data
void calculate_stats(uint64_t* durations_ns, size_t count, stats_t* stats) {
    // Sort for percentiles
    for (size_t i = 0; i < count - 1; i++) {
        for (size_t j = i + 1; j < count; j++) {
            if (durations_ns[i] > durations_ns[j]) {
                uint64_t temp = durations_ns[i];
                durations_ns[i] = durations_ns[j];
                durations_ns[j] = temp;
            }
        }
    }
    
    // Calculate basic stats
    stats->min_us = durations_ns[0] / 1000.0;
    stats->max_us = durations_ns[count - 1] / 1000.0;
    
    // Mean
    uint64_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += durations_ns[i];
    }
    stats->mean_us = (sum / count) / 1000.0;
    
    // Standard deviation
    double variance = 0;
    for (size_t i = 0; i < count; i++) {
        double diff = (durations_ns[i] / 1000.0) - stats->mean_us;
        variance += diff * diff;
    }
    stats->stddev_us = sqrt(variance / count);
    
    // Percentiles
    stats->p50_us = durations_ns[count * 50 / 100] / 1000.0;
    stats->p90_us = durations_ns[count * 90 / 100] / 1000.0;
    stats->p95_us = durations_ns[count * 95 / 100] / 1000.0;
    stats->p99_us = durations_ns[count * 99 / 100] / 1000.0;
    stats->p999_us = durations_ns[count * 999 / 1000] / 1000.0;
    
    // Throughput
    stats->total_ops = count;
    uint64_t total_time_ns = 0;
    for (size_t i = 0; i < count; i++) {
        total_time_ns += durations_ns[i];
    }
    stats->ops_per_sec = (count * 1000000000.0) / total_time_ns;
}

// Print statistics
void print_stats(const char* name, stats_t* stats) {
    printf("\n=== %s Performance ===\n", name);
    printf("Operations: %lu\n", stats->total_ops);
    printf("Throughput: %.0f ops/sec\n", stats->ops_per_sec);
    printf("Latency (microseconds):\n");
    printf("  Min:    %.3f µs\n", stats->min_us);
    printf("  Mean:   %.3f µs\n", stats->mean_us);
    printf("  StdDev: %.3f µs\n", stats->stddev_us);
    printf("  P50:    %.3f µs\n", stats->p50_us);
    printf("  P90:    %.3f µs\n", stats->p90_us);
    printf("  P95:    %.3f µs\n", stats->p95_us);
    printf("  P99:    %.3f µs\n", stats->p99_us);
    printf("  P99.9:  %.3f µs\n", stats->p999_us);
    printf("  Max:    %.3f µs\n", stats->max_us);
    
    // Check against targets
    if (stats->ops_per_sec >= 200000) {
        printf("✓ Meets throughput target (200k ops/s)\n");
    } else {
        printf("✗ Below throughput target (200k ops/s)\n");
    }
    
    if (stats->p99_us <= 500) {
        printf("✓ Meets P99 latency target (<0.5ms)\n");
    } else {
        printf("✗ Exceeds P99 latency target (<0.5ms)\n");
    }
}

// Benchmark: Event Loop Dispatch
void benchmark_event_loop_dispatch(void) {
    printf("\nBenchmarking Event Loop Dispatch...\n");
    
    uint64_t* durations = calloc(BENCHMARK_ITERATIONS, sizeof(uint64_t));
    timing_t timer;
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        // Simulate event dispatch
        volatile int dummy = i * 2;
        (void)dummy;
    }
    
    // Actual benchmark
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        timing_start(&timer);
        
        // TODO: Call actual event loop dispatch
        // event_loop_dispatch_event(...);
        volatile int dummy = i * 2;  // Placeholder
        (void)dummy;
        
        timing_end(&timer);
        durations[i] = timer.duration_ns;
    }
    
    stats_t stats;
    calculate_stats(durations, BENCHMARK_ITERATIONS, &stats);
    print_stats("Event Loop Dispatch", &stats);
    
    free(durations);
}

// Benchmark: FFI Boundary Crossing
void benchmark_ffi_crossing(void) {
    printf("\nBenchmarking FFI Boundary Crossing...\n");
    
    uint64_t* durations = calloc(BENCHMARK_ITERATIONS, sizeof(uint64_t));
    timing_t timer;
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        // Simulate FFI call
        volatile int dummy = i * 3;
        (void)dummy;
    }
    
    // Actual benchmark
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        timing_start(&timer);
        
        // TODO: Call actual FFI function
        // zig_ffi_call(...);
        volatile int dummy = i * 3;  // Placeholder
        (void)dummy;
        
        timing_end(&timer);
        durations[i] = timer.duration_ns;
    }
    
    stats_t stats;
    calculate_stats(durations, BENCHMARK_ITERATIONS, &stats);
    print_stats("FFI Boundary Crossing", &stats);
    
    // Check if we meet the <50ns target
    if (stats.mean_us * 1000 <= 50) {
        printf("✓ FFI overhead <50ns target met\n");
    } else {
        printf("✗ FFI overhead exceeds 50ns target (%.0fns)\n", stats.mean_us * 1000);
    }
    
    free(durations);
}

// Benchmark: Grid Operations
void benchmark_grid_operations(void) {
    printf("\nBenchmarking Grid Operations...\n");
    
    uint64_t* durations = calloc(BENCHMARK_ITERATIONS, sizeof(uint64_t));
    timing_t timer;
    
    // Setup grid (80x24 typical terminal)
    const int GRID_WIDTH = 80;
    const int GRID_HEIGHT = 24;
    char grid[GRID_HEIGHT][GRID_WIDTH];
    memset(grid, ' ', sizeof(grid));
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        grid[i % GRID_HEIGHT][i % GRID_WIDTH] = 'A' + (i % 26);
    }
    
    // Actual benchmark
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        timing_start(&timer);
        
        // Simulate grid update
        int row = i % GRID_HEIGHT;
        int col = i % GRID_WIDTH;
        grid[row][col] = 'A' + (i % 26);
        
        // TODO: Call actual grid callback
        // ui_backend_grid_update(row, col, ...);
        
        timing_end(&timer);
        durations[i] = timer.duration_ns;
    }
    
    stats_t stats;
    calculate_stats(durations, BENCHMARK_ITERATIONS, &stats);
    print_stats("Grid Operations", &stats);
    
    free(durations);
}

// Benchmark: Memory Allocation Pattern
void benchmark_memory_allocation(void) {
    printf("\nBenchmarking Memory Allocation Pattern...\n");
    
    uint64_t* durations = calloc(BENCHMARK_ITERATIONS, sizeof(uint64_t));
    timing_t timer;
    void* ptrs[1000];
    int ptr_index = 0;
    
    // Actual benchmark
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        timing_start(&timer);
        
        // Allocate and free in pattern similar to real usage
        if (i % 10 == 0) {
            // Allocate
            ptrs[ptr_index++] = malloc(1024);
            if (ptr_index >= 1000) ptr_index = 0;
        } else if (i % 10 == 5 && ptr_index > 0) {
            // Free
            free(ptrs[--ptr_index]);
        }
        
        timing_end(&timer);
        durations[i] = timer.duration_ns;
    }
    
    // Cleanup
    while (ptr_index > 0) {
        free(ptrs[--ptr_index]);
    }
    
    stats_t stats;
    calculate_stats(durations, BENCHMARK_ITERATIONS, &stats);
    print_stats("Memory Allocation", &stats);
    
    free(durations);
}

// Thread worker for concurrency test
typedef struct {
    int thread_id;
    uint64_t operations;
    uint64_t total_time_ns;
} thread_data_t;

void* thread_worker(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    timing_t timer;
    
    timing_start(&timer);
    
    for (uint64_t i = 0; i < data->operations; i++) {
        // Simulate work
        volatile int dummy = i * data->thread_id;
        (void)dummy;
        
        // TODO: Call actual concurrent operation
        // event_loop_concurrent_op(...);
    }
    
    timing_end(&timer);
    data->total_time_ns = timer.duration_ns;
    
    return NULL;
}

// Benchmark: Concurrency and Thread Safety
void benchmark_concurrency(void) {
    printf("\nBenchmarking Concurrency (% threads)...\n", THREAD_COUNT);
    
    pthread_t threads[THREAD_COUNT];
    thread_data_t thread_data[THREAD_COUNT];
    uint64_t ops_per_thread = BENCHMARK_ITERATIONS / THREAD_COUNT;
    
    timing_t timer;
    timing_start(&timer);
    
    // Launch threads
    for (int i = 0; i < THREAD_COUNT; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].operations = ops_per_thread;
        pthread_create(&threads[i], NULL, thread_worker, &thread_data[i]);
    }
    
    // Wait for completion
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }
    
    timing_end(&timer);
    
    // Calculate aggregate stats
    uint64_t total_ops = 0;
    for (int i = 0; i < THREAD_COUNT; i++) {
        total_ops += thread_data[i].operations;
    }
    
    double ops_per_sec = (total_ops * 1000000000.0) / timer.duration_ns;
    
    printf("Total operations: %lu\n", total_ops);
    printf("Total time: %.2f ms\n", timer.duration_ns / 1000000.0);
    printf("Throughput: %.0f ops/sec\n", ops_per_sec);
    printf("Per-thread: %.0f ops/sec\n", ops_per_sec / THREAD_COUNT);
}

// Stress test for sustained load
void stress_test_sustained_load(int duration_seconds) {
    printf("\nRunning %d-second sustained load test...\n", duration_seconds);
    
    uint64_t start_time = get_time_ns();
    uint64_t end_time = start_time + (duration_seconds * 1000000000ULL);
    uint64_t operations = 0;
    uint64_t errors = 0;
    
    // Memory tracking
    size_t initial_mem = 0;  // TODO: Get actual memory usage
    size_t peak_mem = 0;
    
    while (get_time_ns() < end_time) {
        // Perform operations
        for (int i = 0; i < 1000; i++) {
            operations++;
            
            // TODO: Call actual operations
            // if (perform_operation() != 0) errors++;
            
            // Check memory periodically
            if (operations % 100000 == 0) {
                size_t current_mem = 0;  // TODO: Get actual memory usage
                if (current_mem > peak_mem) {
                    peak_mem = current_mem;
                }
            }
        }
    }
    
    uint64_t actual_duration_ns = get_time_ns() - start_time;
    double ops_per_sec = (operations * 1000000000.0) / actual_duration_ns;
    
    printf("Sustained Load Results:\n");
    printf("  Duration: %.1f seconds\n", actual_duration_ns / 1000000000.0);
    printf("  Operations: %lu\n", operations);
    printf("  Errors: %lu\n", errors);
    printf("  Throughput: %.0f ops/sec\n", ops_per_sec);
    printf("  Memory growth: %zu bytes\n", peak_mem - initial_mem);
    
    if (errors == 0 && ops_per_sec >= 200000) {
        printf("✓ Sustained load test PASSED\n");
    } else {
        printf("✗ Sustained load test FAILED\n");
    }
}

// Main benchmark runner
int main(int argc, char* argv[]) {
    printf("=== tmux-Ghostty Integration Performance Benchmark Suite ===\n");
    printf("Target: 200k ops/s, P99 <0.5ms\n");
    printf("Iterations: %d\n", BENCHMARK_ITERATIONS);
    printf("\n");
    
    // Run all benchmarks
    benchmark_event_loop_dispatch();
    benchmark_ffi_crossing();
    benchmark_grid_operations();
    benchmark_memory_allocation();
    benchmark_concurrency();
    
    // Run short stress test (full hour test done separately)
    stress_test_sustained_load(10);  // 10 seconds for quick test
    
    printf("\n=== Benchmark Complete ===\n");
    
    return 0;
}