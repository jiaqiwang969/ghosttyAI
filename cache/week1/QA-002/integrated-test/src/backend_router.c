// backend_router.c - Backend Router Implementation for libtmuxcore
// Purpose: Route tty commands to appropriate backend (TTY or UI)
// Author: CORE-002 (libtmux-core-developer)
// Date: 2025-08-25
// Version: 1.0.0

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include "backend_router.h"
#include "tty_write_hooks.h"

// ============================================================================
// Thread Safety Implementation
// ============================================================================

// Mutex for thread-safe operations
static pthread_mutex_t router_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_rwlock_t mapping_rwlock = PTHREAD_RWLOCK_INITIALIZER;

// Atomic counters for statistics (lock-free)
#define ATOMIC_INC(counter) __atomic_add_fetch(&(counter), 1, __ATOMIC_RELAXED)
#define ATOMIC_ADD(counter, val) __atomic_add_fetch(&(counter), (val), __ATOMIC_RELAXED)
#define ATOMIC_MIN(counter, val) do { \
    uint64_t old_val = __atomic_load_n(&(counter), __ATOMIC_RELAXED); \
    while (old_val > (val) && !__atomic_compare_exchange_n(&(counter), &old_val, (val), \
           false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)); \
} while(0)
#define ATOMIC_MAX(counter, val) do { \
    uint64_t old_val = __atomic_load_n(&(counter), __ATOMIC_RELAXED); \
    while (old_val < (val) && !__atomic_compare_exchange_n(&(counter), &old_val, (val), \
           false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)); \
} while(0)

// ============================================================================
// Command Mapping Table
// ============================================================================

// Maximum number of command mappings
#define MAX_CMD_MAPPINGS 64

// Internal command mapping structure
typedef struct {
    backend_cmd_mapping_t* mappings;
    uint32_t count;
    uint32_t capacity;
} cmd_map_table_t;

// ============================================================================
// Recording System for Testing
// ============================================================================

typedef struct {
    recorded_command_t* commands;
    uint32_t count;
    uint32_t capacity;
    bool recording;
} recording_state_t;

// ============================================================================
// Extended Router Structure
// ============================================================================

typedef struct backend_router_impl {
    backend_router_t base;          // Public interface
    
    // Thread safety
    pthread_mutex_t lock;           // Router-specific mutex
    atomic_bool enabled_atomic;     // Atomic enabled flag
    
    // Command mapping
    cmd_map_table_t cmd_table;
    
    // Hybrid mode configuration
    hybrid_mode_config_t hybrid_config;
    
    // Recording state
    recording_state_t recording;
    
    // Error state
    router_error_t last_error;
    
    // Performance tracking
    uint64_t start_time_ns;
} backend_router_impl_t;

// ============================================================================
// Global Router Instance
// ============================================================================

backend_router_t* global_backend_router = NULL;

// ============================================================================
// Helper Functions
// ============================================================================

// Get current time in nanoseconds
static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Find command mapping by function pointer
static const backend_cmd_mapping_t* find_mapping(const backend_router_impl_t* impl,
                                                 tty_cmd_fn cmd_fn) {
    for (uint32_t i = 0; i < impl->cmd_table.count; i++) {
        if (impl->cmd_table.mappings[i].tty_fn == cmd_fn) {
            return &impl->cmd_table.mappings[i];
        }
    }
    return NULL;
}

// Copy tty_ctx for recording (deep copy)
static void copy_tty_ctx(struct tty_ctx* dst, const struct tty_ctx* src) {
    // Simple shallow copy for now - extend as needed
    memcpy(dst, src, sizeof(struct tty_ctx));
}

// ============================================================================
// Default Command Mappings
// ============================================================================

// UI backend command implementations (stubs)
static void ui_cmd_cell(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_cell) {
        backend->ops->cmd_cell(backend, ctx);
    }
}

static void ui_cmd_cells(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_cells) {
        backend->ops->cmd_cells(backend, ctx);
    }
}

static void ui_cmd_insertcharacter(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_insertcharacter) {
        backend->ops->cmd_insertcharacter(backend, ctx);
    }
}

static void ui_cmd_deletecharacter(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_deletecharacter) {
        backend->ops->cmd_deletecharacter(backend, ctx);
    }
}

static void ui_cmd_clearcharacter(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_clearcharacter) {
        backend->ops->cmd_clearcharacter(backend, ctx);
    }
}

