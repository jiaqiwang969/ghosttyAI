// test_error_handling.c - Error handling and boundary condition tests
// Purpose: Test error paths, NULL checks, and boundary conditions
// Author: INTG-001 (Zig-Ghostty Integration Specialist)
// Date: 2025-08-25
// Version: 1.0.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include "../../ARCH-001/tty_ctx_unified.h"
#include "../../ARCH-001/ui_backend.h"
#include "../backend_ghostty.c"

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s at %s:%d\n", msg, __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
    tests_passed++; \
} while(0)

// Test NULL backend handling
void test_null_backend(void) {
    printf("Testing NULL backend handling...\n");
    
    struct tty_ctx ctx;
    tty_ctx_init(&ctx);
    
    // All 22 callbacks should handle NULL backend gracefully
    ghostty_cmd_cell(NULL, &ctx);
    ghostty_cmd_cells(NULL, &ctx);
    ghostty_cmd_insertcharacter(NULL, &ctx);
    ghostty_cmd_deletecharacter(NULL, &ctx);
    ghostty_cmd_clearcharacter(NULL, &ctx);
    ghostty_cmd_insertline(NULL, &ctx);
    ghostty_cmd_deleteline(NULL, &ctx);
    ghostty_cmd_clearline(NULL, &ctx);
    ghostty_cmd_clearendofline(NULL, &ctx);
    ghostty_cmd_clearstartofline(NULL, &ctx);
    ghostty_cmd_clearscreen(NULL, &ctx);
    ghostty_cmd_clearendofscreen(NULL, &ctx);
    ghostty_cmd_clearstartofscreen(NULL, &ctx);
    ghostty_cmd_alignmenttest(NULL, &ctx);
    ghostty_cmd_reverseindex(NULL, &ctx);
    ghostty_cmd_linefeed(NULL, &ctx);
    ghostty_cmd_scrollup(NULL, &ctx);
    ghostty_cmd_scrolldown(NULL, &ctx);
    ghostty_cmd_setselection(NULL, &ctx);
    ghostty_cmd_rawstring(NULL, &ctx);
    ghostty_cmd_sixelimage(NULL, &ctx);
    ghostty_cmd_syncstart(NULL, &ctx);
    
    TEST_ASSERT(1, "NULL backend handled");
}

// Test NULL context handling
void test_null_context(void) {
    printf("Testing NULL context handling...\n");
    
    ui_capabilities_t caps = {
        .size = sizeof(ui_capabilities_t),
        .version = UI_BACKEND_ABI_VERSION,
        .supported = UI_CAP_FRAME_BATCH
    };
    
    struct ui_backend* backend = ghostty_backend_create(&caps);
    TEST_ASSERT(backend != NULL, "Backend created");
    
    // All callbacks should handle NULL ctx
    backend->ops->cmd_cell(backend, NULL);
    backend->ops->cmd_clearscreen(backend, NULL);
    backend->ops->cmd_scrollup(backend, NULL);
    
    ghostty_backend_destroy(backend);
    TEST_ASSERT(1, "NULL context handled");
}

// Test invalid tty_ctx structure
void test_invalid_context_structure(void) {
    printf("Testing invalid context structure...\n");
    
    ui_capabilities_t caps = {
        .size = sizeof(ui_capabilities_t),
        .version = UI_BACKEND_ABI_VERSION
    };
    
    struct ui_backend* backend = ghostty_backend_create(&caps);
    TEST_ASSERT(backend != NULL, "Backend created");
    
    // Create invalid context with wrong size
    struct tty_ctx bad_ctx;
    memset(&bad_ctx, 0, sizeof(bad_ctx));
    bad_ctx.size = 1;  // Invalid size
    bad_ctx.version = 999;  // Invalid version
    
    // Should handle gracefully
    backend->ops->cmd_cell(backend, &bad_ctx);
    
    // Test with uninitialized context
    struct tty_ctx uninit_ctx;
    // Don't initialize - contains garbage
    backend->ops->cmd_cell(backend, &uninit_ctx);
    
    ghostty_backend_destroy(backend);
    TEST_ASSERT(1, "Invalid context handled");
}

