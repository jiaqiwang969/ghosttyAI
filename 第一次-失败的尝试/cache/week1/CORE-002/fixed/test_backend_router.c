// test_backend_router.c - Comprehensive tests for backend router
// Purpose: Validate backend router functionality and thread safety
// Author: CORE-002 (libtmux-core-developer)
// Date: 2025-08-25

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdatomic.h>
#include "tmux_types.h"
#include "backend_router.h"
#include "ui_backend.h"
#include "tty_write_hooks.h"

// Test context structure
typedef struct {
    int value;
    char message[256];
} test_ctx_t;

// Test counters
static _Atomic int tty_call_count = 0;
static _Atomic int ui_call_count = 0;
// static _Atomic int error_count = 0;  // Currently unused, commented out

// Mock TTY command function
void mock_tty_cmd(struct tty* tty, const struct tty_ctx* ctx) {
    (void)tty;  // Suppress unused parameter warning
    (void)ctx;  // Suppress unused parameter warning
    atomic_fetch_add(&tty_call_count, 1);
}

// Mock UI command function
void mock_ui_cmd(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)backend;  // Suppress unused parameter warning
    (void)ctx;      // Suppress unused parameter warning
    atomic_fetch_add(&ui_call_count, 1);
}

// Mock UI backend operations
static ui_backend_ops_t mock_ui_ops = {
    .size = sizeof(ui_backend_ops_t),
    .version = UI_BACKEND_ABI_VERSION,
    .cmd_cell = mock_ui_cmd,
    .cmd_cells = mock_ui_cmd,
    .cmd_clearscreen = mock_ui_cmd,
};

// Mock UI backend
static struct ui_backend mock_backend = {
    .size = sizeof(struct ui_backend),
    .version = UI_BACKEND_ABI_VERSION,
    .type = UI_BACKEND_TEST,
    .ops = &mock_ui_ops,
};

// ============================================================================
// Test Functions
// ============================================================================

// Test 1: Router creation and destruction
void test_router_lifecycle(void) {
    printf("Test 1: Router lifecycle...\n");
    
    backend_router_t* router = backend_router_create(BACKEND_MODE_TTY);
    assert(router != NULL);
    assert(router->mode == BACKEND_MODE_TTY);
    assert(router->enabled == false);
    assert(router->ui_backend == NULL);
    
    backend_router_destroy(router);
    printf("  ✓ Router lifecycle test passed\n");
}

// Test 2: Mode switching
void test_mode_switching(void) {
    printf("Test 2: Mode switching...\n");
    
    backend_router_t* router = backend_router_create(BACKEND_MODE_TTY);
    assert(router->mode == BACKEND_MODE_TTY);
    
    backend_router_set_mode(router, BACKEND_MODE_UI);
    assert(router->mode == BACKEND_MODE_UI);
    
    backend_router_set_mode(router, BACKEND_MODE_HYBRID);
    assert(router->mode == BACKEND_MODE_HYBRID);
    
    backend_router_destroy(router);
    printf("  ✓ Mode switching test passed\n");
}

// Test 3: UI backend registration
void test_backend_registration(void) {
    printf("Test 3: Backend registration...\n");
    
    backend_router_t* router = backend_router_create(BACKEND_MODE_UI);
    
    // Register UI backend
    int result = backend_router_register_ui(router, &mock_backend);
    assert(result == ROUTER_OK);
    assert(router->ui_backend == &mock_backend);
    assert(router->enabled == true);
    
    // Try to register again (should fail)
    result = backend_router_register_ui(router, &mock_backend);
    assert(result == ROUTER_ERR_ALREADY_REGISTERED);
    
    // Unregister
    backend_router_unregister_ui(router);
    assert(router->ui_backend == NULL);
    assert(router->enabled == false);
    
    backend_router_destroy(router);
    printf("  ✓ Backend registration test passed\n");
}

