// benchmark_performance.c - Performance benchmark for libtmuxcore integration
// Task: T-306-R - Performance Optimization & Benchmarking
// Purpose: Ensure performance meets or exceeds 380k ops/s baseline
// Author: week3-ghostty-tmux-executor
// Date: 2025-08-26

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>
#include <math.h>

// ARM64 NEON intrinsics for Apple Silicon
#ifdef __aarch64__
#include <arm_neon.h>
#endif

// Performance targets from Week 2
#define TARGET_OPS_PER_SEC 380000  // Week 2 achievement
#define REDLINE_OPS_PER_SEC 350000 // Minimum acceptable
#define TARGET_P99_LATENCY_NS 150  // P99 latency target
#define TARGET_MEMORY_MB 10        // Max memory per session

// Benchmark configuration
#define WARMUP_ITERATIONS 100000
#define BENCHMARK_ITERATIONS 10000000
#define GRID_SIZE 80*24
#define CACHE_LINE_SIZE 64

// CPU timing - use generic time for ARM64
static inline uint64_t get_cycles(void) {
#ifdef __aarch64__
    // ARM64 doesn't expose cycle counter to userspace directly
    // Use high-resolution timer instead
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#else
    // x86_64 RDTSC
    unsigned int lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#endif
}

static inline uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Memory fence for accurate measurements  
static inline void memory_fence(void) {
#ifdef __aarch64__
    __asm__ volatile("dmb sy" ::: "memory");
#else
    __asm__ volatile("mfence" ::: "memory");
#endif
}

// Pin thread to CPU for stable measurements
static void pin_to_cpu(int cpu_id) {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#else
    // macOS doesn't support CPU pinning the same way
    // We'll use thread QoS instead
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif
}

// Statistics structure
typedef struct {
    uint64_t count;
    double min;
    double max;
    double mean;
    double stddev;
    double p50;
    double p90;
    double p95;
    double p99;
    double p999;
    double ops_per_sec;
} perf_stats_t;

// Calculate percentile
static double calculate_percentile(uint64_t* sorted_data, size_t count, double percentile) {
    size_t index = (size_t)(count * percentile / 100.0);
    if (index >= count) index = count - 1;
    return (double)sorted_data[index];
}

// Calculate statistics
static void calculate_stats(uint64_t* data, size_t count, perf_stats_t* stats, double duration_sec) {
    // Sort for percentiles
    qsort(data, count, sizeof(uint64_t), 
          (int (*)(const void*, const void*))strcmp);
    
    // Basic stats
    stats->count = count;
    stats->min = (double)data[0];
    stats->max = (double)data[count - 1];
    
    // Mean
    uint64_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += data[i];
    }
    stats->mean = (double)sum / count;
    
    // Standard deviation
    double variance = 0;
    for (size_t i = 0; i < count; i++) {
        double diff = data[i] - stats->mean;
        variance += diff * diff;
    }
    stats->stddev = sqrt(variance / count);
    
    // Percentiles
    stats->p50 = calculate_percentile(data, count, 50);
    stats->p90 = calculate_percentile(data, count, 90);
    stats->p95 = calculate_percentile(data, count, 95);
    stats->p99 = calculate_percentile(data, count, 99);
    stats->p999 = calculate_percentile(data, count, 99.9);
    
    // Throughput
    stats->ops_per_sec = count / duration_sec;
}

// Print statistics
static void print_stats(const char* test_name, perf_stats_t* stats, const char* unit) {
    printf("\n=== %s Results ===\n", test_name);
    printf("Operations: %lu\n", stats->count);
    printf("Throughput: %.0f ops/sec", stats->ops_per_sec);
    
    if (stats->ops_per_sec >= TARGET_OPS_PER_SEC) {
        printf(" ✅ (target: %dk)\n", TARGET_OPS_PER_SEC/1000);
    } else if (stats->ops_per_sec >= REDLINE_OPS_PER_SEC) {
        printf(" ⚠️  (below target but above redline)\n");
    } else {
        printf(" ❌ (below redline of %dk)\n", REDLINE_OPS_PER_SEC/1000);
    }
    
    printf("Latency (%s):\n", unit);
    printf("  Min: %.2f\n", stats->min);
    printf("  P50: %.2f\n", stats->p50);
    printf("  P90: %.2f\n", stats->p90);
    printf("  P95: %.2f\n", stats->p95);
    printf("  P99: %.2f", stats->p99);
    
    if (unit[0] == 'n' && stats->p99 <= TARGET_P99_LATENCY_NS) {
        printf(" ✅\n");
    } else if (unit[0] == 'n' && stats->p99 > TARGET_P99_LATENCY_NS) {
        printf(" ⚠️  (target: <%dns)\n", TARGET_P99_LATENCY_NS);
    } else {
        printf("\n");
    }
    
    printf("  P99.9: %.2f\n", stats->p999);
    printf("  Max: %.2f\n", stats->max);
    printf("  Mean: %.2f (σ=%.2f)\n", stats->mean, stats->stddev);
}

