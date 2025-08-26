// event_loop_integration_test.c - Integration tests for event loop vtable
// Purpose: Validate event loop backend integration with real callbacks
// Author: QA-001 (qa-test-lead)
// Date: 2025-08-26
// Task: T-401 - Integration test framework
// Coverage Target: 100% critical paths

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>

// Include components under test
#include "../../cache/week2/CORE-001/src/event_loop_backend.h"

// Test configuration
#define TEST_ITERATIONS 10000
#define CONCURRENT_EVENTS 100
#define PERFORMANCE_TARGET_NS 1000000  // 1ms max overhead
#define MEMORY_CHECK_INTERVAL 1000

// Test statistics
typedef struct {
    uint64_t total_tests;
    uint64_t passed_tests;
    uint64_t failed_tests;
    uint64_t performance_violations;
    uint64_t memory_leaks;
    double avg_latency_ns;
    double max_latency_ns;
    double min_latency_ns;
} test_stats_t;

static test_stats_t g_stats = {0};
static event_loop_router_t* g_router = NULL;

// ============================================================================
// Test Utilities
// ============================================================================

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static void record_latency(uint64_t latency_ns) {
    if (g_stats.min_latency_ns == 0 || latency_ns < g_stats.min_latency_ns) {
        g_stats.min_latency_ns = latency_ns;
    }
    if (latency_ns > g_stats.max_latency_ns) {
        g_stats.max_latency_ns = latency_ns;
    }
    
    // Update average
    double total = g_stats.avg_latency_ns * g_stats.total_tests;
    g_stats.total_tests++;
    g_stats.avg_latency_ns = (total + latency_ns) / g_stats.total_tests;
    
    if (latency_ns > PERFORMANCE_TARGET_NS) {
        g_stats.performance_violations++;
    }
}

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAILED: %s at %s:%d\n", msg, __FILE__, __LINE__); \
        g_stats.failed_tests++; \
        return -1; \
    } \
    g_stats.passed_tests++; \
} while (0)

// ============================================================================
// Test 1: Basic Event Loop Operations
// ============================================================================

static volatile int basic_callback_count = 0;
static volatile int basic_callback_fd = -1;

static void basic_callback(int fd, short events, void* arg) {
    basic_callback_count++;
    basic_callback_fd = fd;
    (void)events;
    (void)arg;
}

