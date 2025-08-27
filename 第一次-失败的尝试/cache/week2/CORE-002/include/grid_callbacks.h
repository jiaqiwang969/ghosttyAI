// grid_callbacks.h - Grid Operations vtable Interface
// Purpose: Abstract tmux grid operations with high-performance batch callbacks
// Author: CORE-002 (libtmux-core-developer)
// Date: 2025-08-26
// Task: T-202 - Grid Operations Batch Optimization
// Version: 1.0.0
// Performance Targets: 10x batch improvement, P99 <0.3ms, memory ≤110%

#ifndef GRID_CALLBACKS_H
#define GRID_CALLBACKS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>

// Forward declarations
struct grid_backend;
struct grid_router;
struct grid_cell;
struct grid_line;
struct dirty_region;
typedef struct grid_backend grid_backend_t;
typedef struct grid_router grid_router_t;

// Grid cell attributes (from tmux)
#define GRID_ATTR_BRIGHT        0x01
#define GRID_ATTR_DIM           0x02
#define GRID_ATTR_UNDERSCORE    0x04
#define GRID_ATTR_BLINK         0x08
#define GRID_ATTR_REVERSE       0x10
#define GRID_ATTR_HIDDEN        0x20
#define GRID_ATTR_ITALICS       0x40
#define GRID_ATTR_STRIKETHROUGH 0x80
#define GRID_ATTR_UNDERSCORE_2  0x100
#define GRID_ATTR_UNDERSCORE_3  0x200
#define GRID_ATTR_UNDERSCORE_4  0x400
#define GRID_ATTR_UNDERSCORE_5  0x800
#define GRID_ATTR_OVERLINE      0x1000

// Grid flags
#define GRID_FLAG_EXTENDED      0x01
#define GRID_FLAG_FG256         0x02
#define GRID_FLAG_BG256         0x04
#define GRID_FLAG_PADDING       0x08
#define GRID_FLAG_CLEARED       0x10
#define GRID_FLAG_TAB           0x20
#define GRID_FLAG_NOPALETTE     0x40

// Batch operation types for optimization
typedef enum {
    BATCH_OP_SET_CELL,      // Set multiple cells
    BATCH_OP_CLEAR,         // Clear region
    BATCH_OP_SCROLL,        // Scroll region
    BATCH_OP_INSERT_LINES,  // Insert lines
    BATCH_OP_DELETE_LINES,  // Delete lines
    BATCH_OP_INSERT_CELLS,  // Insert cells
    BATCH_OP_DELETE_CELLS,  // Delete cells
    BATCH_OP_COPY_REGION,   // Copy region
    BATCH_OP_FILL_REGION,   // Fill with character
} batch_op_type_t;

// Batch operation descriptor
typedef struct batch_operation {
    batch_op_type_t type;
    uint32_t x, y;           // Starting position
    uint32_t width, height;  // Dimensions
    union {
        struct {
            struct grid_cell* cells;  // Cell data for SET_CELL
            size_t count;
        } set;
        struct {
            uint32_t bg;              // Background for CLEAR
        } clear;
        struct {
            int32_t lines;            // Lines to scroll
            uint32_t rupper, rlower;  // Region bounds
        } scroll;
        struct {
            uint32_t count;           // Lines/cells to insert/delete
            uint32_t bg;
        } modify;
        struct {
            uint32_t src_x, src_y;   // Source for copy
        } copy;
        struct {
            struct grid_cell* cell;  // Cell to fill with
        } fill;
    } data;
} batch_operation_t;

// Dirty region tracking
typedef struct dirty_region {
    uint32_t x_min, y_min;
    uint32_t x_max, y_max;
    bool full_redraw;
    uint64_t generation;     // For versioning
} dirty_region_t;

// Grid statistics for performance monitoring
typedef struct grid_stats {
    uint64_t cells_written;
    uint64_t cells_cleared;
    uint64_t batch_operations;
    uint64_t single_operations;
    uint64_t dirty_flushes;
    uint64_t total_latency_ns;
    uint64_t min_latency_ns;
    uint64_t max_latency_ns;
    uint64_t memory_bytes;
    double batch_speedup;    // Actual speedup achieved
} grid_stats_t;

