// backend_router.h - Backend Routing System for libtmuxcore
// Purpose: Route tty commands to appropriate backend (TTY or UI)
// Author: ARCH-001 (System Architect)
// Date: 2025-08-25
// Version: 1.0.0

#ifndef BACKEND_ROUTER_H
#define BACKEND_ROUTER_H

#include <stdint.h>
#include <stdbool.h>
#include "ui_backend.h"

// Forward declarations
struct tty;
struct tty_ctx;

// ============================================================================
// Router Configuration
// ============================================================================

typedef enum {
    BACKEND_MODE_TTY,           // Traditional TTY output (default)
    BACKEND_MODE_UI,            // UI backend (Ghostty)
    BACKEND_MODE_HYBRID,        // Both (for debugging/transition)
} backend_mode_t;

// Router statistics
typedef struct backend_router_stats {
    uint32_t size;              // MUST be first field
    
    // Command routing counts
    uint64_t commands_routed;
    uint64_t commands_to_tty;
    uint64_t commands_to_ui;
    uint64_t commands_dropped;
    
    // Performance metrics
    uint64_t total_routing_time_ns;
    uint64_t min_routing_time_ns;
    uint64_t max_routing_time_ns;
    uint64_t avg_routing_time_ns;
    
    // Error counts
    uint64_t routing_errors;
    uint64_t backend_errors;
} backend_router_stats_t;

// ============================================================================
// Command Mapping Table
// ============================================================================

// Function pointer types
typedef void (*tty_cmd_fn)(struct tty*, const struct tty_ctx*);
typedef void (*ui_cmd_fn)(struct ui_backend*, const struct tty_ctx*);

// Command mapping entry
typedef struct {
    const char* name;           // Command name for logging
    tty_cmd_fn tty_fn;         // Traditional TTY function
    ui_cmd_fn ui_fn;           // UI backend function
    uint32_t flags;            // Command-specific flags
} backend_cmd_mapping_t;

// Command flags
typedef enum {
    CMD_FLAG_BATCHABLE   = 1 << 0,  // Can be batched in frames
    CMD_FLAG_URGENT      = 1 << 1,  // Should bypass batching
    CMD_FLAG_STATEFUL    = 1 << 2,  // Modifies persistent state
    CMD_FLAG_VISUAL      = 1 << 3,  // Visual output command
    CMD_FLAG_CONTROL     = 1 << 4,  // Control/meta command
} backend_cmd_flags_t;

// ============================================================================
// Router Structure
// ============================================================================

typedef struct backend_router {
    uint32_t size;              // MUST be first field
    uint32_t version;
    
    // Configuration
    backend_mode_t mode;
    bool enabled;
    
    // Backends
    struct ui_backend* ui_backend;
    
    // Command mapping table
    const backend_cmd_mapping_t* cmd_map;
    uint32_t cmd_map_size;
    
    // Statistics
    backend_router_stats_t stats;
    
    // Performance monitoring
    bool collect_metrics;
    void (*on_metric)(const char* cmd_name, uint64_t time_ns, void* user);
    void* metric_user_data;
    
    // Error handling
    void (*on_error)(const char* error, void* user);
    void* error_user_data;
} backend_router_t;

// ============================================================================
// Router Management Functions
// ============================================================================

// Create a new router instance
backend_router_t* backend_router_create(backend_mode_t initial_mode);

// Destroy a router instance
void backend_router_destroy(backend_router_t* router);

// Set routing mode
void backend_router_set_mode(backend_router_t* router, backend_mode_t mode);

// Register UI backend with router
int backend_router_register_ui(backend_router_t* router, 
                               struct ui_backend* backend);

// Unregister UI backend
void backend_router_unregister_ui(backend_router_t* router);

// ============================================================================
// Command Routing Functions
// ============================================================================

// Main routing function - replaces direct tty_cmd_* calls
void backend_route_command(backend_router_t* router,
                          struct tty* tty,
                          tty_cmd_fn cmd_fn,
                          const struct tty_ctx* ctx);

// Check if command should be routed to UI
bool backend_should_route_to_ui(const backend_router_t* router,
                                tty_cmd_fn cmd_fn);