// Test 4: Command routing in different modes
void test_command_routing(void) {
    printf("Test 4: Command routing...\n");
    
    backend_router_t* router = backend_router_create(BACKEND_MODE_TTY);
    struct tty_ctx ctx = {0};
    
    // Reset counters
    atomic_store(&tty_call_count, 0);
    atomic_store(&ui_call_count, 0);
    
    // Test TTY mode
    backend_route_command(router, NULL, mock_tty_cmd, &ctx);
    assert(atomic_load(&tty_call_count) == 1);
    assert(atomic_load(&ui_call_count) == 0);
    
    // Switch to UI mode and register backend
    backend_router_set_mode(router, BACKEND_MODE_UI);
    backend_router_register_ui(router, &mock_backend);
    backend_router_add_mapping(router, "test", mock_tty_cmd, mock_ui_cmd, 0);
    
    backend_route_command(router, NULL, mock_tty_cmd, &ctx);
    assert(atomic_load(&tty_call_count) == 1);  // Should not increase
    assert(atomic_load(&ui_call_count) == 1);
    
    // Test hybrid mode
    backend_router_set_mode(router, BACKEND_MODE_HYBRID);
    backend_route_command(router, NULL, mock_tty_cmd, &ctx);
    assert(atomic_load(&tty_call_count) == 2);
    assert(atomic_load(&ui_call_count) == 2);
    
    backend_router_destroy(router);
    printf("  ✓ Command routing test passed\n");
}

// Test 5: Statistics collection
void test_statistics(void) {
    printf("Test 5: Statistics collection...\n");
    
    backend_router_t* router = backend_router_create(BACKEND_MODE_UI);
    backend_router_register_ui(router, &mock_backend);
    backend_router_set_metrics(router, true);
    
    struct tty_ctx ctx = {0};
    
    // Generate some routing activity
    for (int i = 0; i < 100; i++) {
        backend_route_command(router, NULL, tty_cmd_cell, &ctx);
    }
    
    const backend_router_stats_t* stats = backend_router_get_stats(router);
    assert(stats != NULL);
    assert(stats->commands_routed == 100);
    assert(stats->min_routing_time_ns < stats->max_routing_time_ns);
    
    // Reset stats
    backend_router_reset_stats(router);
    stats = backend_router_get_stats(router);
    assert(stats->commands_routed == 0);
    
    backend_router_destroy(router);
    printf("  ✓ Statistics test passed\n");
}

// Test 6: Thread safety
typedef struct {
    backend_router_t* router;
    int thread_id;
    int iterations;
} thread_test_data_t;

void* thread_worker(void* arg) {
    thread_test_data_t* data = (thread_test_data_t*)arg;
    struct tty_ctx ctx = {0};
    
    for (int i = 0; i < data->iterations; i++) {
        // Randomly switch modes
        if (i % 10 == 0) {
            backend_mode_t mode = (backend_mode_t)(rand() % 3);
            backend_router_set_mode(data->router, mode);
        }
        
        // Route commands
        backend_route_command(data->router, NULL, mock_tty_cmd, &ctx);
        
        // Occasionally check stats
        if (i % 50 == 0) {
            backend_router_get_stats(data->router);
        }
    }
    
    return NULL;
}

void test_thread_safety(void) {
    printf("Test 6: Thread safety...\n");
    
    backend_router_t* router = backend_router_create(BACKEND_MODE_HYBRID);
    backend_router_register_ui(router, &mock_backend);
    backend_router_add_mapping(router, "test", mock_tty_cmd, mock_ui_cmd, 0);
    
    const int num_threads = 10;
    const int iterations = 1000;
    pthread_t threads[num_threads];
    thread_test_data_t thread_data[num_threads];
    
    // Start threads
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].router = router;
        thread_data[i].thread_id = i;
        thread_data[i].iterations = iterations;
        pthread_create(&threads[i], NULL, thread_worker, &thread_data[i]);
    }
    
    // Wait for threads
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Verify stats
    const backend_router_stats_t* stats = backend_router_get_stats(router);
    assert(stats->commands_routed == num_threads * iterations);
    
    backend_router_destroy(router);
    printf("  ✓ Thread safety test passed\n");
}

