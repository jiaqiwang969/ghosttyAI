// interface_compatibility.h - 接口兼容性层
// Author: ARCH-001 (System Architect)
// Purpose: 解决函数接口命名不一致的P0缺陷
// Version: 2.0.0 - FIXED

#ifndef INTERFACE_COMPATIBILITY_H
#define INTERFACE_COMPATIBILITY_H

#include <stdint.h>
#include "tty_ctx_unified.h"
#include "ui_backend.h"

// ============================================================================
// SOLUTION 2: Interface Compatibility Layer
// ============================================================================

/**
 * Problem: Different teams used different function names
 * - tty_write_hooks_init() vs tty_hooks_init()
 * - backend_router_register_backend() vs backend_router_register_ui()
 * 
 * Solution: Provide both interfaces with clear deprecation path
 */

// ============================================================================
// 2A: Function Naming Standardization
// ============================================================================

/**
 * Standard naming convention:
 * module_component_action()
 * 
 * Examples:
 * - tty_hooks_init()
 * - backend_router_register()
 * - ui_backend_create()
 */

// Forward declarations
struct tty_ctx;
struct ui_backend;
struct window_pane;
struct screen;
struct backend_router;

// Function declarations (will be implemented in interface_adapter.c)
int tty_hooks_init(void);
int backend_router_register(struct backend_router*, struct ui_backend*);

// ============================================================================
// 2B: Compatibility Macros for Transition Period
// ============================================================================

/**
 * These macros provide backward compatibility during transition
 * Mark as deprecated to encourage migration
 */

#ifdef __GNUC__
#define DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
#define DEPRECATED(msg) __declspec(deprecated(msg))
#else
#define DEPRECATED(msg)
#endif

// Compatibility function declarations (implemented in interface_adapter.c)
DEPRECATED("Use tty_hooks_init() instead")
int tty_write_hooks_init(void);

DEPRECATED("Use backend_router_register() instead")
int backend_router_register_backend(struct backend_router* router, struct ui_backend* backend);

DEPRECATED("Use backend_router_register() instead")
int backend_router_register_ui(struct backend_router* router, struct ui_backend* backend);

// ============================================================================
// 2C: Interface Version Negotiation
// ============================================================================

/**
 * Runtime interface version detection
 */
typedef struct {
    uint32_t size;              // Structure size for ABI stability
    uint32_t major_version;     // Major version (breaking changes)
    uint32_t minor_version;     // Minor version (additions)
    uint32_t patch_version;     // Patch version (fixes)
    
    // Feature flags
    struct {
        uint32_t supports_hooks_v1 : 1;
        uint32_t supports_hooks_v2 : 1;
        uint32_t supports_router_v1 : 1;
        uint32_t supports_router_v2 : 1;
        uint32_t reserved : 28;
    } features;
    
    // Function pointers for version-specific implementations
    int (*init_hooks)(void);
    int (*register_backend)(void*, void*);
} interface_version_t;

/**
 * Get current interface version
 */
static inline interface_version_t* get_interface_version(void) {
    static interface_version_t version = {
        .size = sizeof(interface_version_t),
        .major_version = 2,
        .minor_version = 0,
        .patch_version = 0,
        .features = {
            .supports_hooks_v1 = 1,  // Backward compatible
            .supports_hooks_v2 = 1,  // New interface
            .supports_router_v1 = 1, // Backward compatible
            .supports_router_v2 = 1, // New interface
        },
        .init_hooks = tty_hooks_init,
        .register_backend = (int(*)(void*,void*))backend_router_register,
    };
    return &version;
}

// ============================================================================
// 2D: Canonical Function Interface Definitions
// ============================================================================

/**
 * These are the CANONICAL interfaces that all components must use
 */

// TTY Hooks Interface
typedef struct {
    int (*init)(void);
    void (*cleanup)(void);
    int (*install)(const char* name, void* hook_fn);
    int (*uninstall)(const char* name);
    void* (*get)(const char* name);
} tty_hooks_interface_t;

// Backend Router Interface
typedef struct {
    int (*create)(struct backend_router**);
    void (*destroy)(struct backend_router*);
    int (*register_backend)(struct backend_router*, struct ui_backend*);
    int (*unregister_backend)(struct backend_router*, struct ui_backend*);
    int (*route_command)(struct backend_router*, void*, const struct tty_ctx*);
} backend_router_interface_t;

// UI Backend Interface
typedef struct {
    struct ui_backend* (*create)(ui_backend_type_t, const ui_capabilities_t*);
    void (*destroy)(struct ui_backend*);
    int (*process_command)(struct ui_backend*, void*, const struct tty_ctx*);
    void (*flush)(struct ui_backend*);
} ui_backend_interface_t;

// ============================================================================
// 2E: Global Interface Registry
// ============================================================================

/**
 * Central registry for all interfaces
 * This ensures consistency across all components
 */
typedef struct {
    tty_hooks_interface_t* hooks;
    backend_router_interface_t* router;
    ui_backend_interface_t* backend;
} interface_registry_t;

/**
 * Get the global interface registry
 */
static inline interface_registry_t* get_interface_registry(void) {
    static interface_registry_t registry = {0};
    static int initialized = 0;
    
    if (!initialized) {
        // Initialize with default implementations
        // These would be set during startup
        initialized = 1;
    }
    
    return &registry;
}

// ============================================================================
// 2F: Interface Validation
// ============================================================================

/**
 * Validate that all required interfaces are present
 * Call this during initialization
 */
static inline int validate_interfaces(void) {
    interface_registry_t* reg = get_interface_registry();
    
    // Check all critical interfaces
    if (!reg->hooks || !reg->hooks->init) {
        return -1; // Missing hooks interface
    }
    if (!reg->router || !reg->router->register_backend) {
        return -1; // Missing router interface
    }
    if (!reg->backend || !reg->backend->create) {
        return -1; // Missing backend interface
    }
    
    return 0; // All interfaces valid
}

// ============================================================================
// 2G: Standard Error Codes
// ============================================================================

/**
 * Unified error codes across all interfaces
 */
typedef enum {
    INTERFACE_OK = 0,
    INTERFACE_ERR_NOT_INITIALIZED = -1,
    INTERFACE_ERR_INVALID_PARAM = -2,
    INTERFACE_ERR_VERSION_MISMATCH = -3,
    INTERFACE_ERR_NOT_SUPPORTED = -4,
    INTERFACE_ERR_ALREADY_EXISTS = -5,
    INTERFACE_ERR_NOT_FOUND = -6,
    INTERFACE_ERR_INTERNAL = -99,
} interface_error_t;

/**
 * Get human-readable error string
 */
static inline const char* interface_error_string(interface_error_t err) {
    switch (err) {
        case INTERFACE_OK: return "Success";
        case INTERFACE_ERR_NOT_INITIALIZED: return "Interface not initialized";
        case INTERFACE_ERR_INVALID_PARAM: return "Invalid parameter";
        case INTERFACE_ERR_VERSION_MISMATCH: return "Version mismatch";
        case INTERFACE_ERR_NOT_SUPPORTED: return "Operation not supported";
        case INTERFACE_ERR_ALREADY_EXISTS: return "Already exists";
        case INTERFACE_ERR_NOT_FOUND: return "Not found";
        case INTERFACE_ERR_INTERNAL: return "Internal error";
        default: return "Unknown error";
    }
}

#endif /* INTERFACE_COMPATIBILITY_H */