static int test_basic_operations(void) {
    printf("\n[TEST] Basic Event Loop Operations\n");
    
    // Test router initialization
    g_router = event_loop_router_init(ROUTER_MODE_LIBEVENT);
    TEST_ASSERT(g_router != NULL, "Router initialization failed");
    TEST_ASSERT(g_router->mode == ROUTER_MODE_LIBEVENT, "Wrong router mode");
    TEST_ASSERT(g_router->vtable != NULL, "Vtable not set");
    TEST_ASSERT(g_router->backend_base != NULL, "Backend not initialized");
    
    // Test event creation
    event_handle_t* handle = event_loop_create_event(g_router);
    TEST_ASSERT(handle != NULL, "Event creation failed");
    
    // Create pipe for I/O testing
    int pipe_fds[2];
    TEST_ASSERT(pipe(pipe_fds) == 0, "Pipe creation failed");
    
    // Set non-blocking
    fcntl(pipe_fds[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_fds[1], F_SETFL, O_NONBLOCK);
    
    // Configure event
    event_loop_set(g_router, handle, pipe_fds[0], EL_EVENT_READ | EL_EVENT_PERSIST,
                   basic_callback, NULL);
    
    // Add event
    TEST_ASSERT(event_loop_add(g_router, handle, NULL) == 0, "Event add failed");
    
    // Trigger event by writing to pipe
    write(pipe_fds[1], "test", 4);
    
    // Run event loop once
    uint64_t start_ns = get_time_ns();
    event_loop_run_once(g_router);
    uint64_t latency_ns = get_time_ns() - start_ns;
    record_latency(latency_ns);
    
    // Verify callback was called
    TEST_ASSERT(basic_callback_count == 1, "Callback not called");
    TEST_ASSERT(basic_callback_fd == pipe_fds[0], "Wrong FD in callback");
    
    // Clean up
    event_loop_del(g_router, handle);
    event_loop_free_event(g_router, handle);
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    
    printf("  ✓ Basic operations: PASSED (latency: %.2f µs)\n", latency_ns / 1000.0);
    return 0;
}

// ============================================================================
// Test 2: Backend Switching
// ============================================================================

static int test_backend_switching(void) {
    printf("\n[TEST] Backend Mode Switching\n");
    
    if (!g_router) {
        g_router = event_loop_router_init(ROUTER_MODE_LIBEVENT);
    }
    
    // Get initial stats
    event_loop_stats_t stats_before;
    event_loop_get_stats(g_router, &stats_before);
    
    // Switch to Ghostty mode
    uint64_t switch_start = get_time_ns();
    int result = event_loop_router_switch_mode(g_router, ROUTER_MODE_GHOSTTY);
    uint64_t switch_time = get_time_ns() - switch_start;
    
    TEST_ASSERT(result == 0, "Mode switch to Ghostty failed");
    TEST_ASSERT(g_router->mode == ROUTER_MODE_GHOSTTY, "Mode not updated");
    record_latency(switch_time);
    
    // Verify new backend is functional
    event_handle_t* test_event = event_loop_create_event(g_router);
    TEST_ASSERT(test_event != NULL, "Event creation in new mode failed");
    event_loop_free_event(g_router, test_event);
    
    // Switch to hybrid mode
    result = event_loop_router_switch_mode(g_router, ROUTER_MODE_HYBRID);
    TEST_ASSERT(result == 0, "Mode switch to hybrid failed");
    TEST_ASSERT(g_router->mode == ROUTER_MODE_HYBRID, "Hybrid mode not set");
    
    // Switch back to libevent
    result = event_loop_router_switch_mode(g_router, ROUTER_MODE_LIBEVENT);
    TEST_ASSERT(result == 0, "Mode switch back to libevent failed");
    
    printf("  ✓ Backend switching: PASSED (switch time: %.2f µs)\n", 
           switch_time / 1000.0);
    return 0;
}

// ============================================================================
// Test 3: Thread Safety
// ============================================================================

typedef struct {
    int thread_id;
    int iterations;
    volatile int completed;
    pthread_t thread;
} thread_test_data_t;

static void* thread_worker(void* arg) {
    thread_test_data_t* data = (thread_test_data_t*)arg;
    
    for (int i = 0; i < data->iterations; i++) {
        // Create and destroy events rapidly
        event_handle_t* handle = event_loop_create_event(g_router);
        if (handle) {
            // Random operations to stress thread safety
            if (i % 2 == 0) {
                event_loop_add(g_router, handle, NULL);
                usleep(1);  // Small delay
                event_loop_del(g_router, handle);
            }
            event_loop_free_event(g_router, handle);
        }
        
        // Occasionally switch modes (stress test)
        if (i % 100 == 0) {
            router_mode_t new_mode = (i % 3 == 0) ? ROUTER_MODE_GHOSTTY : 
                                     (i % 3 == 1) ? ROUTER_MODE_HYBRID : 
                                     ROUTER_MODE_LIBEVENT;
            event_loop_router_switch_mode(g_router, new_mode);
        }
    }
    
    data->completed = 1;
    return NULL;
}

static int test_thread_safety(void) {
    printf("\n[TEST] Thread Safety Under Load\n");
    
    const int NUM_THREADS = 10;
    thread_test_data_t threads[NUM_THREADS];
    
    // Initialize threads
    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i].thread_id = i;
        threads[i].iterations = 1000;
        threads[i].completed = 0;
    }
    
    uint64_t start_ns = get_time_ns();
    
    // Start threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i].thread, NULL, thread_worker, &threads[i]);
    }
    
    // Wait for completion
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i].thread, NULL);
    }
    
    uint64_t elapsed_ns = get_time_ns() - start_ns;
    
    // Verify all threads completed
    for (int i = 0; i < NUM_THREADS; i++) {
        TEST_ASSERT(threads[i].completed == 1, "Thread did not complete");
    }
    
    // Check for memory consistency
    event_loop_stats_t final_stats;
    event_loop_get_stats(g_router, &final_stats);
    TEST_ASSERT(final_stats.events_added >= final_stats.events_deleted, 
                "Event count mismatch (memory leak?)");
    
    printf("  ✓ Thread safety: PASSED (%d threads, %.2f ms total)\n",
           NUM_THREADS, elapsed_ns / 1000000.0);
    return 0;
}

