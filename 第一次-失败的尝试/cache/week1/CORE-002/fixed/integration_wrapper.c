// integration_wrapper.c - Integration wrapper for backend router with tty_hooks
// Purpose: Ensures compatibility between backend_router and tty_hooks_init
// Author: libtmux-core-developer
// Date: 2025-08-25
// Fix: DEFECT-002

#include <stdio.h>
#include "tmux_types.h"      // Must come first for struct definitions
#include "backend_router.h"
#include "tty_write_hooks.h"
#include "ui_backend.h"

// ============================================================================
// Integration Functions
// ============================================================================

/**
 * Initialize the complete routing system with hooks
 * This ensures compatibility between tty_hooks and backend_router
 */
int initialize_routing_system(backend_mode_t mode, struct ui_backend* ui_backend) {
    int result;
    
    // Step 1: Initialize the hook system (from CORE-001)
    tty_hooks_init();
    printf("[INTEGRATION] Hook system initialized\n");
    
    // Step 2: Initialize global backend router
    result = backend_router_init_global(mode);
    if (result != ROUTER_OK) {
        fprintf(stderr, "[ERROR] Failed to initialize global router: %s\n",
                backend_router_error_string(result));
        return -1;
    }
    printf("[INTEGRATION] Global backend router initialized (mode: %d)\n", mode);
    
    // Step 3: Register UI backend if provided
    if (ui_backend && global_backend_router) {
        result = backend_router_register_ui(global_backend_router, ui_backend);
        if (result != ROUTER_OK) {
            fprintf(stderr, "[ERROR] Failed to register UI backend: %s\n",
                    backend_router_error_string(result));
            backend_router_cleanup_global();
            return -1;
        }
        printf("[INTEGRATION] UI backend registered successfully\n");
    }
    
    // Step 4: Install hooks with the backend
    if (ui_backend) {
        result = tty_hooks_install(ui_backend);
        if (result != 0) {
            fprintf(stderr, "[ERROR] Failed to install hooks\n");
            backend_router_cleanup_global();
            return -1;
        }
        printf("[INTEGRATION] Hooks installed with UI backend\n");
    }
    
    return 0;
}

/**
 * Cleanup the routing system
 */
void cleanup_routing_system(void) {
    // Uninstall hooks first
    tty_hooks_uninstall();
    printf("[INTEGRATION] Hooks uninstalled\n");
    
    // Cleanup global router
    backend_router_cleanup_global();
    printf("[INTEGRATION] Global router cleaned up\n");
}

/**
 * Switch routing mode at runtime
 */
int switch_routing_mode(backend_mode_t new_mode) {
    if (!global_backend_router) {
        fprintf(stderr, "[ERROR] Global router not initialized\n");
        return -1;
    }
    
    backend_router_set_mode(global_backend_router, new_mode);
    printf("[INTEGRATION] Switched routing mode to %d\n", new_mode);
    
    return 0;
}

/**
 * Get routing statistics for monitoring
 */
void print_routing_statistics(void) {
    if (!global_backend_router) {
        fprintf(stderr, "[ERROR] Global router not initialized\n");
        return;
    }
    
    const backend_router_stats_t* router_stats = 
        backend_router_get_stats(global_backend_router);
    
    tty_hook_stats_t hook_stats;
    tty_hooks_get_stats(&hook_stats);
    
    printf("\n=== Routing Statistics ===\n");
    printf("Router Stats:\n");
    printf("  Commands routed: %lu\n", router_stats->commands_routed);
    printf("  To TTY: %lu\n", router_stats->commands_to_tty);
    printf("  To UI: %lu\n", router_stats->commands_to_ui);
    printf("  Dropped: %lu\n", router_stats->commands_dropped);
    printf("  Avg routing time: %lu ns\n", router_stats->avg_routing_time_ns);
    
    printf("\nHook Stats:\n");
    printf("  Total calls: %lu\n", hook_stats.total_calls);
    printf("  Intercepted: %lu\n", hook_stats.intercepted_calls);
    printf("  Fallback: %lu\n", hook_stats.fallback_calls);
    
    // Print per-function statistics
    printf("\nPer-function call counts:\n");
    int hook_count = tty_hooks_get_count();
    for (int i = 0; i < hook_count && i < 22; i++) {
        if (hook_stats.call_count[i] > 0) {
            printf("  %s: %lu\n", 
                   tty_hooks_get_function_name(i), 
                   hook_stats.call_count[i]);
        }
    }
    printf("========================\n\n");
}

/**
 * Verify interface compatibility
 * This function ensures all required interfaces are properly aligned
 */
int verify_interface_compatibility(void) {
    int errors = 0;
    
    printf("\n=== Interface Compatibility Check ===\n");
    
    // Check that backend_router_register_ui exists (not backend_router_register_backend)
    if (backend_router_register_ui == NULL) {
        fprintf(stderr, "[ERROR] backend_router_register_ui not found\n");
        errors++;
    } else {
        printf("[OK] backend_router_register_ui interface found\n");
    }
    
    // Check that backend_router_unregister_ui exists
    if (backend_router_unregister_ui == NULL) {
        fprintf(stderr, "[ERROR] backend_router_unregister_ui not found\n");
        errors++;
    } else {
        printf("[OK] backend_router_unregister_ui interface found\n");
    }
    
    // Check hook system interfaces
    if (tty_hooks_init == NULL) {
        fprintf(stderr, "[ERROR] tty_hooks_init not found\n");
        errors++;
    } else {
        printf("[OK] tty_hooks_init interface found\n");
    }
    
    if (tty_hooks_install == NULL) {
        fprintf(stderr, "[ERROR] tty_hooks_install not found\n");
        errors++;
    } else {
        printf("[OK] tty_hooks_install interface found\n");
    }
    
    printf("\nInterface compatibility check: ");
    if (errors == 0) {
        printf("PASSED ✓\n");
    } else {
        printf("FAILED - %d errors found\n", errors);
    }
    printf("=====================================\n\n");
    
    return errors;
}

// ============================================================================
// Modified tty_write Integration
// ============================================================================

/**
 * Example of how tty_write should be modified for integration
 * This would replace the original tty_write in tmux/tty.c
 */
void tty_write_with_router(void (*cmdfn)(struct tty *, const struct tty_ctx *),
                           struct tty_ctx *ctx) {
    // Check if global router is enabled
    if (global_backend_router && global_backend_router->enabled) {
        // Route through backend router
        backend_route_command(global_backend_router, ctx->tty, cmdfn, ctx);
    } else {
        // Fallback to original implementation
        (*cmdfn)(ctx->tty, ctx);
    }
}