// Get command name for logging
const char* backend_get_command_name(const backend_router_t* router,
                                     tty_cmd_fn cmd_fn);

// ============================================================================
// Performance Monitoring
// ============================================================================

// Enable/disable metrics collection
void backend_router_set_metrics(backend_router_t* router, bool enable);

// Get current statistics
const backend_router_stats_t* backend_router_get_stats(const backend_router_t* router);

// Reset statistics
void backend_router_reset_stats(backend_router_t* router);

// ============================================================================
// Command Mapping Initialization
// ============================================================================

// Initialize default command mapping table
void backend_router_init_default_mappings(backend_router_t* router);

// Add custom command mapping
void backend_router_add_mapping(backend_router_t* router,
                                const char* name,
                                tty_cmd_fn tty_fn,
                                ui_cmd_fn ui_fn,
                                uint32_t flags);

// Remove command mapping
void backend_router_remove_mapping(backend_router_t* router,
                                   tty_cmd_fn tty_fn);

// ============================================================================
// Integration with tty_write
// ============================================================================

/*
 * Integration Pattern:
 * 
 * In tty.c, modify tty_write() to use the router:
 * 
 * void tty_write(void (*cmdfn)(struct tty *, const struct tty_ctx *),
 *               struct tty_ctx *ctx) {
 *     if (global_router && global_router->enabled) {
 *         backend_route_command(global_router, ctx->tty, cmdfn, ctx);
 *     } else {
 *         // Original implementation
 *         (*cmdfn)(ctx->tty, ctx);
 *     }
 * }
 */

// Global router instance (single router per process)
extern backend_router_t* global_backend_router;

// Initialize global router
int backend_router_init_global(backend_mode_t mode);

// Cleanup global router
void backend_router_cleanup_global(void);

// ============================================================================
// Hybrid Mode Support
// ============================================================================

// In hybrid mode, commands go to both backends
typedef struct {
    bool prefer_ui;             // Which backend to prioritize
    bool sync_output;           // Wait for both to complete
    uint32_t ui_delay_ms;       // Delay for UI backend (debugging)
} hybrid_mode_config_t;

// Configure hybrid mode behavior
void backend_router_configure_hybrid(backend_router_t* router,
                                     const hybrid_mode_config_t* config);

// ============================================================================
// Debugging and Testing
// ============================================================================

// Command recording for testing
typedef struct {
    tty_cmd_fn cmd_fn;
    struct tty_ctx ctx_copy;   // Deep copy of context
    uint64_t timestamp_ns;
} recorded_command_t;

// Start recording commands
void backend_router_start_recording(backend_router_t* router,
                                    uint32_t max_commands);

// Stop recording and get recorded commands
recorded_command_t* backend_router_stop_recording(backend_router_t* router,
                                                  uint32_t* count);

// Replay recorded commands
void backend_router_replay_commands(backend_router_t* router,
                                    const recorded_command_t* commands,
                                    uint32_t count);

// ============================================================================
// Error Handling
// ============================================================================

typedef enum {
    ROUTER_OK = 0,
    ROUTER_ERR_NO_BACKEND = -1,
    ROUTER_ERR_INVALID_MODE = -2,
    ROUTER_ERR_ALREADY_REGISTERED = -3,
    ROUTER_ERR_COMMAND_NOT_FOUND = -4,
    ROUTER_ERR_BACKEND_FAILED = -5,
} router_error_t;

// Get last error
router_error_t backend_router_get_last_error(const backend_router_t* router);

// Get error string
const char* backend_router_error_string(router_error_t err);

// ============================================================================
// Thread Safety
// ============================================================================

/*
 * Thread Safety Guarantees:
 * 
 * 1. Router is NOT thread-safe by default
 * 2. All operations must be called from the same thread (tmux main thread)
 * 3. In hybrid mode, backends are called sequentially, not in parallel
 * 4. Statistics are updated atomically where possible
 * 
 * For multi-threaded scenarios, external synchronization is required.
 */

#endif /* BACKEND_ROUTER_H */