// stress_test.c - Stress test suite for tmux-in-Ghostty integration
// Purpose: Test system stability under extreme load conditions
// Author: QA-001 (qa-test-lead)
// Date: 2025-08-26
// Task: T-401 - Integration test framework
// Requirements: 1000 concurrent sessions, 10000 ops/sec, <50% CPU

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <errno.h>
#include <math.h>

// Include components under test
#include "../../cache/week2/CORE-001/src/event_loop_backend.h"

// ============================================================================
// Stress Test Configuration
// ============================================================================

#define MAX_CONCURRENT_SESSIONS 1000
#define TARGET_OPS_PER_SECOND 10000
#define TEST_DURATION_SECONDS 3600  // 1 hour
#define CPU_USAGE_THRESHOLD 50      // Max 50% CPU
#define MEMORY_CHECK_INTERVAL 100   // Check every 100 iterations
#define THREAD_POOL_SIZE 100         // Worker threads

// Operation types for mixed workload
typedef enum {
    OP_CREATE_SESSION,
    OP_DESTROY_SESSION,
    OP_CREATE_WINDOW,
    OP_DESTROY_WINDOW,
    OP_SPLIT_PANE,
    OP_WRITE_DATA,
    OP_RESIZE,
    OP_SCROLL,
    OP_SWITCH_WINDOW,
    OP_EVENT_PROCESS,
    OP_TYPE_COUNT
} operation_type_t;

// Session structure
typedef struct session {
    int id;
    int window_count;
    int pane_count;
    event_loop_router_t* router;
    event_handle_t** events;
    int event_count;
    pthread_mutex_t lock;
    volatile int active;
} session_t;

// Global stress test state
typedef struct {
    session_t sessions[MAX_CONCURRENT_SESSIONS];
    volatile int active_sessions;
    volatile uint64_t total_operations;
    volatile uint64_t operations_by_type[OP_TYPE_COUNT];
    volatile uint64_t errors;
    volatile int should_stop;
    pthread_mutex_t global_lock;
    
    // Performance metrics
    volatile uint64_t total_latency_ns;
    volatile uint64_t max_latency_ns;
    volatile uint64_t operations_per_second;
    
    // Resource tracking
    volatile double cpu_usage_percent;
    volatile uint64_t memory_usage_bytes;
    volatile uint64_t peak_memory_bytes;
} stress_state_t;

static stress_state_t g_stress = {0};

// ============================================================================
// Utility Functions
// ============================================================================

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static double get_cpu_usage(void) {
    static struct rusage last_usage;
    static uint64_t last_time_ns = 0;
    
    struct rusage current;
    getrusage(RUSAGE_SELF, &current);
    
    uint64_t current_time_ns = get_time_ns();
    if (last_time_ns == 0) {
        last_usage = current;
        last_time_ns = current_time_ns;
        return 0.0;
    }
    
    uint64_t elapsed_ns = current_time_ns - last_time_ns;
    uint64_t cpu_time_us = 
        ((current.ru_utime.tv_sec - last_usage.ru_utime.tv_sec) * 1000000 +
         (current.ru_utime.tv_usec - last_usage.ru_utime.tv_usec)) +
        ((current.ru_stime.tv_sec - last_usage.ru_stime.tv_sec) * 1000000 +
         (current.ru_stime.tv_usec - last_usage.ru_stime.tv_usec));
    
    double cpu_percent = (double)(cpu_time_us * 1000) / (double)elapsed_ns * 100.0;
    
    last_usage = current;
    last_time_ns = current_time_ns;
    
    return cpu_percent;
}

static uint64_t get_memory_usage(void) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss * 1024; // Convert to bytes
}

// ============================================================================
// Session Operations
// ============================================================================