// Grid backend vtable
typedef struct grid_vtable {
    // Backend identifier
    const char* name;
    
    // Initialization and cleanup
    void* (*init)(uint32_t width, uint32_t height, uint32_t history_limit);
    void (*destroy)(void* backend);
    
    // Single cell operations (for compatibility)
    int (*get_cell)(void* backend, uint32_t x, uint32_t y, struct grid_cell* cell);
    int (*set_cell)(void* backend, uint32_t x, uint32_t y, const struct grid_cell* cell);
    int (*clear_cell)(void* backend, uint32_t x, uint32_t y, uint32_t bg);
    
    // Batch operations (10x performance target)
    int (*batch_execute)(void* backend, batch_operation_t* ops, size_t count);
    int (*batch_set_cells)(void* backend, uint32_t x, uint32_t y, 
                          const struct grid_cell* cells, size_t count);
    int (*batch_clear_region)(void* backend, uint32_t x, uint32_t y,
                             uint32_t width, uint32_t height, uint32_t bg);
    int (*batch_copy_region)(void* backend, uint32_t src_x, uint32_t src_y,
                            uint32_t dst_x, uint32_t dst_y,
                            uint32_t width, uint32_t height);
    int (*batch_fill_region)(void* backend, uint32_t x, uint32_t y,
                            uint32_t width, uint32_t height,
                            const struct grid_cell* cell);
    
    // Line operations
    int (*insert_lines)(void* backend, uint32_t y, uint32_t count, uint32_t bg);
    int (*delete_lines)(void* backend, uint32_t y, uint32_t count, uint32_t bg);
    int (*scroll_region)(void* backend, uint32_t upper, uint32_t lower,
                        int32_t lines, uint32_t bg);
    
    // Grid view operations
    int (*get_view)(void* backend, uint32_t x, uint32_t y,
                   uint32_t width, uint32_t height,
                   struct grid_cell** cells);
    int (*set_view_offset)(void* backend, uint32_t offset);
    
    // History management
    int (*scroll_history)(void* backend, uint32_t bg);
    int (*clear_history)(void* backend, uint32_t bg);
    int (*trim_history)(void* backend, uint32_t lines);
    
    // Dirty region tracking
    int (*mark_dirty)(void* backend, uint32_t x, uint32_t y,
                     uint32_t width, uint32_t height);
    int (*get_dirty_region)(void* backend, dirty_region_t* region);
    int (*clear_dirty)(void* backend);
    int (*enable_dirty_tracking)(void* backend, bool enable);
    
    // Optimization hints
    void (*begin_batch)(void* backend);     // Start batch mode
    void (*end_batch)(void* backend);       // End batch mode & flush
    void (*set_defer_mode)(void* backend, bool defer);  // Defer updates
    
    // Thread safety
    void (*lock)(void* backend);
    void (*unlock)(void* backend);
    bool (*try_lock)(void* backend);
    
    // Performance monitoring
    void (*get_stats)(void* backend, grid_stats_t* stats);
    void (*reset_stats)(void* backend);
    
    // Memory management
    size_t (*get_memory_usage)(void* backend);
    int (*compact)(void* backend);  // Compact memory usage
    
    // Resize operations
    int (*resize)(void* backend, uint32_t new_width, uint32_t new_height);
    
    // Snapshot/restore for testing
    void* (*snapshot)(void* backend);
    int (*restore)(void* backend, void* snapshot);
} grid_vtable_t;

// Router modes
typedef enum {
    GRID_MODE_TMUX,      // Use original tmux grid
    GRID_MODE_GHOSTTY,   // Use Ghostty callbacks
    GRID_MODE_HYBRID,    // Use both for transition
    GRID_MODE_BATCH      // Optimized batch mode
} grid_router_mode_t;

// Grid router structure
struct grid_router {
    grid_vtable_t* vtable;          // Current backend vtable
    void* backend;                   // Backend instance
    grid_router_mode_t mode;        // Current mode
    
    // Batch optimization state
    batch_operation_t* batch_buffer;
    size_t batch_capacity;
    size_t batch_count;
    bool batch_mode;
    
    // Dirty tracking
    dirty_region_t dirty;
    bool dirty_tracking_enabled;
    uint64_t dirty_generation;
    
    // Performance metrics
    grid_stats_t stats;
    uint64_t last_op_time_ns;
    
    // Thread safety
    pthread_mutex_t mutex;
    pthread_rwlock_t rwlock;        // For read-heavy workloads
    bool use_rwlock;
    
    // Configuration
    struct {
        size_t batch_threshold;     // Min ops before batching
        size_t batch_max_size;       // Max batch buffer size
        uint64_t batch_timeout_ns;   // Max time before flush
        bool auto_batch;             // Auto-detect batch opportunities
        bool zero_copy;              // Enable zero-copy optimizations
    } config;
};

