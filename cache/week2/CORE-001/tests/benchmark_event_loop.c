// benchmark_event_loop.c - Performance Benchmarks for Event Loop Backend
// Purpose: Verify <1% overhead requirement for T-201
// Author: CORE-001 (c-tmux-specialist)
// Date: 2025-08-25
// Version: 1.0.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <event2/event.h>
#include "../src/event_loop_backend.h"

// Benchmark configuration
#define WARMUP_ITERATIONS 1000
#define BENCHMARK_ITERATIONS 100000
#define NUM_EVENTS 100
#define NUM_RUNS 5

// Timing helpers
static inline uint64_t get_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// Dummy callback
static int callback_count = 0;
static void bench_callback(int fd, short events, void* data) {
    callback_count++;
}

// ============================================================================
// Native libevent benchmark (baseline)
// ============================================================================

double benchmark_native_libevent() {
    struct event_base* base = event_base_new();
    if (!base) {
        printf("Failed to create event base\n");
        return -1;
    }
    
    // Create events
    struct event* events[NUM_EVENTS];
    for (int i = 0; i < NUM_EVENTS; i++) {
        events[i] = event_new(base, -1, 0, bench_callback, NULL);
    }
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        struct timeval tv = {0, 1};
        event_add(events[i % NUM_EVENTS], &tv);
        event_del(events[i % NUM_EVENTS]);
    }
    
    // Benchmark
    uint64_t start = get_time_us();
    
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        int idx = i % NUM_EVENTS;
        struct timeval tv = {0, 1};
        event_add(events[idx], &tv);
        event_del(events[idx]);
    }
    
    uint64_t elapsed = get_time_us() - start;
    
    // Cleanup
    for (int i = 0; i < NUM_EVENTS; i++) {
        event_free(events[i]);
    }
    event_base_free(base);
    
    return (double)elapsed / BENCHMARK_ITERATIONS;
}

// ============================================================================
// Router with libevent backend benchmark
// ============================================================================

double benchmark_router_libevent() {
    event_loop_router_t* router = event_loop_router_init(ROUTER_MODE_LIBEVENT);
    if (!router) {
        printf("Failed to create router\n");
        return -1;
    }
    
    // Create events
    event_handle_t* handles[NUM_EVENTS];
    for (int i = 0; i < NUM_EVENTS; i++) {
        handles[i] = event_loop_create_event(router);
        event_loop_set(router, handles[i], -1, 0, bench_callback, NULL);
    }
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        struct timeval tv = {0, 1};
        event_loop_add(router, handles[i % NUM_EVENTS], &tv);
        event_loop_del(router, handles[i % NUM_EVENTS]);
    }
    
    // Benchmark
    uint64_t start = get_time_us();
    
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        int idx = i % NUM_EVENTS;
        struct timeval tv = {0, 1};
        event_loop_add(router, handles[idx], &tv);
        event_loop_del(router, handles[idx]);
    }
    
    uint64_t elapsed = get_time_us() - start;
    
    // Cleanup
    for (int i = 0; i < NUM_EVENTS; i++) {
        event_loop_free_event(router, handles[i]);
    }
    event_loop_router_cleanup(router);
    
    return (double)elapsed / BENCHMARK_ITERATIONS;
}

// ============================================================================
// Router with Ghostty backend benchmark
// ============================================================================

double benchmark_router_ghostty() {
    event_loop_router_t* router = event_loop_router_init(ROUTER_MODE_GHOSTTY);
    if (!router) {
        printf("Failed to create router\n");
        return -1;
    }
    
    // Create events
    event_handle_t* handles[NUM_EVENTS];
    for (int i = 0; i < NUM_EVENTS; i++) {
        handles[i] = event_loop_create_event(router);
        event_loop_set(router, handles[i], -1, 0, bench_callback, NULL);
    }
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        struct timeval tv = {0, 1};
        event_loop_add(router, handles[i % NUM_EVENTS], &tv);
        event_loop_del(router, handles[i % NUM_EVENTS]);
    }
    
    // Benchmark
    uint64_t start = get_time_us();
    
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        int idx = i % NUM_EVENTS;
        struct timeval tv = {0, 1};
        event_loop_add(router, handles[idx], &tv);
        event_loop_del(router, handles[idx]);
    }
    
    uint64_t elapsed = get_time_us() - start;
    
    // Cleanup
    for (int i = 0; i < NUM_EVENTS; i++) {
        event_loop_free_event(router, handles[i]);
    }
    event_loop_router_cleanup(router);
    
    return (double)elapsed / BENCHMARK_ITERATIONS;
}

// ============================================================================
// Loop dispatch benchmark
// ============================================================================