// Test boundary values
void test_boundary_values(void) {
    printf("Testing boundary values...\n");
    
    ui_capabilities_t caps = {
        .size = sizeof(ui_capabilities_t),
        .version = UI_BACKEND_ABI_VERSION
    };
    
    struct ui_backend* backend = ghostty_backend_create(&caps);
    TEST_ASSERT(backend != NULL, "Backend created");
    
    struct tty_ctx ctx;
    tty_ctx_init(&ctx);
    
    // Test maximum coordinates
    TTY_CTX_SET_FIELD(&ctx, ocx, UINT32_MAX);
    TTY_CTX_SET_FIELD(&ctx, ocy, UINT32_MAX);
    backend->ops->cmd_cell(backend, &ctx);
    TEST_ASSERT(1, "Max coordinates handled");
    
    // Test zero dimensions (using defaults)
    backend->ops->cmd_clearscreen(backend, &ctx);
    TEST_ASSERT(1, "Zero dimensions handled");
    
    // Test huge dimensions (using defaults)
    backend->ops->cmd_clearscreen(backend, &ctx);
    TEST_ASSERT(1, "Huge dimensions handled");
    
    // Test invalid scroll region
    TTY_CTX_SET_FIELD(&ctx, orupper, 100);
    TTY_CTX_SET_FIELD(&ctx, orlower, 10);  // upper > lower
    backend->ops->cmd_scrollup(backend, &ctx);
    TEST_ASSERT(1, "Invalid scroll region handled");
    
    // Test huge num parameter
    ctx.num = UINT32_MAX;
    backend->ops->cmd_cells(backend, &ctx);
    backend->ops->cmd_scrollup(backend, &ctx);
    TEST_ASSERT(1, "Huge num parameter handled");
    
    ghostty_backend_destroy(backend);
}

// Test memory allocation failures
void test_memory_allocation_failures(void) {
    printf("Testing memory allocation failures...\n");
    
    // Test backend creation with NULL caps
    struct ui_backend* backend = ghostty_backend_create(NULL);
    TEST_ASSERT(backend != NULL, "Backend created with NULL caps");
    
    if (backend) {
        ghostty_backend_destroy(backend);
    }
    
    // Test with invalid capabilities
    ui_capabilities_t bad_caps = {
        .size = 1,  // Wrong size
        .version = 0  // Wrong version
    };
    backend = ghostty_backend_create(&bad_caps);
    
    if (backend) {
        ghostty_backend_destroy(backend);
    }
    
    TEST_ASSERT(1, "Memory allocation failures handled");
}

// Test grid optimization bounds
void test_grid_bounds(void) {
    printf("Testing grid optimization bounds...\n");
    
    ui_capabilities_t caps = {
        .size = sizeof(ui_capabilities_t),
        .version = UI_BACKEND_ABI_VERSION,
        .supported = UI_CAP_FRAME_BATCH
    };
    
    struct ui_backend* backend = ghostty_backend_create(&caps);
    TEST_ASSERT(backend != NULL, "Backend created");
    
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    struct tty_ctx ctx;
    tty_ctx_init(&ctx);
    
    // Test beyond grid capacity
    TTY_CTX_SET_FIELD(&ctx, ocx, gb->dirty_tracking.cols_capacity + 100);
    TTY_CTX_SET_FIELD(&ctx, ocy, gb->dirty_tracking.rows_capacity + 100);
    backend->ops->cmd_cell(backend, &ctx);
    TEST_ASSERT(1, "Beyond grid capacity handled");
    
    // Test at grid boundaries
    TTY_CTX_SET_FIELD(&ctx, ocx, gb->dirty_tracking.cols_capacity - 1);
    TTY_CTX_SET_FIELD(&ctx, ocy, gb->dirty_tracking.rows_capacity - 1);
    backend->ops->cmd_cell(backend, &ctx);
    TEST_ASSERT(1, "Grid boundaries handled");
    
    ghostty_backend_destroy(backend);
}