// ============================================================================
// Test 4: Performance Regression
// ============================================================================

static volatile uint64_t perf_callback_count = 0;

static void perf_callback(int fd, short events, void* arg) {
    perf_callback_count++;
    (void)fd; (void)events; (void)arg;
}

static int test_performance_regression(void) {
    printf("\n[TEST] Performance Regression (<1%% overhead)\n");
    
    const int NUM_EVENTS = 1000;
    const int ITERATIONS = 10000;
    event_handle_t* events[NUM_EVENTS];
    int pipe_fds[NUM_EVENTS][2];
    
    // Create many events
    for (int i = 0; i < NUM_EVENTS; i++) {
        TEST_ASSERT(pipe(pipe_fds[i]) == 0, "Pipe creation failed");
        fcntl(pipe_fds[i][0], F_SETFL, O_NONBLOCK);
        
        events[i] = event_loop_create_event(g_router);
        event_loop_set(g_router, events[i], pipe_fds[i][0], 
                      EL_EVENT_READ | EL_EVENT_PERSIST, perf_callback, NULL);
        event_loop_add(g_router, events[i], NULL);
    }
    
    // Baseline: measure with libevent mode
    event_loop_router_switch_mode(g_router, ROUTER_MODE_LIBEVENT);
    perf_callback_count = 0;
    
    uint64_t libevent_start = get_time_ns();
    for (int i = 0; i < ITERATIONS; i++) {
        // Trigger some events
        for (int j = 0; j < 10; j++) {
            write(pipe_fds[j % NUM_EVENTS][1], "x", 1);
        }
        event_loop_run_once(g_router);
    }
    uint64_t libevent_time = get_time_ns() - libevent_start;
    uint64_t libevent_callbacks = perf_callback_count;
    
    // Test: measure with Ghostty mode
    event_loop_router_switch_mode(g_router, ROUTER_MODE_GHOSTTY);
    perf_callback_count = 0;
    
    uint64_t ghostty_start = get_time_ns();
    for (int i = 0; i < ITERATIONS; i++) {
        // Trigger same events
        for (int j = 0; j < 10; j++) {
            write(pipe_fds[j % NUM_EVENTS][1], "x", 1);
        }
        event_loop_run_once(g_router);
    }
    uint64_t ghostty_time = get_time_ns() - ghostty_start;
    uint64_t ghostty_callbacks = perf_callback_count;
    
    // Calculate overhead
    double overhead_percent = ((double)(ghostty_time - libevent_time) / libevent_time) * 100.0;
    double throughput_libevent = (double)libevent_callbacks / (libevent_time / 1000000000.0);
    double throughput_ghostty = (double)ghostty_callbacks / (ghostty_time / 1000000000.0);
    
    // Clean up
    for (int i = 0; i < NUM_EVENTS; i++) {
        event_loop_del(g_router, events[i]);
        event_loop_free_event(g_router, events[i]);
        close(pipe_fds[i][0]);
        close(pipe_fds[i][1]);
    }
    
    TEST_ASSERT(overhead_percent < 1.0, "Performance regression > 1%");
    TEST_ASSERT(throughput_ghostty > 200000, "Throughput < 200k ops/s");
    
    printf("  ✓ Performance: PASSED\n");
    printf("    - Overhead: %.3f%%\n", overhead_percent);
    printf("    - Libevent: %.0f ops/s (%.2f ms total)\n", 
           throughput_libevent, libevent_time / 1000000.0);
    printf("    - Ghostty:  %.0f ops/s (%.2f ms total)\n",
           throughput_ghostty, ghostty_time / 1000000.0);
    
    return 0;
}