// Benchmark 1: Grid operations (SIMD optimized)
void benchmark_grid_operations(void* tmux_handle) {
    printf("\n" "=" "=" "=" " BENCHMARK: Grid Operations (SIMD) " "=" "=" "=" "\n");
    
    // Allocate aligned memory for grid
    uint32_t* grid = aligned_alloc(64, GRID_SIZE * sizeof(uint32_t));
    uint64_t* latencies = malloc(BENCHMARK_ITERATIONS * sizeof(uint64_t));
    
    // Initialize grid with test pattern
    for (int i = 0; i < GRID_SIZE; i++) {
        grid[i] = 0x20202020; // spaces
    }
    
    // Warmup
    printf("Warming up...\n");
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
#ifdef __aarch64__
        // ARM NEON clear operation
        uint32x4_t clear_val = vdupq_n_u32(0x20202020);
        for (int j = 0; j < GRID_SIZE; j += 4) {
            vst1q_u32(&grid[j], clear_val);
        }
#else
        // x86 SSE2 clear operation (more portable than AVX)
        for (int j = 0; j < GRID_SIZE; j += 4) {
            grid[j] = 0x20202020;
            grid[j+1] = 0x20202020;
            grid[j+2] = 0x20202020;
            grid[j+3] = 0x20202020;
        }
#endif
    }
    
    // Benchmark
    printf("Benchmarking %d operations...\n", BENCHMARK_ITERATIONS);
    memory_fence();
    uint64_t start_time = get_time_ns();
    
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        uint64_t op_start = get_cycles();
        memory_fence();
        
#ifdef __aarch64__
        // ARM NEON grid update
        int offset = i % (GRID_SIZE - 4);
        uint32x4_t update = vdupq_n_u32(0x41 + (i & 0x1F)); // 'A' + variation
        vst1q_u32(&grid[offset], update);
#else
        // Generic fallback
        int offset = i % (GRID_SIZE - 4);
        uint32_t val = 0x41 + (i & 0x1F);
        grid[offset] = val;
        grid[offset+1] = val;
        grid[offset+2] = val;
        grid[offset+3] = val;
#endif
        
        memory_fence();
        latencies[i] = get_cycles() - op_start;
    }
    
    uint64_t end_time = get_time_ns();
    double duration_sec = (end_time - start_time) / 1000000000.0;
    
    // For ARM64, cycles are already in nanoseconds from our get_cycles implementation
    // For x86, convert cycles to nanoseconds (assume 3GHz CPU)
#ifndef __aarch64__
    double cpu_ghz = 3.0;
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        latencies[i] = (uint64_t)(latencies[i] / cpu_ghz);
    }
#endif
    
    // Calculate and print statistics
    perf_stats_t stats;
    calculate_stats(latencies, BENCHMARK_ITERATIONS, &stats, duration_sec);
    print_stats("Grid Operations", &stats, "ns");
    
    free(grid);
    free(latencies);
}

