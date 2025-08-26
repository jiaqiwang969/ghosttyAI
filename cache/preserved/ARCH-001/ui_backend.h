// ui_backend.h - UI Backend Abstraction Interface for libtmuxcore
// Purpose: Replace VT sequence generation with structured callbacks
// Author: ARCH-001 (System Architect)
// Date: 2025-08-25
// Version: 1.0.0

#ifndef UI_BACKEND_H
#define UI_BACKEND_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ============================================================================
// ABI Stability Macros
// ============================================================================

#define UI_BACKEND_ABI_VERSION_MAJOR 1
#define UI_BACKEND_ABI_VERSION_MINOR 0
#define UI_BACKEND_ABI_VERSION_PATCH 0
#define UI_BACKEND_ABI_VERSION ((UI_BACKEND_ABI_VERSION_MAJOR << 16) | \
                                (UI_BACKEND_ABI_VERSION_MINOR << 8) | \
                                UI_BACKEND_ABI_VERSION_PATCH)

// Forward declarations
struct tty;
struct tty_ctx;
struct grid_cell;
struct ui_backend;

// ============================================================================
// Core Data Structures
// ============================================================================

// Color representation (RGB + special values)
#define UI_COLOR_DEFAULT    0xFFFFFFFE  // Terminal default color
#define UI_COLOR_INVALID    0xFFFFFFFF  // Invalid/unset color

// Cell attributes
typedef enum {
    UI_ATTR_BOLD       = 1 << 0,
    UI_ATTR_ITALIC     = 1 << 1,
    UI_ATTR_UNDERLINE  = 1 << 2,
    UI_ATTR_DIM        = 1 << 3,
    UI_ATTR_REVERSE    = 1 << 4,
    UI_ATTR_BLINK      = 1 << 5,
    UI_ATTR_STRIKE     = 1 << 6,
    UI_ATTR_DOUBLE_UL  = 1 << 7,
    UI_ATTR_CURLY_UL   = 1 << 8,
    UI_ATTR_DOTTED_UL  = 1 << 9,
    UI_ATTR_DASHED_UL  = 1 << 10,
} ui_attr_flags_t;

// Single cell representation
typedef struct ui_cell {
    uint32_t codepoint;         // Unicode codepoint
    uint32_t fg_rgb;            // Foreground color (RGB or UI_COLOR_*)
    uint32_t bg_rgb;            // Background color
    uint16_t attrs;             // Attribute flags
    uint8_t  width;             // Display width (0, 1, 2)
    uint8_t  cluster_cont;      // Grapheme cluster continuation
} ui_cell_t;

// Span of contiguous cells with same attributes
typedef struct ui_span {
    uint32_t row;               // Row number
    uint32_t col_start;         // Starting column
    uint32_t col_end;           // Ending column (exclusive)
    const ui_cell_t* cells;     // Array of cells
    uint32_t flags;             // Span-specific flags
} ui_span_t;

// Frame flags for batching
typedef enum {
    UI_FRAME_COMPLETE  = 1 << 0,  // Frame contains all changes
    UI_FRAME_PARTIAL   = 1 << 1,  // More updates coming
    UI_FRAME_URGENT    = 1 << 2,  // Bypass batching (cursor/bell)
    UI_FRAME_DROPPED   = 1 << 3,  // Previous frames were dropped
    UI_FRAME_SNAPSHOT  = 1 << 4,  // Full grid snapshot
    UI_FRAME_CURSOR    = 1 << 5,  // Contains cursor update
} ui_frame_flags_t;

// Frame structure for batched updates
typedef struct ui_frame {
    uint32_t size;              // MUST be first field (ABI stability)
    uint64_t frame_seq;         // Monotonic sequence number
    uint64_t timestamp_ns;      // Frame generation timestamp
    uint32_t pane_id;           // Target pane
    uint32_t span_count;        // Number of spans in this frame
    const ui_span_t* spans;     // Array of dirty spans
    ui_frame_flags_t flags;     // Frame metadata
    
    // Statistics
    uint32_t updates_batched;   // Updates merged into this frame
    uint32_t cells_modified;    // Total cells changed
    uint32_t frames_dropped;    // Frames dropped before this
} ui_frame_t;

// ============================================================================
// Backend Command Callbacks (22 tty_cmd_* functions)
// ============================================================================