// ============================================================================
// Test 5: Memory Leak Detection
// ============================================================================

static int test_memory_leaks(void) {
    printf("\n[TEST] Memory Leak Detection\n");
    
    const int LEAK_TEST_ITERATIONS = 10000;
    
    // Get baseline memory stats
    event_loop_stats_t baseline_stats;
    event_loop_get_stats(g_router, &baseline_stats);
    
    // Rapid allocation/deallocation cycles
    for (int i = 0; i < LEAK_TEST_ITERATIONS; i++) {
        event_handle_t* handles[10];
        
        // Allocate
        for (int j = 0; j < 10; j++) {
            handles[j] = event_loop_create_event(g_router);
            TEST_ASSERT(handles[j] != NULL, "Event creation failed");
        }
        
        // Add/remove some
        for (int j = 0; j < 5; j++) {
            event_loop_add(g_router, handles[j], NULL);
        }
        for (int j = 0; j < 5; j++) {
            event_loop_del(g_router, handles[j]);
        }
        
        // Deallocate
        for (int j = 0; j < 10; j++) {
            event_loop_free_event(g_router, handles[j]);
        }
        
        // Periodically check for leaks
        if (i % MEMORY_CHECK_INTERVAL == 0) {
            event_loop_stats_t current_stats;
            event_loop_get_stats(g_router, &current_stats);
            
            // Events added should roughly equal events deleted
            int64_t leak_indicator = current_stats.events_added - current_stats.events_deleted;
            if (leak_indicator > 100) {  // Allow small discrepancy
                g_stats.memory_leaks++;
                fprintf(stderr, "  ! Potential leak detected at iteration %d\n", i);
            }
        }
    }
    
    // Final memory check
    event_loop_stats_t final_stats;
    event_loop_get_stats(g_router, &final_stats);
    
    TEST_ASSERT(g_stats.memory_leaks == 0, "Memory leaks detected");
    TEST_ASSERT(final_stats.events_added == final_stats.events_deleted,
                "Event count mismatch");
    
    printf("  ✓ Memory leaks: PASSED (0 leaks in %d iterations)\n",
           LEAK_TEST_ITERATIONS);
    return 0;
}

// ============================================================================
// Test 6: Signal Handling
// ============================================================================

static volatile int signal_received = 0;
static volatile int signal_number = 0;

static void signal_callback(int fd, short events, void* arg) {
    signal_received = 1;
    signal_number = fd;  // In signal events, fd is the signal number
    (void)events; (void)arg;
}

static int test_signal_handling(void) {
    printf("\n[TEST] Signal Handling\n");
    
    // Create signal event
    event_handle_t* sig_event = event_loop_signal_new(g_router, SIGUSR1,
                                                      signal_callback, NULL);
    TEST_ASSERT(sig_event != NULL, "Signal event creation failed");
    
    // Add signal event
    TEST_ASSERT(event_loop_signal_add(g_router, sig_event) == 0,
                "Signal add failed");
    
    // Send signal to self
    signal_received = 0;
    kill(getpid(), SIGUSR1);
    
    // Process events
    event_loop_run_once(g_router);
    
    TEST_ASSERT(signal_received == 1, "Signal not received");
    TEST_ASSERT(signal_number == SIGUSR1, "Wrong signal number");
    
    // Clean up
    event_loop_signal_del(g_router, sig_event);
    event_loop_free_event(g_router, sig_event);
    
    printf("  ✓ Signal handling: PASSED\n");
    return 0;
}