static void ui_cmd_insertline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_insertline) {
        backend->ops->cmd_insertline(backend, ctx);
    }
}

static void ui_cmd_deleteline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_deleteline) {
        backend->ops->cmd_deleteline(backend, ctx);
    }
}

static void ui_cmd_clearline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_clearline) {
        backend->ops->cmd_clearline(backend, ctx);
    }
}

static void ui_cmd_clearendofline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_clearendofline) {
        backend->ops->cmd_clearendofline(backend, ctx);
    }
}

static void ui_cmd_clearstartofline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_clearstartofline) {
        backend->ops->cmd_clearstartofline(backend, ctx);
    }
}

static void ui_cmd_clearscreen(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_clearscreen) {
        backend->ops->cmd_clearscreen(backend, ctx);
    }
}

static void ui_cmd_clearendofscreen(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_clearendofscreen) {
        backend->ops->cmd_clearendofscreen(backend, ctx);
    }
}

static void ui_cmd_clearstartofscreen(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_clearstartofscreen) {
        backend->ops->cmd_clearstartofscreen(backend, ctx);
    }
}

static void ui_cmd_alignmenttest(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_alignmenttest) {
        backend->ops->cmd_alignmenttest(backend, ctx);
    }
}

static void ui_cmd_reverseindex(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_reverseindex) {
        backend->ops->cmd_reverseindex(backend, ctx);
    }
}

static void ui_cmd_linefeed(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_linefeed) {
        backend->ops->cmd_linefeed(backend, ctx);
    }
}

static void ui_cmd_scrollup(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_scrollup) {
        backend->ops->cmd_scrollup(backend, ctx);
    }
}

static void ui_cmd_scrolldown(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_scrolldown) {
        backend->ops->cmd_scrolldown(backend, ctx);
    }
}

static void ui_cmd_setselection(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_setselection) {
        backend->ops->cmd_setselection(backend, ctx);
    }
}

static void ui_cmd_rawstring(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_rawstring) {
        backend->ops->cmd_rawstring(backend, ctx);
    }
}

static void ui_cmd_sixelimage(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_sixelimage) {
        backend->ops->cmd_sixelimage(backend, ctx);
    }
}

static void ui_cmd_syncstart(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_syncstart) {
        backend->ops->cmd_syncstart(backend, ctx);
    }
}

// ============================================================================
// Router Management Functions
// ============================================================================

backend_router_t* backend_router_create(backend_mode_t initial_mode) {
    backend_router_impl_t* impl = calloc(1, sizeof(backend_router_impl_t));
    if (!impl) {
        return NULL;
    }
    
    // Initialize base structure
    impl->base.size = sizeof(backend_router_t);
    impl->base.version = UI_BACKEND_ABI_VERSION;
    impl->base.mode = initial_mode;
    impl->base.enabled = false;
    atomic_store(&impl->enabled_atomic, false);
    
    // Initialize mutex
    pthread_mutex_init(&impl->lock, NULL);
    
    // Initialize command mapping table
    impl->cmd_table.capacity = MAX_CMD_MAPPINGS;
    impl->cmd_table.mappings = calloc(MAX_CMD_MAPPINGS, sizeof(backend_cmd_mapping_t));
    if (!impl->cmd_table.mappings) {
        free(impl);
        return NULL;
    }
    
    // Initialize statistics
    impl->base.stats.size = sizeof(backend_router_stats_t);
    impl->base.stats.min_routing_time_ns = UINT64_MAX;
    
    // Initialize default mappings
    backend_router_init_default_mappings(&impl->base);
    
    // Set start time
    impl->start_time_ns = get_time_ns();
    
    return &impl->base;
}

void backend_router_destroy(backend_router_t* router) {
    if (!router) return;
    
    backend_router_impl_t* impl = (backend_router_impl_t*)router;
    
    pthread_mutex_lock(&impl->lock);
    
    // Clean up recording if active
    if (impl->recording.commands) {
        free(impl->recording.commands);
    }
    
    // Clean up command mappings
    if (impl->cmd_table.mappings) {
        free(impl->cmd_table.mappings);
    }
    
    pthread_mutex_unlock(&impl->lock);
    pthread_mutex_destroy(&impl->lock);
    
    free(impl);
}

void backend_router_set_mode(backend_router_t* router, backend_mode_t mode) {
    if (!router) return;
    
    backend_router_impl_t* impl = (backend_router_impl_t*)router;
    
    pthread_mutex_lock(&impl->lock);
    router->mode = mode;
    pthread_mutex_unlock(&impl->lock);
}

