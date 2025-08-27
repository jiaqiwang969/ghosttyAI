// layout_callbacks.h - Layout Management vtable Interface
// Purpose: Abstract tmux layout operations with high-performance callbacks
// Author: CORE-001 (c-tmux-specialist) 
// Date: 2025-08-26
// Task: T-203 - Layout Management Callbacks
// Version: 1.0.0
// Performance Targets: <50ms layout change, <10ms pane split, <5ms resize

#ifndef LAYOUT_CALLBACKS_H
#define LAYOUT_CALLBACKS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../src/event_loop_backend.h"
#include "../../CORE-002/include/grid_callbacks.h"

// Forward declarations
struct layout_backend;
struct layout_cell;
struct window_pane;
struct layout_router;
typedef struct layout_backend layout_backend_t;
typedef struct layout_router layout_router_t;

// Layout types (from tmux)
typedef enum {
    LAYOUT_TOPBOTTOM = 0,    // Vertical split
    LAYOUT_LEFTRIGHT = 1,    // Horizontal split
    LAYOUT_WINDOWPANE = 2    // Terminal pane
} layout_type_t;

// Layout presets
typedef enum {
    LAYOUT_PRESET_EVEN_HORIZONTAL,
    LAYOUT_PRESET_EVEN_VERTICAL,
    LAYOUT_PRESET_MAIN_HORIZONTAL,
    LAYOUT_PRESET_MAIN_HORIZONTAL_MIRRORED,
    LAYOUT_PRESET_MAIN_VERTICAL,
    LAYOUT_PRESET_MAIN_VERTICAL_MIRRORED,
    LAYOUT_PRESET_TILED,
    LAYOUT_PRESET_CUSTOM
} layout_preset_t;

// Split direction
typedef enum {
    SPLIT_HORIZONTAL,  // Split left/right
    SPLIT_VERTICAL     // Split top/bottom
} split_direction_t;

// Layout cell structure (tree node)
typedef struct layout_cell {
    layout_type_t type;              // Cell type
    struct layout_cell* parent;      // Parent cell
    
    // Position and size
    uint32_t xoff, yoff;             // Offset from parent
    uint32_t sx, sy;                 // Size
    
    // For container cells
    struct {
        struct layout_cell* first;   // First child
        struct layout_cell* last;    // Last child
        uint32_t count;              // Number of children
    } children;
    
    // For pane cells
    struct {
        uint32_t id;                 // Pane ID
        struct window_pane* wp;      // Window pane pointer
        grid_backend_t* grid;        // Grid backend from T-202
        bool zoomed;                 // Is pane zoomed?
        bool synchronized;           // Is pane synchronized?
    } pane;
    
    // Linked list
    struct layout_cell* next;
    struct layout_cell* prev;
    
    // Dirty tracking
    bool needs_resize;
    bool needs_redraw;
    uint64_t generation;             // Layout generation number
} layout_cell_t;

// Layout change event
typedef struct {
    layout_cell_t* cell;             // Affected cell
    enum {
        LAYOUT_EVENT_SPLIT,
        LAYOUT_EVENT_CLOSE,
        LAYOUT_EVENT_RESIZE,
        LAYOUT_EVENT_ZOOM,
        LAYOUT_EVENT_SWAP,
        LAYOUT_EVENT_ROTATE,
        LAYOUT_EVENT_PRESET_CHANGE
    } type;
    union {
        struct {
            split_direction_t direction;
            uint32_t position;       // Split position
        } split;
        struct {
            int32_t dx, dy;          // Resize delta
        } resize;
        struct {
            layout_cell_t* target;   // Swap target
        } swap;
        struct {
            layout_preset_t preset;  // New preset
        } preset;
    } data;
} layout_event_t;

// Layout operation callbacks
typedef void (*layout_split_callback)(layout_cell_t* cell, split_direction_t dir, 
                                      uint32_t position, void* user_data);
typedef void (*layout_close_callback)(layout_cell_t* cell, void* user_data);
typedef void (*layout_resize_callback)(layout_cell_t* cell, int32_t dx, int32_t dy,
                                       void* user_data);
typedef void (*layout_zoom_callback)(layout_cell_t* cell, bool zoom, void* user_data);
typedef void (*layout_redraw_callback)(layout_cell_t* cell, dirty_region_t* region,
                                       void* user_data);