// ============================================================================
// Test 7: Timer Events
// ============================================================================

static volatile int timer_fired = 0;
static struct timeval timer_fired_at;

static void timer_callback(int fd, short events, void* arg) {
    timer_fired++;
    gettimeofday(&timer_fired_at, NULL);
    (void)fd; (void)events; (void)arg;
}

static int test_timer_events(void) {
    printf("\n[TEST] Timer Events\n");
    
    // Create timer event
    event_handle_t* timer_event = event_loop_timer_new(g_router,
                                                       timer_callback, NULL);
    TEST_ASSERT(timer_event != NULL, "Timer event creation failed");
    
    // Set 10ms timeout
    struct timeval timeout = { .tv_sec = 0, .tv_usec = 10000 };
    struct timeval start_time;
    gettimeofday(&start_time, NULL);
    
    TEST_ASSERT(event_loop_timer_add(g_router, timer_event, &timeout) == 0,
                "Timer add failed");
    
    // Wait for timer
    timer_fired = 0;
    while (timer_fired == 0 && get_time_ns() - (start_time.tv_sec * 1000000000ULL) < 100000000ULL) {
        event_loop_run_once(g_router);
        usleep(1000);
    }
    
    TEST_ASSERT(timer_fired == 1, "Timer did not fire");
    
    // Check timing accuracy (allow 5ms tolerance)
    uint64_t elapsed_us = (timer_fired_at.tv_sec - start_time.tv_sec) * 1000000 +
                         (timer_fired_at.tv_usec - start_time.tv_usec);
    TEST_ASSERT(elapsed_us >= 9000 && elapsed_us <= 15000,
                "Timer accuracy out of range");
    
    // Clean up
    event_loop_timer_del(g_router, timer_event);
    event_loop_free_event(g_router, timer_event);
    
    printf("  ✓ Timer events: PASSED (accuracy: %lu µs)\n", elapsed_us);
    return 0;
}

// ============================================================================
// Main Test Runner
// ============================================================================

static void print_summary(void) {
    printf("\n" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" 
           "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" 
           "═" "═" "═" "═" "═" "═" "═" "═" "\n");
    printf("INTEGRATION TEST SUMMARY\n");
    printf("═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" 
           "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" 
           "═" "═" "═" "═" "═" "═" "═" "═" "\n");
    printf("Total Tests:    %lu\n", g_stats.total_tests);
    printf("Passed:         %lu\n", g_stats.passed_tests);
    printf("Failed:         %lu\n", g_stats.failed_tests);
    printf("Memory Leaks:   %lu\n", g_stats.memory_leaks);
    printf("Perf Violations:%lu\n", g_stats.performance_violations);
    printf("\nPerformance Metrics:\n");
    printf("Average Latency:%.2f µs\n", g_stats.avg_latency_ns / 1000.0);
    printf("Min Latency:    %.2f µs\n", g_stats.min_latency_ns / 1000.0);
    printf("Max Latency:    %.2f µs\n", g_stats.max_latency_ns / 1000.0);
    printf("═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" 
           "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" "═" 
           "═" "═" "═" "═" "═" "═" "═" "═" "\n");
    
    if (g_stats.failed_tests == 0 && g_stats.memory_leaks == 0) {
        printf("✅ ALL TESTS PASSED!\n");
    } else {
        printf("❌ TESTS FAILED!\n");
    }
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    
    printf("Event Loop Integration Test Suite v1.0\n");
    printf("========================================\n");
    
    int failed = 0;
    
    // Run all tests
    failed |= test_basic_operations();
    failed |= test_backend_switching();
    failed |= test_thread_safety();
    failed |= test_performance_regression();
    failed |= test_memory_leaks();
    failed |= test_signal_handling();
    failed |= test_timer_events();
    
    // Clean up router
    if (g_router) {
        event_loop_router_cleanup(g_router);
        g_router = NULL;
    }
    
    print_summary();
    
    return failed ? 1 : 0;
}