double benchmark_loop_dispatch(router_mode_t mode) {
    event_loop_router_t* router = event_loop_router_init(mode);
    if (!router) {
        printf("Failed to create router\n");
        return -1;
    }
    
    // Create timer events
    event_handle_t* timer = event_loop_create_event(router);
    event_loop_set(router, timer, -1, EL_EVENT_TIMEOUT, bench_callback, NULL);
    
    struct timeval tv = {0, 1}; // 1 microsecond
    event_loop_add(router, timer, &tv);
    
    // Warmup
    for (int i = 0; i < 100; i++) {
        event_loop_run_once(router);
    }
    
    // Benchmark
    callback_count = 0;
    uint64_t start = get_time_us();
    
    for (int i = 0; i < 1000; i++) {
        event_loop_run_once(router);
    }
    
    uint64_t elapsed = get_time_us() - start;
    
    // Cleanup
    event_loop_del(router, timer);
    event_loop_free_event(router, timer);
    event_loop_router_cleanup(router);
    
    return (double)elapsed / 1000;
}

// ============================================================================
// Main benchmark runner
// ============================================================================

int main(int argc, char** argv) {
    printf("========================================\n");
    printf("Event Loop Backend Performance Benchmark\n");
    printf("========================================\n\n");
    
    printf("Configuration:\n");
    printf("  Iterations: %d\n", BENCHMARK_ITERATIONS);
    printf("  Events: %d\n", NUM_EVENTS);
    printf("  Runs: %d\n\n", NUM_RUNS);
    
    // Run benchmarks multiple times and average
    double native_times[NUM_RUNS];
    double router_libevent_times[NUM_RUNS];
    double router_ghostty_times[NUM_RUNS];
    
    for (int run = 0; run < NUM_RUNS; run++) {
        printf("Run %d/%d...\n", run + 1, NUM_RUNS);
        
        native_times[run] = benchmark_native_libevent();
        router_libevent_times[run] = benchmark_router_libevent();
        router_ghostty_times[run] = benchmark_router_ghostty();
    }
    
    // Calculate averages
    double native_avg = 0, router_libevent_avg = 0, router_ghostty_avg = 0;
    for (int i = 0; i < NUM_RUNS; i++) {
        native_avg += native_times[i];
        router_libevent_avg += router_libevent_times[i];
        router_ghostty_avg += router_ghostty_times[i];
    }
    native_avg /= NUM_RUNS;
    router_libevent_avg /= NUM_RUNS;
    router_ghostty_avg /= NUM_RUNS;
    
    printf("\n========================================\n");
    printf("Results (average per operation):\n");
    printf("========================================\n");
    printf("Native libevent:        %.3f us\n", native_avg);
    printf("Router (libevent):      %.3f us\n", router_libevent_avg);
    printf("Router (ghostty):       %.3f us\n", router_ghostty_avg);
    printf("\n");
    
    // Calculate overhead
    double libevent_overhead = ((router_libevent_avg - native_avg) / native_avg) * 100;
    double ghostty_overhead = ((router_ghostty_avg - native_avg) / native_avg) * 100;
    
    printf("Overhead:\n");
    printf("  Router (libevent): %.2f%%\n", libevent_overhead);
    printf("  Router (ghostty):  %.2f%%\n", ghostty_overhead);
    printf("\n");
    
    // Loop dispatch benchmark
    printf("Loop dispatch latency:\n");
    double dispatch_native = benchmark_loop_dispatch(ROUTER_MODE_LIBEVENT);
    double dispatch_ghostty = benchmark_loop_dispatch(ROUTER_MODE_GHOSTTY);
    printf("  Libevent backend: %.2f us\n", dispatch_native);
    printf("  Ghostty backend:  %.2f us\n", dispatch_ghostty);
    printf("\n");
    
    // Throughput calculation
    double ops_per_sec_native = 1000000.0 / native_avg;
    double ops_per_sec_router = 1000000.0 / router_libevent_avg;
    
    printf("Throughput:\n");
    printf("  Native:  %.0f ops/sec\n", ops_per_sec_native);
    printf("  Router:  %.0f ops/sec\n", ops_per_sec_router);
    printf("\n");
    
    // Performance verdict
    printf("========================================\n");
    printf("Performance Verdict:\n");
    if (libevent_overhead < 1.0) {
        printf("✅ PASS: Router overhead is %.2f%% (<1%% requirement)\n", libevent_overhead);
    } else {
        printf("❌ FAIL: Router overhead is %.2f%% (>1%% requirement)\n", libevent_overhead);
    }
    
    if (ops_per_sec_router > 200000) {
        printf("✅ PASS: Throughput is %.0f ops/sec (>200k requirement)\n", ops_per_sec_router);
    } else {
        printf("⚠️  WARN: Throughput is %.0f ops/sec (<200k target)\n", ops_per_sec_router);
    }
    printf("========================================\n");
    
    return (libevent_overhead < 1.0) ? 0 : 1;
}