typedef struct ui_backend_ops {
    uint32_t size;              // MUST be first field (ABI stability)
    uint32_t version;           // Interface version
    
    // Character/cell operations
    void (*cmd_cell)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_cells)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_insertcharacter)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_deletecharacter)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_clearcharacter)(struct ui_backend*, const struct tty_ctx*);
    
    // Line operations
    void (*cmd_insertline)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_deleteline)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_clearline)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_clearendofline)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_clearstartofline)(struct ui_backend*, const struct tty_ctx*);
    
    // Screen operations
    void (*cmd_clearscreen)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_clearendofscreen)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_clearstartofscreen)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_alignmenttest)(struct ui_backend*, const struct tty_ctx*);
    
    // Scrolling operations
    void (*cmd_reverseindex)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_linefeed)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_scrollup)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_scrolldown)(struct ui_backend*, const struct tty_ctx*);
    
    // Special operations
    void (*cmd_setselection)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_rawstring)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_sixelimage)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_syncstart)(struct ui_backend*, const struct tty_ctx*);
} ui_backend_ops_t;

// ============================================================================
// Frame Aggregation System
// ============================================================================

typedef struct frame_aggregator {
    uint32_t size;              // MUST be first field
    
    // Configuration
    uint64_t frame_interval_ns; // Target frame interval (16666667 for 60 FPS)
    uint64_t max_latency_ns;    // Maximum buffering time (8ms)
    uint32_t max_spans;         // Maximum spans per frame
    
    // State
    uint64_t last_frame_time_ns;
    uint64_t frame_seq_next;
    
    // Accumulation buffer
    ui_span_t* pending_spans;
    uint32_t pending_count;
    uint32_t pending_capacity;
    
    // Dirty region tracking
    uint32_t dirty_min_row, dirty_max_row;
    uint32_t dirty_min_col, dirty_max_col;
    bool full_refresh_needed;
    
    // Statistics
    uint64_t frames_emitted;
    uint64_t spans_merged;
    uint64_t cells_updated;
    uint64_t frames_dropped;
} frame_aggregator_t;

// ============================================================================
// Backend Capabilities Negotiation
// ============================================================================

typedef enum {
    UI_CAP_FRAME_BATCH    = 1 << 0,  // Supports frame batching
    UI_CAP_UTF8_LINES     = 1 << 1,  // Supports UTF-8 line mode
    UI_CAP_24BIT_COLOR    = 1 << 2,  // Supports true color
    UI_CAP_BORDERS_BY_UI  = 1 << 3,  // UI draws borders
    UI_CAP_CURSOR_SHAPES  = 1 << 4,  // Supports cursor shapes
    UI_CAP_UNDERLINE_STYLES = 1 << 5, // Supports underline styles
    UI_CAP_SIXEL         = 1 << 6,   // Supports sixel images
    UI_CAP_SYNCHRONIZED  = 1 << 7,   // Supports synchronized updates
} ui_cap_flags_t;

typedef struct ui_capabilities {
    uint32_t size;              // MUST be first field
    uint32_t version;
    ui_cap_flags_t supported;  // Bitmask of supported features
    
    // Performance hints
    uint32_t max_fps;           // Maximum frame rate
    uint32_t optimal_batch_size; // Optimal span count per frame
    uint32_t max_dirty_rects;   // Maximum dirty rectangles
} ui_capabilities_t;

// ============================================================================
// Backend Structure
// ============================================================================

typedef enum {
    UI_BACKEND_TTY,             // Traditional TTY output
    UI_BACKEND_GHOSTTY,         // Ghostty native backend
    UI_BACKEND_TEST,            // Test/mock backend
} ui_backend_type_t;

struct ui_backend {
    uint32_t size;              // MUST be first field
    uint32_t version;
    ui_backend_type_t type;
    
    // Operations table
    const ui_backend_ops_t* ops;
    
    // Frame aggregation
    frame_aggregator_t* aggregator;
    
    // Capabilities
    ui_capabilities_t capabilities;
    
    // Callbacks to host
    void (*on_frame)(const ui_frame_t* frame, void* user_data);
    void (*on_bell)(uint32_t pane_id, void* user_data);
    void (*on_title)(uint32_t pane_id, const char* title, void* user_data);
    void (*on_overflow)(uint32_t dropped_frames, void* user_data);
    
