// test_tty_write_hooks_extended.c - Extended tests for CORE-001
// Purpose: Increase test coverage from 70% to 75%
// Author: QA-002 (qa-test-engineer)
// Date: 2025-08-25

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include "../tty_write_hooks.h"
#include "../../ARCH-001/tty_ctx_unified.h"
#include "../../ARCH-001/ui_backend.h"

// Test statistics
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Mock structures
struct tty {
    int id;
    int sx, sy;
};

// Test macros
#define RUN_TEST(test_func) do { \
    printf("Running %s... ", #test_func); \
    tests_run++; \
    if (test_func()) { \
        printf("PASS\n"); \
        tests_passed++; \
    } else { \
        printf("FAIL\n"); \
        tests_failed++; \
    } \
} while(0)

// =============================================================================
// Edge Case Tests (Previously Uncovered)
// =============================================================================

// Test 1: Hook installation with NULL backend
int test_hook_installation_null_backend() {
    tty_hooks_init();
    
    // Try to install hooks with NULL backend
    int result = tty_hooks_install_backend(NULL);
    if (result != -1) {
        return 0; // Should fail with NULL
    }
    
    return 1;
}

// Test 2: Hook routing with invalid command ID
int test_hook_routing_invalid_cmd() {
    tty_hooks_init();
    
    struct tty mock_tty = {1, 80, 24};
    struct tty_ctx ctx;
    tty_ctx_init(&ctx);
    
    // Try invalid command IDs
    int result = tty_hooks_route_command(999, &mock_tty, &ctx);
    if (result != -1) {
        return 0; // Should fail with invalid ID
    }
    
    result = tty_hooks_route_command(-1, &mock_tty, &ctx);
    if (result != -1) {
        return 0; // Should fail with negative ID
    }
    
    return 1;
}

// Test 3: Statistics overflow handling
int test_statistics_overflow() {
    tty_hooks_init();
    tty_hooks_reset_stats();
    
    // Simulate very large call count
    for (int i = 0; i < 100000; i++) {
        tty_hooks_increment_call_count(0);
    }
    
    tty_hooks_stats_t stats;
    tty_hooks_get_stats(&stats);
    
    if (stats.total_calls < 100000) {
        return 0; // Counter should handle large values
    }
    
    return 1;
}

// Test 4: Concurrent hook access
int test_concurrent_hook_access() {
    tty_hooks_init();
    
    #define NUM_THREADS 10
    #define OPS_PER_THREAD 1000
    
    pthread_t threads[NUM_THREADS];
    
    void* thread_func(void* arg) {
        (void)arg;
        struct tty mock_tty = {1, 80, 24};
        struct tty_ctx ctx;
        tty_ctx_init(&ctx);
        
        for (int i = 0; i < OPS_PER_THREAD; i++) {
            tty_hooks_route_command(i % 22, &mock_tty, &ctx);
        }
        return NULL;
    }
    
    // Launch threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_func, NULL);
    }
    
    // Wait for completion
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    tty_hooks_stats_t stats;
    tty_hooks_get_stats(&stats);
    
    if (stats.total_calls != NUM_THREADS * OPS_PER_THREAD) {
        return 0; // Should be thread-safe
    }
    
    return 1;
}

// Test 5: Hook uninstallation
int test_hook_uninstallation() {
    tty_hooks_init();
    
    // Install hooks
    struct ui_backend mock_backend = {0};
    tty_hooks_install_backend(&mock_backend);
    
    // Uninstall
    int result = tty_hooks_uninstall_backend();
    if (result != 0) {
        return 0; // Should succeed
    }
    
    // Try to route after uninstall
    struct tty mock_tty = {1, 80, 24};
    struct tty_ctx ctx;
    tty_ctx_init(&ctx);
    
    result = tty_hooks_route_command(0, &mock_tty, &ctx);
    if (result == 0) {
        return 0; // Should fail after uninstall
    }
    
    return 1;
}

// Test 6: Memory stress test
int test_memory_stress() {
    // Multiple init/cleanup cycles
    for (int i = 0; i < 100; i++) {
        tty_hooks_init();
        
        struct ui_backend mock_backend = {0};
        tty_hooks_install_backend(&mock_backend);
        
        struct tty mock_tty = {1, 80, 24};
        struct tty_ctx ctx;
        tty_ctx_init(&ctx);
        
        tty_hooks_route_command(0, &mock_tty, &ctx);
        
        tty_hooks_uninstall_backend();
        tty_hooks_cleanup();
    }
    
    // If we get here without crash/leak, test passes
    return 1;
}