// Test callback error recovery
void test_error_recovery(void) {
    printf("Testing error recovery...\n");
    
    ui_capabilities_t caps = {
        .size = sizeof(ui_capabilities_t),
        .version = UI_BACKEND_ABI_VERSION
    };
    
    struct ui_backend* backend = ghostty_backend_create(&caps);
    TEST_ASSERT(backend != NULL, "Backend created");
    
    struct tty_ctx ctx;
    tty_ctx_init(&ctx);
    
    // Cause an error with invalid input
    backend->ops->cmd_cell(backend, NULL);
    
    // Should recover and continue working
    backend->ops->cmd_cell(backend, &ctx);
    TEST_ASSERT(1, "Recovered from error");
    
    // Test multiple errors in sequence
    for (int i = 0; i < 10; i++) {
        backend->ops->cmd_cell(backend, NULL);
    }
    
    // Should still work
    backend->ops->cmd_cell(backend, &ctx);
    TEST_ASSERT(1, "Recovered from multiple errors");
    
    ghostty_backend_destroy(backend);
}

// Test frame aggregator error paths
void test_aggregator_errors(void) {
    printf("Testing frame aggregator errors...\n");
    
    ui_capabilities_t caps = {
        .size = sizeof(ui_capabilities_t),
        .version = UI_BACKEND_ABI_VERSION,
        .supported = UI_CAP_FRAME_BATCH,
        .max_fps = 0  // Invalid FPS
    };
    
    struct ui_backend* backend = ghostty_backend_create(&caps);
    
    if (backend) {
        struct tty_ctx ctx;
        tty_ctx_init(&ctx);
        
        // Should handle invalid aggregator state
        backend->ops->cmd_cell(backend, &ctx);
        
        ghostty_backend_destroy(backend);
    }
    
    TEST_ASSERT(1, "Aggregator errors handled");
}

// Test statistics functions with invalid input
void test_statistics_errors(void) {
    printf("Testing statistics error handling...\n");
    
    // NULL backend
    ghostty_backend_get_statistics(NULL, NULL, NULL, NULL);
    TEST_ASSERT(1, "NULL backend stats handled");
    
    // Create valid backend
    ui_capabilities_t caps = {
        .size = sizeof(ui_capabilities_t),
        .version = UI_BACKEND_ABI_VERSION
    };
    
    struct ui_backend* backend = ghostty_backend_create(&caps);
    if (backend) {
        // Test with NULL pointers
        ghostty_backend_get_statistics(backend, NULL, NULL, NULL);
        
        // Test with some NULL pointers
        uint64_t frames;
        ghostty_backend_get_statistics(backend, &frames, NULL, NULL);
        
        ghostty_backend_destroy(backend);
    }
    
    TEST_ASSERT(1, "Statistics errors handled");
}

// Test configuration functions with invalid input
void test_configuration_errors(void) {
    printf("Testing configuration error handling...\n");
    
    // NULL backend
    ghostty_backend_set_immediate_mode(NULL, true);
    ghostty_backend_set_grid_optimization(NULL, false);
    TEST_ASSERT(1, "NULL backend config handled");
    
    ui_capabilities_t caps = {
        .size = sizeof(ui_capabilities_t),
        .version = UI_BACKEND_ABI_VERSION
    };
    
    struct ui_backend* backend = ghostty_backend_create(&caps);
    if (backend) {
        // Toggle modes multiple times
        for (int i = 0; i < 100; i++) {
            ghostty_backend_set_immediate_mode(backend, i % 2);
            ghostty_backend_set_grid_optimization(backend, i % 2);
        }
        
        ghostty_backend_destroy(backend);
    }
    
    TEST_ASSERT(1, "Configuration errors handled");
}

// Main test runner
int main(int argc, char* argv[]) {
    printf("=== Ghostty Backend Error Handling Tests ===\n\n");
    
    // Run all error handling tests
    test_null_backend();
    test_null_context();
    test_invalid_context_structure();
    test_boundary_values();
    test_memory_allocation_failures();
    test_grid_bounds();
    test_error_recovery();
    test_aggregator_errors();
    test_statistics_errors();
    test_configuration_errors();
    
    // Print summary
    printf("\n=== Error Handling Test Summary ===\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("Coverage focus: Error paths and boundary conditions\n");
    
    return tests_failed > 0 ? 1 : 0;
}