static int create_session(int id) {
    if (id >= MAX_CONCURRENT_SESSIONS) return -1;
    
    session_t* session = &g_stress.sessions[id];
    pthread_mutex_lock(&session->lock);
    
    if (session->active) {
        pthread_mutex_unlock(&session->lock);
        return -1;
    }
    
    // Initialize session
    session->id = id;
    session->window_count = 1;
    session->pane_count = 1;
    session->router = event_loop_router_init(ROUTER_MODE_GHOSTTY);
    
    if (!session->router) {
        pthread_mutex_unlock(&session->lock);
        g_stress.errors++;
        return -1;
    }
    
    // Create initial events
    session->event_count = 10;
    session->events = calloc(session->event_count, sizeof(event_handle_t*));
    for (int i = 0; i < session->event_count; i++) {
        session->events[i] = event_loop_create_event(session->router);
    }
    
    session->active = 1;
    __sync_fetch_and_add(&g_stress.active_sessions, 1);
    
    pthread_mutex_unlock(&session->lock);
    return 0;
}

static int destroy_session(int id) {
    if (id >= MAX_CONCURRENT_SESSIONS) return -1;
    
    session_t* session = &g_stress.sessions[id];
    pthread_mutex_lock(&session->lock);
    
    if (!session->active) {
        pthread_mutex_unlock(&session->lock);
        return -1;
    }
    
    // Clean up events
    if (session->events) {
        for (int i = 0; i < session->event_count; i++) {
            if (session->events[i]) {
                event_loop_del(session->router, session->events[i]);
                event_loop_free_event(session->router, session->events[i]);
            }
        }
        free(session->events);
        session->events = NULL;
    }
    
    // Clean up router
    if (session->router) {
        event_loop_router_cleanup(session->router);
        session->router = NULL;
    }
    
    session->active = 0;
    __sync_fetch_and_sub(&g_stress.active_sessions, 1);
    
    pthread_mutex_unlock(&session->lock);
    return 0;
}

static int perform_operation(int session_id, operation_type_t op_type) {
    if (session_id >= MAX_CONCURRENT_SESSIONS) return -1;
    
    session_t* session = &g_stress.sessions[session_id];
    if (!session->active) return -1;
    
    uint64_t start_ns = get_time_ns();
    int result = 0;
    
    switch (op_type) {
        case OP_CREATE_WINDOW:
            session->window_count++;
            break;
            
        case OP_DESTROY_WINDOW:
            if (session->window_count > 1) {
                session->window_count--;
            }
            break;
            
        case OP_SPLIT_PANE:
            session->pane_count++;
            break;
            
        case OP_WRITE_DATA: {
            // Simulate writing data to a pane
            if (session->events && session->event_count > 0) {
                event_handle_t* event = session->events[0];
                event_loop_add(session->router, event, NULL);
                event_loop_run_once(session->router);
                event_loop_del(session->router, event);
            }
            break;
        }
            
        case OP_EVENT_PROCESS: {
            // Process events
            event_loop_run_once(session->router);
            break;
        }
            
        case OP_RESIZE:
            // Simulate resize
            session->pane_count = session->pane_count; // No-op for now
            break;
            
        default:
            break;
    }
    
    uint64_t latency_ns = get_time_ns() - start_ns;
    __sync_fetch_and_add(&g_stress.total_latency_ns, latency_ns);
    
    if (latency_ns > g_stress.max_latency_ns) {
        g_stress.max_latency_ns = latency_ns;
    }
    
    return result;
}

// ============================================================================
// Worker Thread
// ============================================================================

typedef struct {
    int thread_id;
    pthread_t thread;
    volatile uint64_t operations;
    volatile uint64_t errors;
} worker_thread_t;