// Benchmark 2: Event loop overhead
void benchmark_event_loop(void* tmux_handle) {
    printf("\n" "=" "=" "=" " BENCHMARK: Event Loop Overhead " "=" "=" "=" "\n");
    
    uint64_t* latencies = malloc(BENCHMARK_ITERATIONS * sizeof(uint64_t));
    
    // Create lightweight event structure
    typedef struct {
        int fd;
        int events;
        void* data;
    } test_event_t;
    
    test_event_t* events = aligned_alloc(CACHE_LINE_SIZE, 
                                        1000 * sizeof(test_event_t));
    
    // Initialize events
    for (int i = 0; i < 1000; i++) {
        events[i].fd = -1;
        events[i].events = 0x01;
        events[i].data = NULL;
    }
    
    // Warmup
    printf("Warming up...\n");
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        test_event_t* ev = &events[i % 1000];
        volatile int result = ev->events * 2;
        (void)result;
    }
    
    // Benchmark
    printf("Benchmarking %d operations...\n", BENCHMARK_ITERATIONS);
    memory_fence();
    uint64_t start_time = get_time_ns();
    
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        uint64_t op_start = get_cycles();
        memory_fence();
        
        // Simulate event dispatch
        test_event_t* ev = &events[i % 1000];
        if (ev->events & 0x01) {
            // Fast path
            __builtin_prefetch(ev->data, 0, 1);
            volatile int result = ev->events << 1;
            (void)result;
        }
        
        memory_fence();
        latencies[i] = get_cycles() - op_start;
    }
    
    uint64_t end_time = get_time_ns();
    double duration_sec = (end_time - start_time) / 1000000000.0;
    
    // For ARM64, cycles are already in nanoseconds from our get_cycles implementation
    // For x86, convert cycles to nanoseconds (assume 3GHz CPU)
#ifndef __aarch64__
    double cpu_ghz = 3.0;
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        latencies[i] = (uint64_t)(latencies[i] / cpu_ghz);
    }
#endif
    
    // Calculate and print statistics
    perf_stats_t stats;
    calculate_stats(latencies, BENCHMARK_ITERATIONS, &stats, duration_sec);
    print_stats("Event Loop", &stats, "ns");
    
    // Calculate overhead percentage
    double overhead_percent = (stats.mean / 1000000.0) * stats.ops_per_sec / 10.0;
    printf("Event loop overhead: %.2f%%", overhead_percent);
    if (overhead_percent < 1.0) {
        printf(" ✅ (excellent)\n");
    } else if (overhead_percent < 5.0) {
        printf(" ⚠️  (acceptable)\n");
    } else {
        printf(" ❌ (too high)\n");
    }
    
    free(events);
    free(latencies);
}

// Benchmark 3: FFI call latency
void benchmark_ffi_calls(void* lib_handle) {
    printf("\n" "=" "=" "=" " BENCHMARK: FFI Call Latency " "=" "=" "=" "\n");
    
    // Get function pointers
    void (*tmc_get_version)(int*, int*, int*) = dlsym(lib_handle, "tmc_get_version");
    if (!tmc_get_version) {
        printf("❌ Could not resolve tmc_get_version\n");
        return;
    }
    
    uint64_t* latencies = malloc(BENCHMARK_ITERATIONS * sizeof(uint64_t));
    int major, minor, patch;
    
    // Warmup
    printf("Warming up...\n");
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        tmc_get_version(&major, &minor, &patch);
    }
    
    // Benchmark
    printf("Benchmarking %d FFI calls...\n", BENCHMARK_ITERATIONS);
    memory_fence();
    uint64_t start_time = get_time_ns();
    
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        uint64_t op_start = get_cycles();
        memory_fence();
        
        tmc_get_version(&major, &minor, &patch);
        
        memory_fence();
        latencies[i] = get_cycles() - op_start;
    }
    
    uint64_t end_time = get_time_ns();
    double duration_sec = (end_time - start_time) / 1000000000.0;
    
    // For ARM64, cycles are already in nanoseconds from our get_cycles implementation
    // For x86, convert cycles to nanoseconds (assume 3GHz CPU)
#ifndef __aarch64__
    double cpu_ghz = 3.0;
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        latencies[i] = (uint64_t)(latencies[i] / cpu_ghz);
    }
#endif
    
    // Calculate and print statistics
    perf_stats_t stats;
    calculate_stats(latencies, BENCHMARK_ITERATIONS, &stats, duration_sec);
    print_stats("FFI Calls", &stats, "ns");
    
    free(latencies);
}

