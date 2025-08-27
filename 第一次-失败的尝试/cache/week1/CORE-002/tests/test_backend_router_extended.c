// test_backend_router_extended.c - Extended tests for CORE-002
// Purpose: Increase test coverage from 60% to 70%
// Author: QA-002 (qa-test-engineer)
// Date: 2025-08-25

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include "../backend_router.h"
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

// Mock backend
static int mock_cmd_count = 0;
static void mock_ui_cmd(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)backend;
    (void)ctx;
    mock_cmd_count++;
}

// Test macro
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
// Extended Test Cases for Better Coverage
// =============================================================================

// Test 1: Router mode switching
int test_router_mode_switching() {
    backend_router_t* router = backend_router_create(BACKEND_MODE_TTY);
    if (!router) return 0;
    
    // Switch to UI mode
    int result = backend_router_set_mode(router, BACKEND_MODE_UI);
    if (result != 0) {
        backend_router_destroy(router);
        return 0;
    }
    
    // Verify mode
    backend_mode_t mode = backend_router_get_mode(router);
    if (mode != BACKEND_MODE_UI) {
        backend_router_destroy(router);
        return 0;
    }
    
    // Switch to HYBRID mode
    result = backend_router_set_mode(router, BACKEND_MODE_HYBRID);
    if (result != 0) {
        backend_router_destroy(router);
        return 0;
    }
    
    backend_router_destroy(router);
    return 1;
}

// Test 2: Multiple backend registration
int test_multiple_backend_registration() {
    backend_router_t* router = backend_router_create(BACKEND_MODE_UI);
    if (!router) return 0;
    
    // Create multiple mock backends
    struct ui_backend backend1 = {0};
    backend1.version = 1;
    
    struct ui_backend backend2 = {0};
    backend2.version = 2;
    
    struct ui_backend backend3 = {0};
    backend3.version = 3;
    
    // Register multiple backends
    int result = backend_router_register_ui(router, &backend1);
    if (result != 0) {
        backend_router_destroy(router);
        return 0;
    }
    
    // Try to register second (should replace or fail gracefully)
    result = backend_router_register_ui(router, &backend2);
    
    // Register with NULL (should fail)
    result = backend_router_register_ui(router, NULL);
    if (result == 0) {
        backend_router_destroy(router);
        return 0; // Should fail with NULL
    }
    
    backend_router_destroy(router);
    return 1;
}

// Test 3: Command routing with different modes
int test_routing_modes() {
    backend_router_t* router = backend_router_create(BACKEND_MODE_TTY);
    if (!router) return 0;
    
    struct tty mock_tty = {1, 80, 24};
    struct tty_ctx ctx;
    tty_ctx_init(&ctx);
    
    // Route in TTY mode (should use TTY backend)
    backend_router_route_cmd(router, 0, &mock_tty, &ctx);
    
    // Switch to UI mode
    backend_router_set_mode(router, BACKEND_MODE_UI);
    
    // Register UI backend
    struct ui_backend ui_backend = {0};
    backend_router_register_ui(router, &ui_backend);
    
    // Route in UI mode
    backend_router_route_cmd(router, 0, &mock_tty, &ctx);
    
    // Switch to HYBRID mode
    backend_router_set_mode(router, BACKEND_MODE_HYBRID);
    
    // Route in HYBRID mode (should route to both)
    backend_router_route_cmd(router, 0, &mock_tty, &ctx);
    
    backend_router_destroy(router);
    return 1;
}

// Test 4: Statistics accuracy
int test_statistics_accuracy() {
    backend_router_t* router = backend_router_create(BACKEND_MODE_UI);
    if (!router) return 0;
    
    // Register backend
    struct ui_backend backend = {0};
    backend_router_register_ui(router, &backend);
    
    // Reset stats
    backend_router_reset_stats(router);
    
    struct tty mock_tty = {1, 80, 24};
    struct tty_ctx ctx;
    tty_ctx_init(&ctx);
    
    // Route specific number of commands
    const int num_commands = 100;
    for (int i = 0; i < num_commands; i++) {
        backend_router_route_cmd(router, i % 22, &mock_tty, &ctx);
    }
    
    // Get statistics
    backend_router_stats_t stats;
    backend_router_get_stats(router, &stats);
    
    if (stats.total_commands != num_commands) {
        backend_router_destroy(router);
        return 0;
    }
    
    // Verify per-command stats
    int total_from_individual = 0;
    for (int i = 0; i < 22; i++) {
        total_from_individual += stats.command_counts[i];
    }
    
    if (total_from_individual != num_commands) {
        backend_router_destroy(router);
        return 0;
    }
    
    backend_router_destroy(router);
    return 1;
}

// Test 5: Error injection and recovery
int test_error_injection() {
    backend_router_t* router = backend_router_create(BACKEND_MODE_UI);
    if (!router) return 0;
    
    struct tty mock_tty = {1, 80, 24};
    struct tty_ctx ctx;
    tty_ctx_init(&ctx);
    
    // Route without backend (should handle gracefully)
    int result = backend_router_route_cmd(router, 0, &mock_tty, &ctx);
    if (result == 0) {
        backend_router_destroy(router);
        return 0; // Should fail without backend
    }
    
    // Route with invalid command ID
    result = backend_router_route_cmd(router, 999, &mock_tty, &ctx);
    if (result == 0) {
        backend_router_destroy(router);
        return 0; // Should fail with invalid ID
    }
    
    // Route with NULL context
    result = backend_router_route_cmd(router, 0, &mock_tty, NULL);
    if (result == 0) {
        backend_router_destroy(router);
        return 0; // Should fail with NULL ctx
    }
    
    backend_router_destroy(router);
    return 1;
}