// Layout backend vtable
typedef struct layout_vtable {
    const char* name;
    
    // Cell management
    layout_cell_t* (*create_cell)(layout_type_t type, layout_cell_t* parent);
    void (*free_cell)(layout_cell_t* cell);
    void (*attach_pane)(layout_cell_t* cell, struct window_pane* wp);
    void (*detach_pane)(layout_cell_t* cell);
    
    // Layout operations
    int (*split_pane)(layout_cell_t* cell, split_direction_t dir, uint32_t size,
                     layout_cell_t** new_cell);
    int (*close_pane)(layout_cell_t* cell);
    int (*resize_pane)(layout_cell_t* cell, int32_t dx, int32_t dy);
    int (*swap_panes)(layout_cell_t* cell1, layout_cell_t* cell2);
    int (*rotate_panes)(layout_cell_t* parent, int direction);
    
    // Layout presets
    int (*apply_preset)(layout_cell_t* root, layout_preset_t preset);
    int (*parse_custom)(layout_cell_t* root, const char* layout_string);
    char* (*dump_layout)(layout_cell_t* root);
    
    // Zoom management
    int (*zoom_pane)(layout_cell_t* cell);
    int (*unzoom_pane)(layout_cell_t* cell);
    bool (*is_zoomed)(layout_cell_t* cell);
    
    // Synchronization
    int (*sync_panes)(layout_cell_t* parent, bool enable);
    bool (*are_synchronized)(layout_cell_t* cell);
    
    // Resize algorithms
    void (*resize_layout)(layout_cell_t* root, uint32_t sx, uint32_t sy);
    void (*balance_layout)(layout_cell_t* root);
    void (*maximize_pane)(layout_cell_t* cell);
    
    // Navigation
    layout_cell_t* (*find_pane)(layout_cell_t* root, uint32_t id);
    layout_cell_t* (*find_adjacent)(layout_cell_t* cell, int direction);
    layout_cell_t* (*find_by_position)(layout_cell_t* root, uint32_t x, uint32_t y);
    
    // Callbacks
    void (*set_split_callback)(layout_split_callback cb, void* user_data);
    void (*set_close_callback)(layout_close_callback cb, void* user_data);
    void (*set_resize_callback)(layout_resize_callback cb, void* user_data);
    void (*set_zoom_callback)(layout_zoom_callback cb, void* user_data);
    void (*set_redraw_callback)(layout_redraw_callback cb, void* user_data);
    
    // Performance monitoring
    void (*get_stats)(struct {
        uint64_t total_splits;
        uint64_t total_closes;
        uint64_t total_resizes;
        uint64_t avg_split_time_us;
        uint64_t avg_resize_time_us;
        uint64_t avg_layout_change_time_us;
    }* stats);
} layout_vtable_t;

// Layout router modes
typedef enum {
    LAYOUT_MODE_TMUX,      // Original tmux layout
    LAYOUT_MODE_GHOSTTY,   // Ghostty optimized layout
    LAYOUT_MODE_HYBRID     // Mixed mode
} layout_mode_t;

// Layout router structure
struct layout_router {
    layout_vtable_t* vtable;        // Current backend vtable
    layout_mode_t mode;              // Current mode
    layout_cell_t* root;             // Root layout cell
    
    // Window information
    uint32_t sx, sy;                 // Window size
    uint32_t pane_count;             // Number of panes
    
    // Zoom state
    layout_cell_t* zoomed_pane;      // Currently zoomed pane
    layout_cell_t* saved_layout;     // Saved layout before zoom
    
    // Synchronization
    bool sync_enabled;               // Global sync flag
    uint32_t sync_group_count;       // Number of sync groups
    
    // Performance tracking
    uint64_t generation;             // Layout generation number
    uint64_t last_change_time_us;   // Last change timestamp
    
    // Event integration
    event_loop_router_t* event_router;  // From T-201
    grid_router_t* grid_router;         // From T-202
    
    // Thread safety
    void* mutex;                     // Platform-specific mutex
};

// ============================================================================
// Public API
// ============================================================================

// Initialize layout router
layout_router_t* layout_router_init(layout_mode_t mode,
                                    event_loop_router_t* event_router,
                                    grid_router_t* grid_router);

// Cleanup layout router
void layout_router_cleanup(layout_router_t* router);

// Switch backend mode
int layout_router_switch_mode(layout_router_t* router, layout_mode_t mode);

// Create root layout
layout_cell_t* layout_create_root(layout_router_t* router, uint32_t sx, uint32_t sy);

// Pane operations
layout_cell_t* layout_split_pane(layout_router_t* router, layout_cell_t* cell,
                                 split_direction_t dir, uint32_t size);
int layout_close_pane(layout_router_t* router, layout_cell_t* cell);
int layout_resize_pane(layout_router_t* router, layout_cell_t* cell,
                      int32_t dx, int32_t dy);
int layout_swap_panes(layout_router_t* router, layout_cell_t* cell1,
                     layout_cell_t* cell2);
int layout_rotate_panes(layout_router_t* router, layout_cell_t* parent,
                       int direction);

// Layout presets
int layout_apply_preset(layout_router_t* router, layout_preset_t preset);
int layout_parse_custom(layout_router_t* router, const char* layout_string);
char* layout_dump(layout_router_t* router);

// Zoom operations
int layout_zoom_pane(layout_router_t* router, layout_cell_t* cell);
int layout_unzoom_pane(layout_router_t* router);
bool layout_is_zoomed(layout_router_t* router);

// Synchronization
int layout_sync_panes(layout_router_t* router, bool enable);
bool layout_are_synchronized(layout_router_t* router);

// Window resize
void layout_resize(layout_router_t* router, uint32_t sx, uint32_t sy);
void layout_balance(layout_router_t* router);

// Navigation
layout_cell_t* layout_find_pane(layout_router_t* router, uint32_t id);
layout_cell_t* layout_find_adjacent(layout_router_t* router, layout_cell_t* cell,
                                    int direction);
layout_cell_t* layout_find_by_position(layout_router_t* router, uint32_t x, uint32_t y);

// Focus management
int layout_set_focus(layout_router_t* router, layout_cell_t* cell);
layout_cell_t* layout_get_focus(layout_router_t* router);

// Performance
double layout_get_overhead_percent(layout_router_t* router);
void layout_get_stats(layout_router_t* router, void* stats);

// ============================================================================
// Compatibility macros for tmux integration
// ============================================================================

#ifdef TMUX_LAYOUT_COMPAT
#define layout_create_cell(parent)        layout_router_create_cell(global_layout_router, LAYOUT_WINDOWPANE, parent)
#define layout_free_cell(cell)            layout_router_free_cell(global_layout_router, cell)
#define layout_split_pane(cell, dir, sz)  layout_split_pane(global_layout_router, cell, dir, sz)
#define layout_close_pane(cell)           layout_close_pane(global_layout_router, cell)

extern layout_router_t* global_layout_router;
#endif

#endif // LAYOUT_CALLBACKS_H