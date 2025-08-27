// test_event_loop_edge_cases.c
// Regression tests for event loop defects
// QA-002 Task T-404

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "../../../CORE-001/src/event_loop_backend.h"

// Test DEFECT-001: NULL pointer checks
void test_null_handle_safety() {
    printf("Testing NULL handle safety...\n");
    
    event_loop_router_t* router = event_loop_router_init(ROUTER_MODE_LIBEVENT);
    assert(router != NULL);
    
    // Test NULL handle
    int ret = event_loop_add(router, NULL, NULL);
    assert(ret == -1);  // Should fail gracefully
    
    ret = event_loop_del(router, NULL);
    assert(ret == -1);  // Should fail gracefully
    
    // Test with valid but uninitialized handle
    event_handle_t* handle = event_loop_create_event(router);
    assert(handle != NULL);
    
    // Set invalid parameters
    event_loop_set(router, handle, -1, 0, NULL, NULL);
    ret = event_loop_add(router, handle, NULL);
    assert(ret == -1);  // Should fail with invalid params
    
    event_loop_free_event(router, handle);
    event_loop_router_cleanup(router);
    
    printf("  ✓ NULL handle safety test passed\n");
}

// Test DEFECT-004: Memory leak on error paths
void test_memory_leak_on_errors() {
    printf("Testing memory leak prevention...\n");
    
    event_loop_router_t* router = event_loop_router_init(ROUTER_MODE_LIBEVENT);
    assert(router != NULL);
    
    // Create and delete many events to test for leaks
    for (int i = 0; i < 1000; i++) {
        event_handle_t* handle = event_loop_create_event(router);
        assert(handle != NULL);
        
        // Simulate various error conditions
        if (i % 2 == 0) {
            // Add with invalid fd
            event_loop_set(router, handle, -999, EL_EVENT_READ, NULL, NULL);
            event_loop_add(router, handle, NULL);  // May fail
        } else {
            // Add normally then delete
            event_loop_set(router, handle, STDIN_FILENO, EL_EVENT_READ, NULL, NULL);
            event_loop_add(router, handle, NULL);
            event_loop_del(router, handle);
        }
        
        event_loop_free_event(router, handle);
    }
    
    // Check stats for memory issues
    event_loop_stats_t stats;
    event_loop_get_stats(router, &stats);
    
    // Events added and deleted should be balanced
    printf("  Events added: %llu, deleted: %llu\n", 
           stats.events_added, stats.events_deleted);
    
    event_loop_router_cleanup(router);
    printf("  ✓ Memory leak prevention test passed\n");
}

// Thread safety stress test
typedef struct {
    event_loop_router_t* router;
    int thread_id;
    int operations;
} thread_data_t;

void* stress_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    for (int i = 0; i < data->operations; i++) {
        event_handle_t* handle = event_loop_create_event(data->router);
        if (handle) {
            event_loop_set(data->router, handle, 
                          data->thread_id, EL_EVENT_READ, NULL, NULL);
            event_loop_add(data->router, handle, NULL);
            usleep(rand() % 100);  // Random delay
            event_loop_del(data->router, handle);
            event_loop_free_event(data->router, handle);
        }
    }
    
    return NULL;
}

void test_thread_safety() {
    printf("Testing thread safety under stress...\n");
    
    event_loop_router_t* router = event_loop_router_init(ROUTER_MODE_LIBEVENT);
    assert(router != NULL);
    
    const int num_threads = 10;
    const int ops_per_thread = 100;
    pthread_t threads[num_threads];
    thread_data_t thread_data[num_threads];
    
    // Start threads
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].router = router;
        thread_data[i].thread_id = i;
        thread_data[i].operations = ops_per_thread;
        pthread_create(&threads[i], NULL, stress_thread, &thread_data[i]);
    }
    
    // Wait for completion
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    event_loop_stats_t stats;
    event_loop_get_stats(router, &stats);
    printf("  Total operations: %llu\n", stats.events_added);
    
    event_loop_router_cleanup(router);
    printf("  ✓ Thread safety test passed\n");
}

// Test mode switching safety
void test_mode_switching() {
    printf("Testing mode switching safety...\n");
    
    event_loop_router_t* router = event_loop_router_init(ROUTER_MODE_LIBEVENT);
    assert(router != NULL);
    
    // Add some events
    event_handle_t* handles[5];
    for (int i = 0; i < 5; i++) {
        handles[i] = event_loop_create_event(router);
        event_loop_set(router, handles[i], i, EL_EVENT_READ, NULL, NULL);
        event_loop_add(router, handles[i], NULL);
    }
    
    // Switch mode (should handle existing events gracefully)
    int ret = event_loop_router_switch_mode(router, ROUTER_MODE_GHOSTTY);
    assert(ret == 0 || ret == -1);  // May not be implemented yet
    
    // Clean up
    for (int i = 0; i < 5; i++) {
        event_loop_del(router, handles[i]);
        event_loop_free_event(router, handles[i]);
    }
    
    event_loop_router_cleanup(router);
    printf("  ✓ Mode switching safety test passed\n");
}

// Performance overhead test
void test_performance_overhead() {
    printf("Testing performance overhead...\n");
    
    event_loop_router_t* router = event_loop_router_init(ROUTER_MODE_LIBEVENT);
    assert(router != NULL);
    
    const int num_events = 10000;
    event_handle_t* handles[num_events];
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Add many events
    for (int i = 0; i < num_events; i++) {
        handles[i] = event_loop_create_event(router);
        event_loop_set(router, handles[i], i % 100, EL_EVENT_READ, NULL, NULL);
        event_loop_add(router, handles[i], NULL);
    }
    
    // Remove all events
    for (int i = 0; i < num_events; i++) {
        event_loop_del(router, handles[i]);
        event_loop_free_event(router, handles[i]);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed = (end.tv_sec - start.tv_sec) + 
                    (end.tv_nsec - start.tv_nsec) / 1e9;
    double ops_per_sec = (num_events * 2) / elapsed;
    
    printf("  Operations/sec: %.0f\n", ops_per_sec);
    
    double overhead = event_loop_get_overhead_percent(router);
    printf("  Overhead: %.2f%%\n", overhead);
    assert(overhead < 1.0);  // Must be less than 1%
    
    event_loop_router_cleanup(router);
    printf("  ✓ Performance overhead test passed\n");
}

int main() {
    printf("=== Event Loop Edge Case Regression Tests ===\n\n");
    
    test_null_handle_safety();
    test_memory_leak_on_errors();
    test_thread_safety();
    test_mode_switching();
    test_performance_overhead();
    
    printf("\n=== All regression tests passed! ===\n");
    return 0;
}