    // User data for callbacks
    void* user_data;
    
    // Private implementation data
    void* priv;
};

// ============================================================================
// Backend Management Functions
// ============================================================================

// Create a new backend instance
struct ui_backend* ui_backend_create(ui_backend_type_t type,
                                     const ui_capabilities_t* requested_caps);

// Destroy a backend instance
void ui_backend_destroy(struct ui_backend* backend);

// Register backend with tty
int ui_backend_register(struct tty* tty, struct ui_backend* backend);

// Unregister backend from tty
void ui_backend_unregister(struct tty* tty);

// Process a tty command through the backend
void ui_backend_process_command(struct ui_backend* backend,
                                void (*cmd_fn)(struct tty*, const struct tty_ctx*),
                                const struct tty_ctx* ctx);

// Force frame emission (for urgent updates)
void ui_backend_flush_frame(struct ui_backend* backend);

// Query backend capabilities
const ui_capabilities_t* ui_backend_get_capabilities(const struct ui_backend* backend);

// Update frame aggregation settings
void ui_backend_set_frame_rate(struct ui_backend* backend, uint32_t target_fps);

// ============================================================================
// Frame Aggregation Functions
// ============================================================================

// Create frame aggregator
frame_aggregator_t* frame_aggregator_create(uint32_t target_fps);

// Destroy frame aggregator
void frame_aggregator_destroy(frame_aggregator_t* agg);

// Add update to aggregator
void frame_aggregator_add_update(frame_aggregator_t* agg,
                                 const struct tty_ctx* ctx);

// Check if frame should be emitted
bool frame_aggregator_should_emit(const frame_aggregator_t* agg);

// Emit accumulated frame
ui_frame_t* frame_aggregator_emit(frame_aggregator_t* agg);

// Reset aggregator state
void frame_aggregator_reset(frame_aggregator_t* agg);

// ============================================================================
// Utility Functions
// ============================================================================

// Convert grid_cell to ui_cell
void ui_cell_from_grid(ui_cell_t* ui_cell, const struct grid_cell* grid_cell);

// Merge adjacent spans
uint32_t ui_merge_spans(ui_span_t* spans, uint32_t count);

// Calculate dirty rectangle
void ui_calculate_dirty_rect(const ui_span_t* spans, uint32_t count,
                             uint32_t* min_row, uint32_t* max_row,
                             uint32_t* min_col, uint32_t* max_col);

// ============================================================================
// Thread Safety Guarantees
// ============================================================================

/*
 * Thread Safety Model:
 * 
 * 1. Backend operations (cmd_*) are called from tmux main thread
 * 2. Callbacks (on_frame, etc.) are invoked in the same thread
 * 3. Frame aggregation is single-threaded (no locks needed)
 * 4. Backend destruction must happen on the same thread as creation
 * 
 * The backend implementation MUST NOT:
 * - Hold locks when invoking callbacks
 * - Create additional threads without documentation
 * - Access tmux internal structures directly
 * 
 * The backend implementation MUST:
 * - Copy data before invoking callbacks
 * - Handle reentrancy if callbacks might trigger new commands
 * - Validate all pointers before use
 */

// ============================================================================
// Memory Management
// ============================================================================

/*
 * Memory Ownership Rules:
 * 
 * 1. ui_backend owns its internal structures
 * 2. Frame data is valid only during callback execution
 * 3. Spans and cells are views into backend-owned memory
 * 4. Callbacks must copy data if needed beyond callback scope
 * 5. Backend destruction frees all associated memory
 */

// ============================================================================
// Error Codes
// ============================================================================

typedef enum {
    UI_BACKEND_OK = 0,
    UI_BACKEND_ERR_INVALID_TYPE = -1,
    UI_BACKEND_ERR_NOMEM = -2,
    UI_BACKEND_ERR_ALREADY_REGISTERED = -3,
    UI_BACKEND_ERR_NOT_REGISTERED = -4,
    UI_BACKEND_ERR_INVALID_CAPS = -5,
    UI_BACKEND_ERR_FRAME_OVERFLOW = -6,
} ui_backend_error_t;

// Get error string
const char* ui_backend_error_string(ui_backend_error_t err);

#endif /* UI_BACKEND_H */