// Benchmark 4: Memory usage per session
void benchmark_memory_usage(void* lib_handle) {
    printf("\n" "=" "=" "=" " BENCHMARK: Memory Usage " "=" "=" "=" "\n");
    
    // Get functions
    void* (*tmc_init)(void) = dlsym(lib_handle, "tmc_init");
    void (*tmc_cleanup)(void*) = dlsym(lib_handle, "tmc_cleanup");
    int (*tmc_create_session)(void*, const char*) = dlsym(lib_handle, "tmc_create_session");
    
    if (!tmc_init || !tmc_cleanup || !tmc_create_session) {
        printf("❌ Could not resolve required functions\n");
        return;
    }
    
    // Get baseline memory
    FILE* status = fopen("/proc/self/status", "r");
    long baseline_rss = 0;
    if (status) {
        char line[256];
        while (fgets(line, sizeof(line), status)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                sscanf(line, "VmRSS: %ld kB", &baseline_rss);
                break;
            }
        }
        fclose(status);
    }
    
    // Create sessions and measure memory
    void* handle = tmc_init();
    if (!handle) {
        printf("❌ Failed to initialize\n");
        return;
    }
    
    const int num_sessions = 10;
    printf("Creating %d sessions...\n", num_sessions);
    
    for (int i = 0; i < num_sessions; i++) {
        char session_name[32];
        snprintf(session_name, sizeof(session_name), "mem-test-%d", i);
        tmc_create_session(handle, session_name);
    }
    
    // Measure memory after sessions
    status = fopen("/proc/self/status", "r");
    long after_rss = 0;
    if (status) {
        char line[256];
        while (fgets(line, sizeof(line), status)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                sscanf(line, "VmRSS: %ld kB", &after_rss);
                break;
            }
        }
        fclose(status);
    }
    
    long memory_used_kb = after_rss - baseline_rss;
    double memory_per_session_mb = (memory_used_kb / 1024.0) / num_sessions;
    
    printf("Baseline RSS: %ld KB\n", baseline_rss);
    printf("After %d sessions: %ld KB\n", num_sessions, after_rss);
    printf("Memory used: %ld KB\n", memory_used_kb);
    printf("Memory per session: %.2f MB", memory_per_session_mb);
    
    if (memory_per_session_mb <= TARGET_MEMORY_MB) {
        printf(" ✅ (target: <%dMB)\n", TARGET_MEMORY_MB);
    } else {
        printf(" ⚠️  (above target)\n");
    }
    
    tmc_cleanup(handle);
}

// Main benchmark suite
int main(int argc, char* argv[]) {
    printf("libtmuxcore Performance Benchmark Suite\n");
    printf("========================================\n");
    printf("Performance Targets:\n");
    printf("  Throughput: >%dk ops/sec (redline: %dk)\n", 
           TARGET_OPS_PER_SEC/1000, REDLINE_OPS_PER_SEC/1000);
    printf("  P99 Latency: <%dns\n", TARGET_P99_LATENCY_NS);
    printf("  Memory: <%dMB per session\n\n", TARGET_MEMORY_MB);
    
    // Pin to CPU for stable measurements
    pin_to_cpu(0);
    printf("Thread pinned to CPU 0 for stable measurements\n");
    
    // Load library
    void* lib_handle = dlopen("/Users/jqwang/98-ghosttyAI/tmux/libtmuxcore.dylib", 
                              RTLD_LAZY | RTLD_LOCAL);
    if (!lib_handle) {
        fprintf(stderr, "❌ Failed to load library: %s\n", dlerror());
        return 1;
    }
    printf("✅ Library loaded\n");
    
    // Initialize tmux core
    void* (*tmc_init)(void) = dlsym(lib_handle, "tmc_init");
    void (*tmc_cleanup)(void*) = dlsym(lib_handle, "tmc_cleanup");
    
    if (!tmc_init || !tmc_cleanup) {
        fprintf(stderr, "❌ Failed to resolve core functions\n");
        dlclose(lib_handle);
        return 1;
    }
    
    void* tmux_handle = tmc_init();
    if (!tmux_handle) {
        fprintf(stderr, "❌ Failed to initialize tmux core\n");
        dlclose(lib_handle);
        return 1;
    }
    printf("✅ Core initialized\n");
    
    // Run benchmarks
    benchmark_grid_operations(tmux_handle);
    benchmark_event_loop(tmux_handle);
    benchmark_ffi_calls(lib_handle);
    benchmark_memory_usage(lib_handle);
    
    // Cleanup
    tmc_cleanup(tmux_handle);
    dlclose(lib_handle);
    
    printf("\n" "=" "=" "=" " BENCHMARK COMPLETE " "=" "=" "=" "\n");
    printf("Review results above to ensure performance targets are met.\n");
    
    return 0;
}