int backend_router_register_ui(backend_router_t* router, struct ui_backend* backend) {
    if (!router) return ROUTER_ERR_INVALID_MODE;
    
    backend_router_impl_t* impl = (backend_router_impl_t*)router;
    
    pthread_mutex_lock(&impl->lock);
    
    if (router->ui_backend) {
        impl->last_error = ROUTER_ERR_ALREADY_REGISTERED;
        pthread_mutex_unlock(&impl->lock);
        return ROUTER_ERR_ALREADY_REGISTERED;
    }
    
    router->ui_backend = backend;
    router->enabled = true;
    atomic_store(&impl->enabled_atomic, true);
    
    pthread_mutex_unlock(&impl->lock);
    
    return ROUTER_OK;
}

void backend_router_unregister_ui(backend_router_t* router) {
    if (!router) return;
    
    backend_router_impl_t* impl = (backend_router_impl_t*)router;
    
    pthread_mutex_lock(&impl->lock);
    router->ui_backend = NULL;
    router->enabled = false;
    atomic_store(&impl->enabled_atomic, false);
    pthread_mutex_unlock(&impl->lock);
}

// ============================================================================
// Command Routing Functions
// ============================================================================

void backend_route_command(backend_router_t* router,
                          struct tty* tty,
                          tty_cmd_fn cmd_fn,
                          const struct tty_ctx* ctx) {
    if (!router || !cmd_fn) return;
    
    backend_router_impl_t* impl = (backend_router_impl_t*)router;
    
    // Fast path: check if enabled using atomic
    if (!atomic_load(&impl->enabled_atomic)) {
        // Fallback to traditional TTY
        if (cmd_fn) {
            cmd_fn(tty, ctx);
        }
        ATOMIC_INC(router->stats.commands_to_tty);
        return;
    }
    
    uint64_t start_time = 0;
    if (router->collect_metrics) {
        start_time = get_time_ns();
    }
    
    // Find command mapping (using read lock)
    pthread_rwlock_rdlock(&mapping_rwlock);
    const backend_cmd_mapping_t* mapping = find_mapping(impl, cmd_fn);
    pthread_rwlock_unlock(&mapping_rwlock);
    
    // Record command if recording is active
    if (impl->recording.recording) {
        pthread_mutex_lock(&impl->lock);
        if (impl->recording.count < impl->recording.capacity) {
            recorded_command_t* rec = &impl->recording.commands[impl->recording.count++];
            rec->cmd_fn = cmd_fn;
            copy_tty_ctx(&rec->ctx_copy, ctx);
            rec->timestamp_ns = get_time_ns();
        }
        pthread_mutex_unlock(&impl->lock);
    }
    
    // Route based on mode
    bool routed_to_ui = false;
    bool routed_to_tty = false;
    
    switch (router->mode) {
        case BACKEND_MODE_TTY:
            // Traditional TTY output
            if (cmd_fn) {
                cmd_fn(tty, ctx);
                routed_to_tty = true;
            }
            break;
            
        case BACKEND_MODE_UI:
            // UI backend only
            if (router->ui_backend && mapping && mapping->ui_fn) {
                mapping->ui_fn(router->ui_backend, ctx);
                routed_to_ui = true;
            } else {
                // Fallback to TTY if UI function not available
                if (cmd_fn) {
                    cmd_fn(tty, ctx);
                    routed_to_tty = true;
                }
            }
            break;
            
        case BACKEND_MODE_HYBRID:
            // Both backends (sequential, not parallel for thread safety)
            if (impl->hybrid_config.sync_output) {
                // Synchronous execution
                if (cmd_fn) {
                    cmd_fn(tty, ctx);
                    routed_to_tty = true;
                }
                if (router->ui_backend && mapping && mapping->ui_fn) {
                    mapping->ui_fn(router->ui_backend, ctx);
                    routed_to_ui = true;
                }
            } else {
                // Prefer one backend based on configuration
                if (impl->hybrid_config.prefer_ui) {
                    if (router->ui_backend && mapping && mapping->ui_fn) {
                        mapping->ui_fn(router->ui_backend, ctx);
                        routed_to_ui = true;
                    }
                    if (!routed_to_ui && cmd_fn) {
                        cmd_fn(tty, ctx);
                        routed_to_tty = true;
                    }
                } else {
                    if (cmd_fn) {
                        cmd_fn(tty, ctx);
                        routed_to_tty = true;
                    }
                    if (router->ui_backend && mapping && mapping->ui_fn) {
                        mapping->ui_fn(router->ui_backend, ctx);
                        routed_to_ui = true;
                    }
                }
            }
            break;
    }
    
    // Update statistics (atomic operations)
    ATOMIC_INC(router->stats.commands_routed);
    if (routed_to_ui) {
        ATOMIC_INC(router->stats.commands_to_ui);
    }
    if (routed_to_tty) {
        ATOMIC_INC(router->stats.commands_to_tty);
    }
    if (!routed_to_ui && !routed_to_tty) {
        ATOMIC_INC(router->stats.commands_dropped);
    }
    
    // Update performance metrics
    if (router->collect_metrics && start_time > 0) {
        uint64_t elapsed = get_time_ns() - start_time;
        ATOMIC_ADD(router->stats.total_routing_time_ns, elapsed);
        ATOMIC_MIN(router->stats.min_routing_time_ns, elapsed);
        ATOMIC_MAX(router->stats.max_routing_time_ns, elapsed);
        
        // Update average (not atomic, acceptable for statistics)
        if (router->stats.commands_routed > 0) {
            router->stats.avg_routing_time_ns = 
                router->stats.total_routing_time_ns / router->stats.commands_routed;
        }
        
        // Invoke metric callback if set
        if (router->on_metric && mapping) {
            router->on_metric(mapping->name, elapsed, router->metric_user_data);
        }
    }
}

