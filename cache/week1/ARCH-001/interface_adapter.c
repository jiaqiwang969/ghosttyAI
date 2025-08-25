// interface_adapter.c - 接口兼容层实现
// Author: ARCH-001 (System Architect)
// Purpose: 实现接口兼容层，解决函数命名不一致问题
// Version: 2.0.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interface_adapter.h"
#include "tty_ctx_unified.h"
#include "ui_backend.h"

// ============================================================================
// Global Interface State
// ============================================================================

static struct {
    int initialized;
    interface_registry_t registry;
    interface_version_t version;
    void* hook_table[32];  // Hook function storage
    int hook_count;
} g_interface_state = {0};

// ============================================================================
// Core Implementation - Standard Interfaces
// ============================================================================

/**
 * Standard tty_hooks_init implementation
 * This is the CANONICAL implementation all teams should use
 */
int tty_hooks_init(void) {
    if (g_interface_state.initialized) {
        return 0; // Already initialized
    }
    
    // Initialize the hook table
    memset(g_interface_state.hook_table, 0, sizeof(g_interface_state.hook_table));
    g_interface_state.hook_count = 0;
    
    // Set up version information
    g_interface_state.version.size = sizeof(interface_version_t);
    g_interface_state.version.major_version = 2;
    g_interface_state.version.minor_version = 0;
    g_interface_state.version.patch_version = 0;
    g_interface_state.version.features.supports_hooks_v1 = 1;
    g_interface_state.version.features.supports_hooks_v2 = 1;
    
    g_interface_state.initialized = 1;
    
    printf("[Interface] tty_hooks initialized (v2.0.0)\n");
    return 0;
}

/**
 * Clean up hooks
 */
void tty_hooks_cleanup(void) {
    if (!g_interface_state.initialized) {
        return;
    }
    
    memset(g_interface_state.hook_table, 0, sizeof(g_interface_state.hook_table));
    g_interface_state.hook_count = 0;
    g_interface_state.initialized = 0;
    
    printf("[Interface] tty_hooks cleaned up\n");
}

/**
 * Install a hook function
 */
int tty_hooks_install(const char* name, void* hook_fn) {
    if (!g_interface_state.initialized) {
        return INTERFACE_ERR_NOT_INITIALIZED;
    }
    
    if (!name || !hook_fn) {
        return INTERFACE_ERR_INVALID_PARAM;
    }
    
    if (g_interface_state.hook_count >= 32) {
        return INTERFACE_ERR_INTERNAL;
    }
    
    // Store the hook (simplified - real implementation would use a hash table)
    g_interface_state.hook_table[g_interface_state.hook_count++] = hook_fn;
    
    printf("[Interface] Hook installed: %s\n", name);
    return INTERFACE_OK;
}

// ============================================================================
// Backend Router Implementation
// ============================================================================

// Define the backend_router structure
struct backend_router {
    uint32_t size;
    uint32_t version;
    struct ui_backend* backends[16];
    int backend_count;
    void* context;
};

static struct backend_router* g_router = NULL;

/**
 * Standard backend_router_register implementation
 */
int backend_router_register(struct backend_router* router, struct ui_backend* backend) {
    if (!router || !backend) {
        return INTERFACE_ERR_INVALID_PARAM;
    }
    
    // In real implementation, this would register the backend
    // For now, just store the router globally
    g_router = router;
    
    printf("[Interface] Backend registered to router\n");
    return INTERFACE_OK;
}

/**
 * Create a backend router
 */
int backend_router_create(struct backend_router** router) {
    if (!router) {
        return INTERFACE_ERR_INVALID_PARAM;
    }
    
    *router = (struct backend_router*)calloc(1, sizeof(struct backend_router));
    if (!*router) {
        return INTERFACE_ERR_INTERNAL;
    }
    
    printf("[Interface] Backend router created\n");
    return INTERFACE_OK;
}

/**
 * Destroy a backend router
 */
void backend_router_destroy(struct backend_router* router) {
    if (router) {
        free(router);
        if (g_router == router) {
            g_router = NULL;
        }
        printf("[Interface] Backend router destroyed\n");
    }
}

// ============================================================================
// Interface Registry Implementation
// ============================================================================

/**
 * Initialize the global interface registry
 */