// ============================================================================
// Public API - Grid Router Interface
// ============================================================================

// Initialize router with specified mode
grid_router_t* grid_router_init(grid_router_mode_t mode, 
                                uint32_t width, uint32_t height,
                                uint32_t history_limit);

// Cleanup router
void grid_router_destroy(grid_router_t* router);

// Switch backend mode at runtime
int grid_router_switch_mode(grid_router_t* router, grid_router_mode_t mode);

// Single cell operations
int grid_router_get_cell(grid_router_t* router, uint32_t x, uint32_t y,
                        struct grid_cell* cell);
int grid_router_set_cell(grid_router_t* router, uint32_t x, uint32_t y,
                        const struct grid_cell* cell);

// Batch operations (primary interface for performance)
int grid_router_batch_begin(grid_router_t* router);
int grid_router_batch_add(grid_router_t* router, batch_operation_t* op);
int grid_router_batch_execute(grid_router_t* router);
int grid_router_batch_end(grid_router_t* router);

// High-level batch operations
int grid_router_set_cells(grid_router_t* router, uint32_t x, uint32_t y,
                         const struct grid_cell* cells, size_t count);
int grid_router_clear_region(grid_router_t* router, uint32_t x, uint32_t y,
                            uint32_t width, uint32_t height, uint32_t bg);
int grid_router_scroll(grid_router_t* router, uint32_t upper, uint32_t lower,
                       int32_t lines, uint32_t bg);

// Dirty region tracking
int grid_router_get_dirty(grid_router_t* router, dirty_region_t* region);
int grid_router_clear_dirty(grid_router_t* router);
int grid_router_enable_dirty_tracking(grid_router_t* router, bool enable);

// Performance optimization
void grid_router_hint_batch_start(grid_router_t* router, size_t expected_ops);
void grid_router_hint_batch_end(grid_router_t* router);
void grid_router_set_auto_batch(grid_router_t* router, bool enable);

// Statistics and monitoring
void grid_router_get_stats(grid_router_t* router, grid_stats_t* stats);
void grid_router_reset_stats(grid_router_t* router);
double grid_router_get_batch_speedup(grid_router_t* router);
size_t grid_router_get_memory_usage(grid_router_t* router);

// Configuration
void grid_router_set_batch_threshold(grid_router_t* router, size_t threshold);
void grid_router_set_batch_timeout(grid_router_t* router, uint64_t timeout_ns);
void grid_router_enable_zero_copy(grid_router_t* router, bool enable);

// Thread safety helpers
void grid_router_lock(grid_router_t* router);
void grid_router_unlock(grid_router_t* router);
bool grid_router_try_lock(grid_router_t* router);

// Backend registration
int grid_register_backend(const char* name, grid_vtable_t* vtable);

// ============================================================================
// Compatibility Macros for tmux Integration
// ============================================================================

#ifdef TMUX_GRID_COMPAT
// Map tmux grid calls to router API
#define grid_create(sx, sy, hlimit)     grid_router_init(GRID_MODE_TMUX, sx, sy, hlimit)
#define grid_destroy(gd)                grid_router_destroy(gd)
#define grid_get_cell(gd, px, py, gc)   grid_router_get_cell(gd, px, py, gc)
#define grid_set_cell(gd, px, py, gc)   grid_router_set_cell(gd, px, py, gc)
#define grid_clear(gd, px, py, nx, ny, bg) \
    grid_router_clear_region(gd, px, py, nx, ny, bg)

// Global router for compatibility
extern grid_router_t* global_grid_router;
#endif

// ============================================================================
// Performance Macros and Inline Helpers
// ============================================================================

// Fast path for single cell when not in batch mode
static inline int grid_router_set_cell_fast(grid_router_t* router,
                                            uint32_t x, uint32_t y,
                                            const struct grid_cell* cell) {
    if (router->batch_mode && router->config.auto_batch) {
        batch_operation_t op = {
            .type = BATCH_OP_SET_CELL,
            .x = x, .y = y,
            .data.set = { .cells = (struct grid_cell*)cell, .count = 1 }
        };
        return grid_router_batch_add(router, &op);
    }
    return router->vtable->set_cell(router->backend, x, y, cell);
}

// Check if batching would be beneficial
static inline bool grid_router_should_batch(grid_router_t* router, size_t ops) {
    return ops >= router->config.batch_threshold ||
           (router->batch_mode && router->batch_count > 0);
}

#endif // GRID_CALLBACKS_H