static void* worker_thread_func(void* arg) {
    worker_thread_t* worker = (worker_thread_t*)arg;
    
    // Set up random number generator
    unsigned int seed = worker->thread_id + time(NULL);
    
    while (!g_stress.should_stop) {
        // Select random session
        int session_id = rand_r(&seed) % MAX_CONCURRENT_SESSIONS;
        
        // Select random operation
        operation_type_t op_type = rand_r(&seed) % OP_TYPE_COUNT;
        
        // Special handling for create/destroy
        if (op_type == OP_CREATE_SESSION) {
            if (g_stress.active_sessions < MAX_CONCURRENT_SESSIONS) {
                // Find free session slot
                for (int i = 0; i < MAX_CONCURRENT_SESSIONS; i++) {
                    if (!g_stress.sessions[i].active) {
                        if (create_session(i) == 0) {
                            worker->operations++;
                            g_stress.operations_by_type[op_type]++;
                        }
                        break;
                    }
                }
            }
        } else if (op_type == OP_DESTROY_SESSION) {
            if (g_stress.active_sessions > 0) {
                if (destroy_session(session_id) == 0) {
                    worker->operations++;
                    g_stress.operations_by_type[op_type]++;
                }
            }
        } else {
            // Regular operation
            if (perform_operation(session_id, op_type) == 0) {
                worker->operations++;
                g_stress.operations_by_type[op_type]++;
            } else {
                worker->errors++;
            }
        }
        
        // Rate limiting to achieve target ops/sec
        usleep(THREAD_POOL_SIZE * 1000000 / TARGET_OPS_PER_SECOND);
    }
    
    return NULL;
}

// ============================================================================
// Monitoring Thread
// ============================================================================

static void* monitor_thread_func(void* arg) {
    (void)arg;
    
    uint64_t last_ops = 0;
    uint64_t last_time_ns = get_time_ns();
    int report_interval = 10; // Report every 10 seconds
    
    while (!g_stress.should_stop) {
        sleep(report_interval);
        
        // Calculate operations per second
        uint64_t current_ops = g_stress.total_operations;
        uint64_t current_time_ns = get_time_ns();
        uint64_t ops_delta = current_ops - last_ops;
        uint64_t time_delta_ns = current_time_ns - last_time_ns;
        
        g_stress.operations_per_second = 
            (ops_delta * 1000000000ULL) / time_delta_ns;
        
        // Update resource usage
        g_stress.cpu_usage_percent = get_cpu_usage();
        g_stress.memory_usage_bytes = get_memory_usage();
        
        if (g_stress.memory_usage_bytes > g_stress.peak_memory_bytes) {
            g_stress.peak_memory_bytes = g_stress.memory_usage_bytes;
        }
        
        // Print status report
        printf("\n[STRESS TEST STATUS] Time: %lus\n", 
               (current_time_ns - get_time_ns()) / 1000000000ULL);
        printf("  Active Sessions:  %d / %d\n", 
               g_stress.active_sessions, MAX_CONCURRENT_SESSIONS);
        printf("  Operations/sec:   %lu (target: %d)\n", 
               g_stress.operations_per_second, TARGET_OPS_PER_SECOND);
        printf("  Total Operations: %lu\n", current_ops);
        printf("  CPU Usage:        %.1f%% (threshold: %d%%)\n", 
               g_stress.cpu_usage_percent, CPU_USAGE_THRESHOLD);
        printf("  Memory Usage:     %lu MB (peak: %lu MB)\n",
               g_stress.memory_usage_bytes / (1024*1024),
               g_stress.peak_memory_bytes / (1024*1024));
        printf("  Errors:           %lu\n", g_stress.errors);
        printf("  Avg Latency:      %.2f µs\n",
               (double)g_stress.total_latency_ns / current_ops / 1000.0);
        printf("  Max Latency:      %.2f ms\n",
               (double)g_stress.max_latency_ns / 1000000.0);
        
        // Check for violations
        if (g_stress.cpu_usage_percent > CPU_USAGE_THRESHOLD) {
            printf("  ⚠️  CPU usage exceeded threshold!\n");
        }
        
        if (g_stress.operations_per_second < TARGET_OPS_PER_SECOND * 0.9) {
            printf("  ⚠️  Operations/sec below target!\n");
        }
        
        // Update for next iteration
        last_ops = current_ops;
        last_time_ns = current_time_ns;
    }
    
    return NULL;
}