bool backend_should_route_to_ui(const backend_router_t* router, tty_cmd_fn cmd_fn) {
    if (!router || !cmd_fn) return false;
    
    backend_router_impl_t* impl = (backend_router_impl_t*)router;
    
    if (!atomic_load(&impl->enabled_atomic)) {
        return false;
    }
    
    if (router->mode == BACKEND_MODE_TTY) {
        return false;
    }
    
    pthread_rwlock_rdlock(&mapping_rwlock);
    const backend_cmd_mapping_t* mapping = find_mapping(impl, cmd_fn);
    pthread_rwlock_unlock(&mapping_rwlock);
    
    return (mapping && mapping->ui_fn && router->ui_backend);
}

const char* backend_get_command_name(const backend_router_t* router, tty_cmd_fn cmd_fn) {
    if (!router || !cmd_fn) return "unknown";
    
    backend_router_impl_t* impl = (backend_router_impl_t*)router;
    
    pthread_rwlock_rdlock(&mapping_rwlock);
    const backend_cmd_mapping_t* mapping = find_mapping(impl, cmd_fn);
    pthread_rwlock_unlock(&mapping_rwlock);
    
    return mapping ? mapping->name : "unmapped";
}

// ============================================================================
// Performance Monitoring
// ============================================================================

void backend_router_set_metrics(backend_router_t* router, bool enable) {
    if (!router) return;
    router->collect_metrics = enable;
}

const backend_router_stats_t* backend_router_get_stats(const backend_router_t* router) {
    if (!router) return NULL;
    return &router->stats;
}

void backend_router_reset_stats(backend_router_t* router) {
    if (!router) return;
    
    backend_router_impl_t* impl = (backend_router_impl_t*)router;
    
    pthread_mutex_lock(&impl->lock);
    memset(&router->stats, 0, sizeof(backend_router_stats_t));
    router->stats.size = sizeof(backend_router_stats_t);
    router->stats.min_routing_time_ns = UINT64_MAX;
    pthread_mutex_unlock(&impl->lock);
}

// ============================================================================
// Command Mapping Initialization
// ============================================================================

