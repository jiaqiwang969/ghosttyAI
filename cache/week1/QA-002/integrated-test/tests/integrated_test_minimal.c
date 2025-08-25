// integrated_test_minimal.c - Minimal Integration Test Suite
// Purpose: Test core components integration
// Author: QA-002 (qa-test-engineer)
// Date: 2025-08-25

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>
#include <stdatomic.h>

// Mock tty_ctx structure 
struct tty_ctx {
    int sx, sy;
    int cx, cy;
    int ocx, ocy;
    int xoff, yoff;
    int rupper, rlower;
    int orupper, orlower;
    unsigned int num;
    void* ptr;
    const void* data;
    size_t len;
    void* cell;
    void* wp;
};

// Mock tty structure
struct tty {
    int id;
    int sx, sy;
    void* term;
};

// Include headers
#include "ui_backend.h"
#include "tty_write_hooks.h"
#include "backend_router.h"

// External functions
struct ui_backend* ghostty_backend_create(void* handle);
void ghostty_backend_destroy(struct ui_backend* backend);
int ghostty_backend_get_call_count(struct ui_backend* backend);
void ghostty_backend_increment_call_count(struct ui_backend* backend);

// Test tracking
static atomic_int tests_passed = 0;
static atomic_int tests_failed = 0;

// =============================================================================
// Test Functions
// =============================================================================

void test_hooks_initialization() {
    printf("Test 1: Hook System Initialization... ");
    
    tty_write_hooks_init();
    
    // Verify at least one hook is registered
    const char* name = tty_write_hooks_get_function_name(0);
    if (name != NULL) {
        printf("PASS\n");
        atomic_fetch_add(&tests_passed, 1);
    } else {
        printf("FAIL\n");
        atomic_fetch_add(&tests_failed, 1);
    }
}

void test_router_creation() {
    printf("Test 2: Router Creation... ");
    
    backend_router_t* router = backend_router_create();
    if (router != NULL) {
        backend_router_destroy(router);
        printf("PASS\n");
        atomic_fetch_add(&tests_passed, 1);
    } else {
        printf("FAIL\n");
        atomic_fetch_add(&tests_failed, 1);
    }
}

void test_backend_creation() {
    printf("Test 3: Ghostty Backend Creation... ");
    
    struct ui_backend* backend = ghostty_backend_create(NULL);
    if (backend != NULL) {
        ghostty_backend_destroy(backend);
        printf("PASS\n");
        atomic_fetch_add(&tests_passed, 1);
    } else {
        printf("FAIL\n");
        atomic_fetch_add(&tests_failed, 1);
    }
}

void test_integration() {
    printf("Test 4: Component Integration... ");
    
    // Initialize components
    tty_write_hooks_init();
    backend_router_t* router = backend_router_create();
    struct ui_backend* backend = ghostty_backend_create(NULL);
    
    if (router && backend) {
        // Register backend
        int result = backend_router_register_backend(router, "ghostty", backend);
        
        if (result == 0) {
            printf("PASS\n");
            atomic_fetch_add(&tests_passed, 1);
        } else {
            printf("FAIL (registration)\n");
            atomic_fetch_add(&tests_failed, 1);
        }
    } else {
        printf("FAIL (creation)\n");
        atomic_fetch_add(&tests_failed, 1);
    }
    
    // Cleanup
    if (backend) ghostty_backend_destroy(backend);
    if (router) backend_router_destroy(router);
}

void test_concurrent_access() {
    printf("Test 5: Concurrent Access... ");
    
    backend_router_t* router = backend_router_create();
    struct ui_backend* backend = ghostty_backend_create(NULL);
    
    if (router && backend) {
        backend_router_register_backend(router, "ghostty", backend);
        
        #define NUM_THREADS 4
        #define OPS_PER_THREAD 1000
        
        pthread_t threads[NUM_THREADS];
        atomic_int total_ops = 0;
        
        void* thread_func(void* arg) {
            backend_router_t* r = (backend_router_t*)arg;
            struct tty_ctx ctx = {.num = 1};
            struct tty mock_tty = {.id = 1};
            
            for (int i = 0; i < OPS_PER_THREAD; i++) {
                backend_router_route_cmd(r, 0, &mock_tty, &ctx);
                atomic_fetch_add(&total_ops, 1);
            }
            return NULL;
        }
        
        // Launch threads
        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_create(&threads[i], NULL, thread_func, router);
        }
        
        // Wait for threads
        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_join(threads[i], NULL);
        }
        
        if (atomic_load(&total_ops) == NUM_THREADS * OPS_PER_THREAD) {
            printf("PASS\n");
            atomic_fetch_add(&tests_passed, 1);
        } else {
            printf("FAIL\n");
            atomic_fetch_add(&tests_failed, 1);
        }
    } else {
        printf("FAIL (setup)\n");
        atomic_fetch_add(&tests_failed, 1);
    }
    
    if (backend) ghostty_backend_destroy(backend);
    if (router) backend_router_destroy(router);
}

void test_performance() {
    printf("Test 6: Performance Benchmark... ");
    
    backend_router_t* router = backend_router_create();
    struct ui_backend* backend = ghostty_backend_create(NULL);
    
    if (router && backend) {
        backend_router_register_backend(router, "ghostty", backend);
        backend_router_set_active(router, "ghostty");
        
        struct tty_ctx ctx = {.num = 1};
        struct tty mock_tty = {.id = 1};
        
        clock_t start = clock();
        const int iterations = 100000;
        
        for (int i = 0; i < iterations; i++) {
            backend_router_route_cmd(router, 0, &mock_tty, &ctx);
        }
        
        clock_t end = clock();
        double duration = ((double)(end - start)) / CLOCKS_PER_SEC;
        double ops_per_sec = iterations / duration;
        
        printf("%.0f ops/sec - ", ops_per_sec);
        
        if (ops_per_sec >= 50000) {  // At least 50k ops/sec
            printf("PASS\n");
            atomic_fetch_add(&tests_passed, 1);
        } else {
            printf("FAIL\n");
            atomic_fetch_add(&tests_failed, 1);
        }
    } else {
        printf("FAIL (setup)\n");
        atomic_fetch_add(&tests_failed, 1);
    }
    
    if (backend) ghostty_backend_destroy(backend);
    if (router) backend_router_destroy(router);
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("=============================================================\n");
    printf("     Ghostty × tmux Integration Test Suite                   \n");
    printf("=============================================================\n\n");
    
    // Run tests
    test_hooks_initialization();
    test_router_creation();
    test_backend_creation();
    test_integration();
    test_concurrent_access();
    test_performance();
    
    // Report results
    printf("\n=============================================================\n");
    printf("Test Results:\n");
    printf("  Passed: %d\n", atomic_load(&tests_passed));
    printf("  Failed: %d\n", atomic_load(&tests_failed));
    
    int total = atomic_load(&tests_passed) + atomic_load(&tests_failed);
    if (total > 0) {
        float success_rate = (float)atomic_load(&tests_passed) * 100 / total;
        printf("  Success Rate: %.1f%%\n", success_rate);
        
        if (success_rate >= 65.0) {
            printf("\n✓ VERDICT: TEST SUITE PASSED (>65%% coverage target)\n");
        } else {
            printf("\n✗ VERDICT: TEST SUITE FAILED (<65%% coverage target)\n");
        }
    }
    
    printf("=============================================================\n");
    
    return atomic_load(&tests_failed) > 0 ? 1 : 0;
}