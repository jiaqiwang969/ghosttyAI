/*
 * test_monitoring_overhead.c - Performance Impact Test Suite
 * 
 * Measures the overhead introduced by monitoring instrumentation.
 * Target: <0.1% CPU overhead, <1MB memory overhead.
 */

#include "../include/monitoring_integration.h"
#include "../src/metrics_collection.c"
#include "../src/logging_framework.c"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>
#include <pthread.h>
#include <unistd.h>

// Test configuration
#define TEST_ITERATIONS 10000000
#define TEST_THREADS 4
#define WARMUP_ITERATIONS 1000000

// Simulated event loop operation
static void simulate_event_loop_callback(void) {
    // Minimal work to simulate real callback
    volatile int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += i;
    }
}

// Simulated grid operation
static void simulate_grid_operation(int batch_size) {
    // Simulate batch processing
    volatile int cells = batch_size * 8;
    for (int i = 0; i < cells; i++) {
        volatile int value = i * 2;
        (void)value;
    }
}

// Benchmark without monitoring
static double benchmark_no_monitoring(void) {
    struct timespec start, end;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        simulate_event_loop_callback();
        
        if (i % 1000 == 0) {
            simulate_grid_operation(32);
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed = (end.tv_sec - start.tv_sec) + 
                    (end.tv_nsec - start.tv_nsec) / 1e9;
    return elapsed;
}

// Benchmark with monitoring enabled
static double benchmark_with_monitoring(void) {
    struct timespec start, end;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        METRICS_EVENT_LOOP_CALLBACK_START();
        simulate_event_loop_callback();
        METRICS_EVENT_LOOP_CALLBACK_END();
        
        if (i % 1000 == 0) {
            int batch_size = 32;
            METRICS_GRID_BATCH_UPDATE(batch_size, batch_size * 8);
            simulate_grid_operation(batch_size);
        }
        
        // Add some FFI metrics
        if (i % 10000 == 0) {
            METRICS_FFI_CALL_START();
            usleep(1); // Simulate FFI call
            METRICS_FFI_CALL_END();
        }
        
        // Add memory metrics
        if (i % 100000 == 0) {
            METRICS_MEMORY_ALLOC(1024);
            METRICS_MEMORY_FREE(1024);
        }
        
        // Add logging
        if (i % 1000000 == 0) {
            log_info("test", "Progress: %d/%d", i, TEST_ITERATIONS);
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed = (end.tv_sec - start.tv_sec) + 
                    (end.tv_nsec - start.tv_nsec) / 1e9;
    return elapsed;
}

// Thread worker for parallel testing
typedef struct {
    int thread_id;
    int iterations;
    double elapsed_time;
    int with_monitoring;
} thread_context_t;

static void* thread_worker(void* arg) {
    thread_context_t* ctx = (thread_context_t*)arg;
    struct timespec start, end;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < ctx->iterations; i++) {
        if (ctx->with_monitoring) {
            timing_context_t timer = METRICS_TIMING_START(METRIC_EVENT_LOOP_LATENCY);
            simulate_event_loop_callback();
            METRICS_TIMING_END(&timer);
            
            if (i % 1000 == 0) {
                log_debug("worker", "Thread %d progress: %d", ctx->thread_id, i);
            }
        } else {
            simulate_event_loop_callback();
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    ctx->elapsed_time = (end.tv_sec - start.tv_sec) + 
                        (end.tv_nsec - start.tv_nsec) / 1e9;
    return NULL;
}

// Memory usage test
static void test_memory_overhead(void) {
    struct rusage usage_before, usage_after;
    
    getrusage(RUSAGE_SELF, &usage_before);
    
    // Initialize monitoring
    metrics_init("/tmp/test_metrics.txt");
    logging_init("/tmp/test_log.json");
    
    // Generate lots of metrics
    for (int i = 0; i < 100000; i++) {
        METRICS_INCREMENT(METRIC_EVENT_LOOP_OPS);
        METRICS_OBSERVE(METRIC_GRID_BATCH_SIZE, rand() % 100);
        
        if (i % 100 == 0) {
            log_info("memory_test", "Iteration %d", i);
        }
    }
    
    getrusage(RUSAGE_SELF, &usage_after);
    
    long memory_increase = usage_after.ru_maxrss - usage_before.ru_maxrss;
    printf("Memory overhead: %ld KB\n", memory_increase);
    
    // Cleanup
    metrics_shutdown();
    logging_shutdown();
}

// Main test runner
int main(int argc, char* argv[]) {
    printf("=== Monitoring Overhead Test Suite ===\n\n");
    
    // Initialize monitoring systems
    printf("Initializing monitoring systems...\n");
    metrics_init("/tmp/metrics_test.txt");
    logging_init("/tmp/log_test.json");
    metrics_enable();
    
    // Warmup
    printf("Warming up...\n");
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        simulate_event_loop_callback();
    }
    
    // Single-threaded benchmark
    printf("\n--- Single-threaded Benchmark ---\n");
    
    printf("Running baseline (no monitoring)...\n");
    double time_no_monitoring = benchmark_no_monitoring();
    double ops_per_sec_base = TEST_ITERATIONS / time_no_monitoring;
    printf("Baseline: %.2f seconds (%.0f ops/sec)\n", 
           time_no_monitoring, ops_per_sec_base);
    
    printf("Running with monitoring...\n");
    double time_with_monitoring = benchmark_with_monitoring();
    double ops_per_sec_monitored = TEST_ITERATIONS / time_with_monitoring;
    printf("With monitoring: %.2f seconds (%.0f ops/sec)\n", 
           time_with_monitoring, ops_per_sec_monitored);
    
    double overhead_percent = ((time_with_monitoring - time_no_monitoring) / 
                              time_no_monitoring) * 100;
    printf("Overhead: %.3f%%\n", overhead_percent);
    
    if (overhead_percent < 0.1) {
        printf("✅ PASS: Overhead within target (<0.1%%)\n");
    } else if (overhead_percent < 1.0) {
        printf("⚠️  WARNING: Overhead higher than target but acceptable\n");
    } else {
        printf("❌ FAIL: Overhead too high (>1%%)\n");
    }
    
    // Multi-threaded benchmark
    printf("\n--- Multi-threaded Benchmark ---\n");
    printf("Testing with %d threads...\n", TEST_THREADS);
    
    pthread_t threads[TEST_THREADS];
    thread_context_t contexts[TEST_THREADS];
    
    // Without monitoring
    for (int i = 0; i < TEST_THREADS; i++) {
        contexts[i].thread_id = i;
        contexts[i].iterations = TEST_ITERATIONS / TEST_THREADS;
        contexts[i].with_monitoring = 0;
        pthread_create(&threads[i], NULL, thread_worker, &contexts[i]);
    }
    
    double total_time_mt_base = 0;
    for (int i = 0; i < TEST_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total_time_mt_base += contexts[i].elapsed_time;
    }
    
    printf("Multi-threaded baseline: %.2f seconds (avg)\n", 
           total_time_mt_base / TEST_THREADS);
    
    // With monitoring
    for (int i = 0; i < TEST_THREADS; i++) {
        contexts[i].with_monitoring = 1;
        pthread_create(&threads[i], NULL, thread_worker, &contexts[i]);
    }
    
    double total_time_mt_monitored = 0;
    for (int i = 0; i < TEST_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total_time_mt_monitored += contexts[i].elapsed_time;
    }
    
    printf("Multi-threaded with monitoring: %.2f seconds (avg)\n", 
           total_time_mt_monitored / TEST_THREADS);
    
    double mt_overhead_percent = ((total_time_mt_monitored - total_time_mt_base) / 
                                  total_time_mt_base) * 100;
    printf("Multi-threaded overhead: %.3f%%\n", mt_overhead_percent);
    
    // Memory overhead test
    printf("\n--- Memory Overhead Test ---\n");
    test_memory_overhead();
    
    // Export final metrics
    printf("\n--- Final Metrics ---\n");
    uint64_t logged, dropped;
    logging_get_stats(&logged, &dropped);
    printf("Log entries: %llu logged, %llu dropped\n", logged, dropped);
    
    // Performance summary
    printf("\n=== Performance Summary ===\n");
    printf("Single-threaded overhead: %.3f%%\n", overhead_percent);
    printf("Multi-threaded overhead: %.3f%%\n", mt_overhead_percent);
    printf("Target overhead: <0.1%%\n");
    
    double avg_overhead = (overhead_percent + mt_overhead_percent) / 2;
    if (avg_overhead < 0.1) {
        printf("\n🎉 EXCELLENT: Average overhead %.3f%% is well within target!\n", 
               avg_overhead);
    } else if (avg_overhead < 0.5) {
        printf("\n✅ GOOD: Average overhead %.3f%% is acceptable\n", avg_overhead);
    } else {
        printf("\n⚠️  NEEDS OPTIMIZATION: Average overhead %.3f%% is too high\n", 
               avg_overhead);
    }
    
    // Cleanup
    metrics_shutdown();
    logging_shutdown();
    
    return (avg_overhead < 1.0) ? 0 : 1;
}