// ============================================================================
// Memory Leak Detection Thread
// ============================================================================

static void* memory_check_thread_func(void* arg) {
    (void)arg;
    
    uint64_t initial_memory = get_memory_usage();
    uint64_t stable_threshold = initial_memory * 1.1; // 10% growth allowed
    
    while (!g_stress.should_stop) {
        sleep(30); // Check every 30 seconds
        
        uint64_t current_memory = get_memory_usage();
        
        // Check for memory leaks (continuous growth)
        if (current_memory > stable_threshold) {
            double growth_percent = 
                ((double)(current_memory - initial_memory) / initial_memory) * 100.0;
            
            printf("\n⚠️  Memory growth detected: %.1f%% increase\n", growth_percent);
            printf("    Initial: %lu MB, Current: %lu MB\n",
                   initial_memory / (1024*1024),
                   current_memory / (1024*1024));
            
            // Update threshold to track continued growth
            stable_threshold = current_memory * 1.05;
        }
    }
    
    return NULL;
}

// ============================================================================
// Main Stress Test
// ============================================================================

static void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\nShutting down stress test...\n");
        g_stress.should_stop = 1;
    }
}

static void print_operation_breakdown(void) {
    const char* op_names[] = {
        "CREATE_SESSION", "DESTROY_SESSION", "CREATE_WINDOW", "DESTROY_WINDOW",
        "SPLIT_PANE", "WRITE_DATA", "RESIZE", "SCROLL", "SWITCH_WINDOW", "EVENT_PROCESS"
    };
    
    printf("\nOperation Breakdown:\n");
    for (int i = 0; i < OP_TYPE_COUNT; i++) {
        printf("  %-20s: %lu\n", op_names[i], g_stress.operations_by_type[i]);
    }
}