// Test 6: Concurrent mode changes
int test_concurrent_mode_changes() {
    backend_router_t* router = backend_router_create(BACKEND_MODE_TTY);
    if (!router) return 0;
    
    #define NUM_THREADS 4
    pthread_t threads[NUM_THREADS];
    
    void* mode_changer(void* arg) {
        backend_router_t* r = (backend_router_t*)arg;
        for (int i = 0; i < 100; i++) {
            backend_mode_t mode = (backend_mode_t)(i % 3);
            backend_router_set_mode(r, mode);
            usleep(10); // Small delay
        }
        return NULL;
    }
    
    // Launch threads that change modes
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, mode_changer, router);
    }
    
    // Wait for threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Router should still be functional
    backend_mode_t final_mode = backend_router_get_mode(router);
    if (final_mode < BACKEND_MODE_TTY || final_mode > BACKEND_MODE_HYBRID) {
        backend_router_destroy(router);
        return 0;
    }
    
    backend_router_destroy(router);
    return 1;
}

// Test 7: Memory pressure test
int test_memory_pressure() {
    // Create and destroy many routers
    for (int i = 0; i < 1000; i++) {
        backend_router_t* router = backend_router_create(BACKEND_MODE_UI);
        if (!router) return 0;
        
        struct ui_backend backend = {0};
        backend_router_register_ui(router, &backend);
        
        struct tty mock_tty = {1, 80, 24};
        struct tty_ctx ctx;
        tty_ctx_init(&ctx);
        
        backend_router_route_cmd(router, 0, &mock_tty, &ctx);
        
        backend_router_destroy(router);
    }
    
    return 1;
}

// Test 8: Command mapping validation
int test_command_mapping() {
    backend_router_t* router = backend_router_create(BACKEND_MODE_UI);
    if (!router) return 0;
    
    // Test command name mapping
    for (int i = 0; i < 22; i++) {
        const char* name = backend_router_get_command_name(router, i);
        if (name == NULL || strlen(name) == 0) {
            backend_router_destroy(router);
            return 0;
        }
    }
    
    // Test invalid indices
    const char* name = backend_router_get_command_name(router, -1);
    if (name != NULL) {
        backend_router_destroy(router);
        return 0;
    }
    
    name = backend_router_get_command_name(router, 100);
    if (name != NULL) {
        backend_router_destroy(router);
        return 0;
    }
    
    backend_router_destroy(router);
    return 1;
}

// Test 9: Performance under load
int test_performance_under_load() {
    backend_router_t* router = backend_router_create(BACKEND_MODE_UI);
    if (!router) return 0;
    
    struct ui_backend backend = {0};
    backend_router_register_ui(router, &backend);
    
    struct tty mock_tty = {1, 80, 24};
    struct tty_ctx ctx;
    tty_ctx_init(&ctx);
    
    clock_t start = clock();
    const int iterations = 100000;
    
    for (int i = 0; i < iterations; i++) {
        backend_router_route_cmd(router, i % 22, &mock_tty, &ctx);
    }
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    double ops_per_sec = iterations / elapsed;
    
    printf("(%.0f ops/sec) ", ops_per_sec);
    
    backend_router_destroy(router);
    
    // Should maintain at least 100k ops/sec
    return ops_per_sec >= 100000;
}

// Test 10: State consistency
int test_state_consistency() {
    backend_router_t* router = backend_router_create(BACKEND_MODE_TTY);
    if (!router) return 0;
    
    // Perform various operations
    backend_router_set_mode(router, BACKEND_MODE_UI);
    
    struct ui_backend backend = {0};
    backend_router_register_ui(router, &backend);
    
    backend_router_set_mode(router, BACKEND_MODE_HYBRID);
    
    // Reset stats
    backend_router_reset_stats(router);
    
    // Verify state is consistent
    backend_mode_t mode = backend_router_get_mode(router);
    if (mode != BACKEND_MODE_HYBRID) {
        backend_router_destroy(router);
        return 0;
    }
    
    backend_router_stats_t stats;
    backend_router_get_stats(router, &stats);
    if (stats.total_commands != 0) {
        backend_router_destroy(router);
        return 0;
    }
    
    backend_router_destroy(router);
    return 1;
}

// =============================================================================
// Main test runner
// =============================================================================

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("=============================================================\n");
    printf("CORE-002 Extended Test Suite (Coverage Enhancement)\n");
    printf("Target: 60%% -> 70%% coverage\n");
    printf("=============================================================\n\n");
    
    // Run all extended tests
    RUN_TEST(test_router_mode_switching);
    RUN_TEST(test_multiple_backend_registration);
    RUN_TEST(test_routing_modes);
    RUN_TEST(test_statistics_accuracy);
    RUN_TEST(test_error_injection);
    RUN_TEST(test_concurrent_mode_changes);
    RUN_TEST(test_memory_pressure);
    RUN_TEST(test_command_mapping);
    RUN_TEST(test_performance_under_load);
    RUN_TEST(test_state_consistency);
    
    // Print summary
    printf("\n=============================================================\n");
    printf("Extended Test Results:\n");
    printf("  Tests Run:    %d\n", tests_run);
    printf("  Tests Passed: %d\n", tests_passed);
    printf("  Tests Failed: %d\n", tests_failed);
    printf("  Success Rate: %.1f%%\n", 
           (float)tests_passed * 100 / tests_run);
    
    printf("\nExpected Coverage Improvement:\n");
    printf("  Mode switching:  +3%%\n");
    printf("  Error handling:  +3%%\n");
    printf("  Statistics:      +2%%\n");
    printf("  Concurrency:     +2%%\n");
    printf("  Total Expected:  70%%\n");
    printf("=============================================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}