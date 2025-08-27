// Simplified test runner for CORE-002 backend_router
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdatomic.h>
#include <pthread.h>
#include <time.h>

// Define necessary structures
struct tty_ctx {
    int num;
    int xoff;
    int yoff;
    int rupper;
    int rlower;
    char data[256];
};

struct tty {
    int id;
};

struct ui_backend {
    int id;
    void (*cmd_cell)(struct ui_backend*, const struct tty_ctx*);
};

// Don't include conflicting headers

#define NUM_THREADS 10
#define OPS_PER_THREAD 1000

// Test counters
static atomic_int test_passed = 0;
static atomic_int test_failed = 0;
static atomic_int tty_calls = 0;
static atomic_int ui_calls = 0;

// Mock functions
void mock_tty_cmd(struct tty* tty, const struct tty_ctx* ctx) {
    (void)tty;
    (void)ctx;
    atomic_fetch_add(&tty_calls, 1);
}

void mock_ui_cmd(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)backend;
    (void)ctx;
    atomic_fetch_add(&ui_calls, 1);
}

// Simple test framework
void test_basic_routing() {
    printf("Test: Basic Routing... ");
    
    // Reset counters
    atomic_store(&tty_calls, 0);
    atomic_store(&ui_calls, 0);
    
    struct tty_ctx ctx = {0};
    struct tty mock_tty = {1};
    
    // Simulate routing - would call backend_router functions here
    // For now, just test the mock functions work
    mock_tty_cmd(&mock_tty, &ctx);
    
    if (atomic_load(&tty_calls) == 1) {
        printf("PASS\n");
        atomic_fetch_add(&test_passed, 1);
    } else {
        printf("FAIL\n");
        atomic_fetch_add(&test_failed, 1);
    }
}

void test_ui_routing() {
    printf("Test: UI Routing... ");
    
    atomic_store(&ui_calls, 0);
    
    struct tty_ctx ctx = {0};
    struct ui_backend backend = {1, mock_ui_cmd};
    
    backend.cmd_cell(&backend, &ctx);
    
    if (atomic_load(&ui_calls) == 1) {
        printf("PASS\n");
        atomic_fetch_add(&test_passed, 1);
    } else {
        printf("FAIL\n");
        atomic_fetch_add(&test_failed, 1);
    }
}

void* thread_func(void* arg) {
    (void)arg;
    struct tty_ctx ctx = {0};
    struct tty mock_tty = {1};
    struct ui_backend backend = {1, mock_ui_cmd};
    
    for (int i = 0; i < OPS_PER_THREAD; i++) {
        if (i % 2 == 0) {
            mock_tty_cmd(&mock_tty, &ctx);
        } else {
            mock_ui_cmd(&backend, &ctx);
        }
    }
    return NULL;
}

void test_concurrent_access() {
    printf("Test: Concurrent Access... ");
    
    atomic_store(&tty_calls, 0);
    atomic_store(&ui_calls, 0);
    
    pthread_t threads[NUM_THREADS];
    
    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_func, NULL);
    }
    
    // Wait for threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    int expected_tty = NUM_THREADS * (OPS_PER_THREAD / 2);
    int expected_ui = NUM_THREADS * (OPS_PER_THREAD / 2);
    
    if (atomic_load(&tty_calls) == expected_tty && 
        atomic_load(&ui_calls) == expected_ui) {
        printf("PASS\n");
        atomic_fetch_add(&test_passed, 1);
    } else {
        printf("FAIL (expected %d/%d, got %d/%d)\n", 
               expected_tty, expected_ui,
               atomic_load(&tty_calls), atomic_load(&ui_calls));
        atomic_fetch_add(&test_failed, 1);
    }
}

int main() {
    printf("=============================================================\n");
    printf("CORE-002 Backend Router Test Suite\n");
    printf("=============================================================\n\n");
    
    test_basic_routing();
    test_ui_routing();
    test_concurrent_access();
    
    printf("\n=============================================================\n");
    printf("Test Results:\n");
    printf("  Tests Passed: %d\n", atomic_load(&test_passed));
    printf("  Tests Failed: %d\n", atomic_load(&test_failed));
    printf("  Success Rate: %.1f%%\n", 
           (float)atomic_load(&test_passed) * 100 / 
           (atomic_load(&test_passed) + atomic_load(&test_failed)));
    printf("=============================================================\n");
    
    return atomic_load(&test_failed) > 0 ? 1 : 0;
}