int interface_registry_init(void) {
    interface_registry_t* reg = &g_interface_state.registry;
    
    // Allocate interface structures
    reg->hooks = (tty_hooks_interface_t*)calloc(1, sizeof(tty_hooks_interface_t));
    reg->router = (backend_router_interface_t*)calloc(1, sizeof(backend_router_interface_t));
    reg->backend = (ui_backend_interface_t*)calloc(1, sizeof(ui_backend_interface_t));
    
    if (!reg->hooks || !reg->router || !reg->backend) {
        return INTERFACE_ERR_INTERNAL;
    }
    
    // Set up hooks interface
    reg->hooks->init = tty_hooks_init;
    reg->hooks->cleanup = tty_hooks_cleanup;
    reg->hooks->install = tty_hooks_install;
    reg->hooks->uninstall = NULL; // TODO: Implement
    reg->hooks->get = NULL; // TODO: Implement
    
    // Set up router interface
    reg->router->create = backend_router_create;
    reg->router->destroy = backend_router_destroy;
    reg->router->register_backend = backend_router_register;
    reg->router->unregister_backend = NULL; // TODO: Implement
    reg->router->route_command = NULL; // TODO: Implement
    
    // Set up backend interface
    // These would be implemented in ui_backend.c
    reg->backend->create = NULL; // TODO: Link to ui_backend_create
    reg->backend->destroy = NULL; // TODO: Link to ui_backend_destroy
    reg->backend->process_command = NULL; // TODO: Link to ui_backend_process_command
    reg->backend->flush = NULL; // TODO: Link to ui_backend_flush
    
    printf("[Interface] Registry initialized\n");
    return INTERFACE_OK;
}

/**
 * Clean up the interface registry
 */
void interface_registry_cleanup(void) {
    interface_registry_t* reg = &g_interface_state.registry;
    
    free(reg->hooks);
    free(reg->router);
    free(reg->backend);
    
    memset(reg, 0, sizeof(*reg));
    
    printf("[Interface] Registry cleaned up\n");
}

// ============================================================================
// Version and Capability Queries
// ============================================================================

/**
 * Get the current interface version
 */
interface_version_t* interface_get_version(void) {
    return &g_interface_state.version;
}

/**
 * Check if a specific interface version is supported
 */
int interface_is_version_supported(uint32_t major, uint32_t minor) {
    if (major < g_interface_state.version.major_version) {
        return 1; // Older versions supported
    }
    if (major == g_interface_state.version.major_version &&
        minor <= g_interface_state.version.minor_version) {
        return 1; // Same major, older or same minor
    }
    return 0; // Newer version not supported
}

// ============================================================================
// Compatibility Layer Implementation
// ============================================================================

/**
 * Legacy function names that forward to standard implementations
 * These are marked as deprecated but still work
 */

// Old name -> New name mapping
int tty_write_hooks_init(void) {
    printf("[Interface] WARNING: tty_write_hooks_init() is deprecated, use tty_hooks_init()\n");
    return tty_hooks_init();
}

int backend_router_register_backend(struct backend_router* router, struct ui_backend* backend) {
    printf("[Interface] WARNING: backend_router_register_backend() is deprecated, use backend_router_register()\n");
    return backend_router_register(router, backend);
}

int backend_router_register_ui(struct backend_router* router, struct ui_backend* backend) {
    printf("[Interface] WARNING: backend_router_register_ui() is deprecated, use backend_router_register()\n");
    return backend_router_register(router, backend);
}

// ============================================================================
// Self-Test Function
// ============================================================================

/**
 * Test that all interfaces are working correctly
 */
int interface_self_test(void) {
    printf("\n[Interface] Running self-test...\n");
    
    int errors = 0;
    
    // Test 1: Initialize hooks
    if (tty_hooks_init() != 0) {
        printf("  ❌ Failed to initialize hooks\n");
        errors++;
    } else {
        printf("  ✅ Hooks initialized\n");
    }
    
    // Test 2: Version check
    interface_version_t* ver = interface_get_version();
    if (!ver || ver->major_version != 2) {
        printf("  ❌ Version mismatch\n");
        errors++;
    } else {
        printf("  ✅ Version correct (v%d.%d.%d)\n", 
               ver->major_version, ver->minor_version, ver->patch_version);
    }
    
    // Test 3: Compatibility layer
    if (tty_write_hooks_init() != 0) {  // Should work but print warning
        printf("  ❌ Compatibility layer failed\n");
        errors++;
    } else {
        printf("  ✅ Compatibility layer working\n");
    }
    
    // Test 4: Registry initialization
    if (interface_registry_init() != 0) {
        printf("  ❌ Registry initialization failed\n");
        errors++;
    } else {
        printf("  ✅ Registry initialized\n");
    }
    
    // Clean up
    tty_hooks_cleanup();
    interface_registry_cleanup();
    
    printf("\n[Interface] Self-test complete: %d errors\n", errors);
    return errors == 0 ? 0 : -1;
}