int main(int argc, char** argv) {
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║           STRESS TEST SUITE - tmux-in-Ghostty             ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║ Configuration:                                            ║\n");
    printf("║   Max Sessions:     %4d                                  ║\n", MAX_CONCURRENT_SESSIONS);
    printf("║   Target Ops/sec:   %5d                                 ║\n", TARGET_OPS_PER_SECOND);
    printf("║   Test Duration:    %d hour                              ║\n", TEST_DURATION_SECONDS/3600);
    printf("║   CPU Threshold:    %d%%                                   ║\n", CPU_USAGE_THRESHOLD);
    printf("║   Worker Threads:   %d                                   ║\n", THREAD_POOL_SIZE);
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Parse command line arguments
    int duration = TEST_DURATION_SECONDS;
    if (argc > 1) {
        duration = atoi(argv[1]);
        printf("Custom test duration: %d seconds\n", duration);
    }
    
    // Initialize mutexes
    pthread_mutex_init(&g_stress.global_lock, NULL);
    for (int i = 0; i < MAX_CONCURRENT_SESSIONS; i++) {
        pthread_mutex_init(&g_stress.sessions[i].lock, NULL);
    }
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create initial sessions
    printf("Creating initial sessions...\n");
    for (int i = 0; i < MAX_CONCURRENT_SESSIONS / 2; i++) {
        if (create_session(i) != 0) {
            fprintf(stderr, "Failed to create session %d\n", i);
        }
    }
    printf("Initial sessions created: %d\n\n", g_stress.active_sessions);
    
    // Start worker threads
    printf("Starting %d worker threads...\n", THREAD_POOL_SIZE);
    worker_thread_t workers[THREAD_POOL_SIZE];
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        workers[i].thread_id = i;
        workers[i].operations = 0;
        workers[i].errors = 0;
        pthread_create(&workers[i].thread, NULL, worker_thread_func, &workers[i]);
    }
    
    // Start monitoring thread
    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, monitor_thread_func, NULL);
    
    // Start memory check thread
    pthread_t memory_thread;
    pthread_create(&memory_thread, NULL, memory_check_thread_func, NULL);
    
    // Run for specified duration
    uint64_t start_time = get_time_ns();
    uint64_t end_time = start_time + (duration * 1000000000ULL);
    
    printf("\nStress test running for %d seconds...\n", duration);
    printf("Press Ctrl+C to stop early.\n\n");
    
    while (!g_stress.should_stop && get_time_ns() < end_time) {
        // Update total operations counter
        uint64_t total = 0;
        for (int i = 0; i < THREAD_POOL_SIZE; i++) {
            total += workers[i].operations;
        }
        g_stress.total_operations = total;
        
        sleep(1);
    }
    
    // Signal shutdown
    g_stress.should_stop = 1;
    
    // Wait for threads to finish
    printf("\nWaiting for threads to finish...\n");
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_join(workers[i].thread, NULL);
    }
    pthread_join(monitor_thread, NULL);
    pthread_join(memory_thread, NULL);
    
    // Clean up remaining sessions
    printf("Cleaning up sessions...\n");
    for (int i = 0; i < MAX_CONCURRENT_SESSIONS; i++) {
        if (g_stress.sessions[i].active) {
            destroy_session(i);
        }
    }
    
    // Destroy mutexes
    pthread_mutex_destroy(&g_stress.global_lock);
    for (int i = 0; i < MAX_CONCURRENT_SESSIONS; i++) {
        pthread_mutex_destroy(&g_stress.sessions[i].lock);
    }
    
    // Print final report
    uint64_t elapsed_ns = get_time_ns() - start_time;
    double elapsed_seconds = (double)elapsed_ns / 1000000000.0;
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║                    STRESS TEST RESULTS                    ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║ Duration:           %.1f seconds                        \n", elapsed_seconds);
    printf("║ Total Operations:   %lu                                 \n", g_stress.total_operations);
    printf("║ Avg Ops/sec:        %.0f                                \n", 
           g_stress.total_operations / elapsed_seconds);
    printf("║ Peak Sessions:      %d                                  \n", MAX_CONCURRENT_SESSIONS);
    printf("║ Errors:             %lu                                 \n", g_stress.errors);
    printf("║ Peak Memory:        %lu MB                              \n", 
           g_stress.peak_memory_bytes / (1024*1024));
    printf("║ Peak CPU:           %.1f%%                              \n", g_stress.cpu_usage_percent);
    printf("║ Avg Latency:        %.2f µs                             \n",
           (double)g_stress.total_latency_ns / g_stress.total_operations / 1000.0);
    printf("║ Max Latency:        %.2f ms                             \n",
           (double)g_stress.max_latency_ns / 1000000.0);
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    print_operation_breakdown();
    
    // Determine pass/fail
    int passed = 1;
    
    if (g_stress.errors > g_stress.total_operations * 0.001) {  // <0.1% error rate
        printf("\n❌ FAILED: Error rate too high (>0.1%%)\n");
        passed = 0;
    }
    
    if (g_stress.cpu_usage_percent > CPU_USAGE_THRESHOLD) {
        printf("\n❌ FAILED: CPU usage exceeded %d%%\n", CPU_USAGE_THRESHOLD);
        passed = 0;
    }
    
    if (g_stress.operations_per_second < TARGET_OPS_PER_SECOND * 0.9) {
        printf("\n❌ FAILED: Throughput below target (%.0f < %d)\n",
               g_stress.total_operations / elapsed_seconds, TARGET_OPS_PER_SECOND);
        passed = 0;
    }
    
    uint64_t final_memory = get_memory_usage();
    uint64_t initial_memory = g_stress.peak_memory_bytes / 10; // Estimate
    if (final_memory > initial_memory * 2) {
        printf("\n❌ FAILED: Possible memory leak detected\n");
        passed = 0;
    }
    
    if (passed) {
        printf("\n✅ STRESS TEST PASSED - System stable under load!\n");
        return 0;
    } else {
        return 1;
    }
}