void backend_router_init_default_mappings(backend_router_t* router) {
    if (!router) return;
    
    // Add all 22 command mappings
    backend_router_add_mapping(router, "cell", 
        tty_cmd_cell, ui_cmd_cell, CMD_FLAG_VISUAL | CMD_FLAG_BATCHABLE);
    backend_router_add_mapping(router, "cells",
        tty_cmd_cells, ui_cmd_cells, CMD_FLAG_VISUAL | CMD_FLAG_BATCHABLE);
    backend_router_add_mapping(router, "insertcharacter",
        tty_cmd_insertcharacter, ui_cmd_insertcharacter, CMD_FLAG_VISUAL | CMD_FLAG_STATEFUL);
    backend_router_add_mapping(router, "deletecharacter",
        tty_cmd_deletecharacter, ui_cmd_deletecharacter, CMD_FLAG_VISUAL | CMD_FLAG_STATEFUL);
    backend_router_add_mapping(router, "clearcharacter",
        tty_cmd_clearcharacter, ui_cmd_clearcharacter, CMD_FLAG_VISUAL);
    
    backend_router_add_mapping(router, "insertline",
        tty_cmd_insertline, ui_cmd_insertline, CMD_FLAG_VISUAL | CMD_FLAG_STATEFUL);
    backend_router_add_mapping(router, "deleteline",
        tty_cmd_deleteline, ui_cmd_deleteline, CMD_FLAG_VISUAL | CMD_FLAG_STATEFUL);
    backend_router_add_mapping(router, "clearline",
        tty_cmd_clearline, ui_cmd_clearline, CMD_FLAG_VISUAL);
    backend_router_add_mapping(router, "clearendofline",
        tty_cmd_clearendofline, ui_cmd_clearendofline, CMD_FLAG_VISUAL);
    backend_router_add_mapping(router, "clearstartofline",
        tty_cmd_clearstartofline, ui_cmd_clearstartofline, CMD_FLAG_VISUAL);
    
    backend_router_add_mapping(router, "clearscreen",
        tty_cmd_clearscreen, ui_cmd_clearscreen, CMD_FLAG_VISUAL | CMD_FLAG_URGENT);
    backend_router_add_mapping(router, "clearendofscreen",
        tty_cmd_clearendofscreen, ui_cmd_clearendofscreen, CMD_FLAG_VISUAL);
    backend_router_add_mapping(router, "clearstartofscreen",
        tty_cmd_clearstartofscreen, ui_cmd_clearstartofscreen, CMD_FLAG_VISUAL);
    backend_router_add_mapping(router, "alignmenttest",
        tty_cmd_alignmenttest, ui_cmd_alignmenttest, CMD_FLAG_VISUAL);
    
    backend_router_add_mapping(router, "reverseindex",
        tty_cmd_reverseindex, ui_cmd_reverseindex, CMD_FLAG_VISUAL | CMD_FLAG_STATEFUL);
    backend_router_add_mapping(router, "linefeed",
        tty_cmd_linefeed, ui_cmd_linefeed, CMD_FLAG_VISUAL | CMD_FLAG_STATEFUL);
    backend_router_add_mapping(router, "scrollup",
        tty_cmd_scrollup, ui_cmd_scrollup, CMD_FLAG_VISUAL | CMD_FLAG_STATEFUL);
    backend_router_add_mapping(router, "scrolldown",
        tty_cmd_scrolldown, ui_cmd_scrolldown, CMD_FLAG_VISUAL | CMD_FLAG_STATEFUL);
    
    backend_router_add_mapping(router, "setselection",
        tty_cmd_setselection, ui_cmd_setselection, CMD_FLAG_CONTROL);
    backend_router_add_mapping(router, "rawstring",
        tty_cmd_rawstring, ui_cmd_rawstring, CMD_FLAG_URGENT);
    backend_router_add_mapping(router, "sixelimage",
        tty_cmd_sixelimage, ui_cmd_sixelimage, CMD_FLAG_VISUAL);
    backend_router_add_mapping(router, "syncstart",
        tty_cmd_syncstart, ui_cmd_syncstart, CMD_FLAG_CONTROL);
}

void backend_router_add_mapping(backend_router_t* router,
                                const char* name,
                                tty_cmd_fn tty_fn,
                                ui_cmd_fn ui_fn,
                                uint32_t flags) {
    if (!router || !name || !tty_fn) return;
    
    backend_router_impl_t* impl = (backend_router_impl_t*)router;
    
    pthread_rwlock_wrlock(&mapping_rwlock);
    
    if (impl->cmd_table.count < impl->cmd_table.capacity) {
        backend_cmd_mapping_t* mapping = &impl->cmd_table.mappings[impl->cmd_table.count++];
        mapping->name = name;
        mapping->tty_fn = tty_fn;
        mapping->ui_fn = ui_fn;
        mapping->flags = flags;
    }
    
    pthread_rwlock_unlock(&mapping_rwlock);
}