// Test 7: Recording and replay
void test_recording_replay(void) {
    printf("Test 7: Recording and replay...\n");
    
    backend_router_t* router = backend_router_create(BACKEND_MODE_UI);
    backend_router_register_ui(router, &mock_backend);
    
    // Start recording
    backend_router_start_recording(router, 10);
    
    struct tty_ctx ctx = {0};
    
    // Record some commands
    for (int i = 0; i < 5; i++) {
        ctx.ocy = i;  // Vary context
        backend_route_command(router, NULL, tty_cmd_cell, &ctx);
    }
    
    // Stop recording
    uint32_t count = 0;
    recorded_command_t* commands = backend_router_stop_recording(router, &count);
    assert(commands != NULL);
    assert(count == 5);
    
    // Reset counters
    atomic_store(&ui_call_count, 0);
    
    // Replay commands
    backend_router_replay_commands(router, commands, count);
    
    // Verify replay worked
    const backend_router_stats_t* stats = backend_router_get_stats(router);
    assert(stats->commands_routed >= 10);  // 5 original + 5 replay
    
    free(commands);
    backend_router_destroy(router);
    printf("  ✓ Recording/replay test passed\n");
}

// Test 8: Global router
void test_global_router(void) {
    printf("Test 8: Global router...\n");
    
    // Initialize global router
    int result = backend_router_init_global(BACKEND_MODE_TTY);
    assert(result == ROUTER_OK);
    assert(global_backend_router != NULL);
    
    // Try to initialize again (should fail)
    result = backend_router_init_global(BACKEND_MODE_UI);
    assert(result == ROUTER_ERR_ALREADY_REGISTERED);
    
    // Cleanup
    backend_router_cleanup_global();
    assert(global_backend_router == NULL);
    
    printf("  ✓ Global router test passed\n");
}

// Test 9: Error handling
void test_error_handling(void) {
    printf("Test 9: Error handling...\n");
    
    backend_router_t* router = backend_router_create(BACKEND_MODE_UI);
    
    // Test various error conditions
    backend_router_register_ui(NULL, &mock_backend);  // NULL router
    backend_router_set_mode(NULL, BACKEND_MODE_HYBRID);  // NULL router
    backend_route_command(NULL, NULL, mock_tty_cmd, NULL);  // NULL router
    
    // Get error string
    const char* err_str = backend_router_error_string(ROUTER_ERR_NO_BACKEND);
    assert(err_str != NULL);
    assert(strlen(err_str) > 0);
    
    backend_router_destroy(router);
    printf("  ✓ Error handling test passed\n");
}

// Test 10: Hybrid mode configuration
void test_hybrid_mode(void) {
    printf("Test 10: Hybrid mode configuration...\n");
    
    backend_router_t* router = backend_router_create(BACKEND_MODE_HYBRID);
    backend_router_register_ui(router, &mock_backend);
    backend_router_add_mapping(router, "test", mock_tty_cmd, mock_ui_cmd, 0);
    
    hybrid_mode_config_t config = {
        .prefer_ui = true,
        .sync_output = true,
        .ui_delay_ms = 0
    };
    
    backend_router_configure_hybrid(router, &config);
    
    // Reset counters
    atomic_store(&tty_call_count, 0);
    atomic_store(&ui_call_count, 0);
    
    struct tty_ctx ctx = {0};
    backend_route_command(router, NULL, mock_tty_cmd, &ctx);
    
    // Both should be called in sync mode
    assert(atomic_load(&tty_call_count) == 1);
    assert(atomic_load(&ui_call_count) == 1);
    
    backend_router_destroy(router);
    printf("  ✓ Hybrid mode test passed\n");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char* argv[]) {
    (void)argc;  // Suppress unused parameter warning
    (void)argv;  // Suppress unused parameter warning
    
    printf("==============================================\n");
    printf("Backend Router Test Suite\n");
    printf("==============================================\n\n");
    
    // Run all tests
    test_router_lifecycle();
    test_mode_switching();
    test_backend_registration();
    test_command_routing();
    test_statistics();
    test_thread_safety();
    test_recording_replay();
    test_global_router();
    test_error_handling();
    test_hybrid_mode();
    
    printf("\n==============================================\n");
    printf("All tests passed successfully! ✓\n");
    printf("==============================================\n");
    
    return 0;
}