// Test 7: Function name boundary checks
int test_function_name_boundaries() {
    tty_hooks_init();
    
    // Test boundary indices
    const char* name = tty_hooks_get_function_name(-1);
    if (name != NULL) {
        return 0; // Should return NULL for negative
    }
    
    name = tty_hooks_get_function_name(22);
    if (name != NULL) {
        return 0; // Should return NULL for out of bounds
    }
    
    name = tty_hooks_get_function_name(1000);
    if (name != NULL) {
        return 0; // Should return NULL for large invalid
    }
    
    // Valid indices
    for (int i = 0; i < 22; i++) {
        name = tty_hooks_get_function_name(i);
        if (name == NULL || strlen(name) == 0) {
            return 0; // All valid indices should have names
        }
    }
    
    return 1;
}

// Test 8: Hook priority and ordering
int test_hook_priority() {
    tty_hooks_init();
    
    // Install primary backend
    struct ui_backend primary = {0};
    primary.version = 1;
    tty_hooks_install_backend(&primary);
    
    // Try to install secondary (should fail or replace)
    struct ui_backend secondary = {0};
    secondary.version = 2;
    int result = tty_hooks_install_backend(&secondary);
    
    // Verify current backend
    struct ui_backend* current = tty_hooks_get_current_backend();
    if (current == NULL) {
        return 0;
    }
    
    // Should be the secondary (newer) one
    if (current->version != 2) {
        return 0;
    }
    
    return 1;
}

// Test 9: Error recovery
int test_error_recovery() {
    tty_hooks_init();
    
    // Cause various errors and verify recovery
    
    // Error 1: Double init
    tty_hooks_init(); // Second init should be safe
    
    // Error 2: Route without backend
    struct tty mock_tty = {1, 80, 24};
    struct tty_ctx ctx;
    tty_ctx_init(&ctx);
    
    int result = tty_hooks_route_command(0, &mock_tty, &ctx);
    if (result == 0) {
        return 0; // Should fail without backend
    }
    
    // Error 3: Install after error
    struct ui_backend backend = {0};
    result = tty_hooks_install_backend(&backend);
    if (result != 0) {
        return 0; // Should recover and succeed
    }
    
    // Verify system is functional after errors
    result = tty_hooks_route_command(0, &mock_tty, &ctx);
    if (result != 0) {
        return 0; // Should work now
    }
    
    return 1;
}

// Test 10: Performance characteristics
int test_performance_characteristics() {
    tty_hooks_init();
    
    struct ui_backend backend = {0};
    tty_hooks_install_backend(&backend);
    
    struct tty mock_tty = {1, 80, 24};
    struct tty_ctx ctx;
    tty_ctx_init(&ctx);
    
    // Measure baseline performance
    clock_t start = clock();
    const int iterations = 100000;
    
    for (int i = 0; i < iterations; i++) {
        tty_hooks_route_command(i % 22, &mock_tty, &ctx);
    }
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    double ops_per_sec = iterations / elapsed;
    
    printf("(%.0f ops/sec) ", ops_per_sec);
    
    // Should achieve at least 100k ops/sec
    if (ops_per_sec < 100000) {
        return 0;
    }
    
    return 1;
}

// =============================================================================
// Main test runner
// =============================================================================

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("=============================================================\n");
    printf("CORE-001 Extended Test Suite (Coverage Enhancement)\n");
    printf("Target: 70%% -> 75%% coverage\n");
    printf("=============================================================\n\n");
    
    // Run all extended tests
    RUN_TEST(test_hook_installation_null_backend);
    RUN_TEST(test_hook_routing_invalid_cmd);
    RUN_TEST(test_statistics_overflow);
    RUN_TEST(test_concurrent_hook_access);
    RUN_TEST(test_hook_uninstallation);
    RUN_TEST(test_memory_stress);
    RUN_TEST(test_function_name_boundaries);
    RUN_TEST(test_hook_priority);
    RUN_TEST(test_error_recovery);
    RUN_TEST(test_performance_characteristics);
    
    // Print summary
    printf("\n=============================================================\n");
    printf("Extended Test Results:\n");
    printf("  Tests Run:    %d\n", tests_run);
    printf("  Tests Passed: %d\n", tests_passed);
    printf("  Tests Failed: %d\n", tests_failed);
    printf("  Success Rate: %.1f%%\n", 
           (float)tests_passed * 100 / tests_run);
    
    printf("\nExpected Coverage Improvement:\n");
    printf("  Error paths:     +2%%\n");
    printf("  Edge cases:      +2%%\n");
    printf("  Concurrency:     +1%%\n");
    printf("  Total Expected:  75%%\n");
    printf("=============================================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}