void backend_router_remove_mapping(backend_router_t* router, tty_cmd_fn tty_fn) {
    if (!router || !tty_fn) return;
    
    backend_router_impl_t* impl = (backend_router_impl_t*)router;
    
    pthread_rwlock_wrlock(&mapping_rwlock);
    
    for (uint32_t i = 0; i < impl->cmd_table.count; i++) {
        if (impl->cmd_table.mappings[i].tty_fn == tty_fn) {
            // Move remaining mappings down
            memmove(&impl->cmd_table.mappings[i],
                   &impl->cmd_table.mappings[i + 1],
                   (impl->cmd_table.count - i - 1) * sizeof(backend_cmd_mapping_t));
            impl->cmd_table.count--;
            break;
        }
    }
    
    pthread_rwlock_unlock(&mapping_rwlock);
}

// ============================================================================
// Global Router Management
// ============================================================================

int backend_router_init_global(backend_mode_t mode) {
    pthread_mutex_lock(&router_mutex);
    
    if (global_backend_router) {
        pthread_mutex_unlock(&router_mutex);
        return ROUTER_ERR_ALREADY_REGISTERED;
    }
    
    global_backend_router = backend_router_create(mode);
    if (!global_backend_router) {
        pthread_mutex_unlock(&router_mutex);
        return ROUTER_ERR_INVALID_MODE;
    }
    
    pthread_mutex_unlock(&router_mutex);
    return ROUTER_OK;
}

void backend_router_cleanup_global(void) {
    pthread_mutex_lock(&router_mutex);
    
    if (global_backend_router) {
        backend_router_destroy(global_backend_router);
        global_backend_router = NULL;
    }
    
    pthread_mutex_unlock(&router_mutex);
}

// ============================================================================
// Hybrid Mode Support
// ============================================================================

void backend_router_configure_hybrid(backend_router_t* router,
                                     const hybrid_mode_config_t* config) {
    if (!router || !config) return;
    
    backend_router_impl_t* impl = (backend_router_impl_t*)router;
    
    pthread_mutex_lock(&impl->lock);
    memcpy(&impl->hybrid_config, config, sizeof(hybrid_mode_config_t));
    pthread_mutex_unlock(&impl->lock);
}

// ============================================================================
// Recording and Replay
// ============================================================================

void backend_router_start_recording(backend_router_t* router, uint32_t max_commands) {
    if (!router || max_commands == 0) return;
    
    backend_router_impl_t* impl = (backend_router_impl_t*)router;
    
    pthread_mutex_lock(&impl->lock);
    
    // Clean up existing recording
    if (impl->recording.commands) {
        free(impl->recording.commands);
    }
    
    impl->recording.commands = calloc(max_commands, sizeof(recorded_command_t));
    if (impl->recording.commands) {
        impl->recording.capacity = max_commands;
        impl->recording.count = 0;
        impl->recording.recording = true;
    }
    
    pthread_mutex_unlock(&impl->lock);
}

recorded_command_t* backend_router_stop_recording(backend_router_t* router, uint32_t* count) {
    if (!router || !count) return NULL;
    
    backend_router_impl_t* impl = (backend_router_impl_t*)router;
    
    pthread_mutex_lock(&impl->lock);
    
    impl->recording.recording = false;
    *count = impl->recording.count;
    
    recorded_command_t* commands = impl->recording.commands;
    impl->recording.commands = NULL;
    impl->recording.count = 0;
    impl->recording.capacity = 0;
    
    pthread_mutex_unlock(&impl->lock);
    
    return commands;
}

void backend_router_replay_commands(backend_router_t* router,
                                    const recorded_command_t* commands,
                                    uint32_t count) {
    if (!router || !commands || count == 0) return;
    
    for (uint32_t i = 0; i < count; i++) {
        const recorded_command_t* rec = &commands[i];
        backend_route_command(router, NULL, rec->cmd_fn, &rec->ctx_copy);
    }
}

// ============================================================================
// Error Handling
// ============================================================================

router_error_t backend_router_get_last_error(const backend_router_t* router) {
    if (!router) return ROUTER_ERR_INVALID_MODE;
    
    backend_router_impl_t* impl = (backend_router_impl_t*)router;
    return impl->last_error;
}

const char* backend_router_error_string(router_error_t err) {
    switch (err) {
        case ROUTER_OK:
            return "Success";
        case ROUTER_ERR_NO_BACKEND:
            return "No backend registered";
        case ROUTER_ERR_INVALID_MODE:
            return "Invalid routing mode";
        case ROUTER_ERR_ALREADY_REGISTERED:
            return "Backend already registered";
        case ROUTER_ERR_COMMAND_NOT_FOUND:
            return "Command not found in mapping";
        case ROUTER_ERR_BACKEND_FAILED:
            return "Backend operation failed";
        default:
            return "Unknown error";
    }
}