// backend_ghostty.c - Ghostty UI Backend Implementation
// Purpose: Native Ghostty backend for structured terminal rendering
// Author: INTG-001 (Zig-Ghostty Integration Specialist)
// Date: 2025-08-25
// Version: 1.0.0

#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>
#include <assert.h>
#include "ui_backend.h"
#include "tty_write_hooks.h"
#include "backend_router.h"

// ============================================================================
// Ghostty FFI Declarations
// ============================================================================

// Opaque handle to Ghostty terminal instance
typedef struct ghostty_terminal ghostty_terminal_t;

// Ghostty FFI functions (implemented in Zig)
extern ghostty_terminal_t* ghostty_ffi_create_terminal(void* user_data);
extern void ghostty_ffi_destroy_terminal(ghostty_terminal_t* term);
extern void ghostty_ffi_register_callbacks(ghostty_terminal_t* term, void* callbacks);
extern void ghostty_ffi_process_frame(ghostty_terminal_t* term, const ui_frame_t* frame);
extern void ghostty_ffi_flush_immediate(ghostty_terminal_t* term);
extern uint32_t ghostty_ffi_get_capabilities(void);

// ============================================================================
// Ghostty Backend Structure
// ============================================================================

typedef struct ghostty_backend {
    ui_backend_t base;              // Base UI backend interface
    ghostty_terminal_t* terminal;    // Ghostty terminal instance
    
    // Thread safety
    pthread_mutex_t frame_mutex;     // Protects frame aggregation
    pthread_rwlock_t state_lock;     // Protects backend state
    
    // Frame aggregation state
    frame_aggregator_t* aggregator;
    ui_frame_t* pending_frame;
    uint32_t pending_spans_count;
    
    // Grid optimization
    struct {
        uint32_t* dirty_rows;        // Bitmap of dirty rows
        uint32_t* dirty_cols;        // Bitmap of dirty columns per row
        uint32_t rows_capacity;
        uint32_t cols_capacity;
    } dirty_tracking;
    
    // Statistics
    atomic_uint_fast64_t frames_sent;
    atomic_uint_fast64_t cells_updated;
    atomic_uint_fast64_t frames_batched;
    
    // Configuration
    bool immediate_mode;             // Bypass batching for testing
    bool grid_optimization;          // Enable grid optimization
    uint32_t max_batch_size;         // Maximum spans per batch
} ghostty_backend_t;

// ============================================================================
// Grid Optimization Functions
// ============================================================================

static void mark_dirty_region(ghostty_backend_t* backend,
                              uint32_t row_start, uint32_t row_end,
                              uint32_t col_start, uint32_t col_end) {
    pthread_mutex_lock(&backend->frame_mutex);
    
    // Mark rows as dirty
    for (uint32_t r = row_start; r < row_end && r < backend->dirty_tracking.rows_capacity; r++) {
        backend->dirty_tracking.dirty_rows[r / 32] |= (1U << (r % 32));
        
        // Mark columns as dirty
        if (backend->dirty_tracking.dirty_cols) {
            uint32_t* col_bitmap = &backend->dirty_tracking.dirty_cols[r * (backend->dirty_tracking.cols_capacity / 32)];
            for (uint32_t c = col_start; c < col_end && c < backend->dirty_tracking.cols_capacity; c++) {
                col_bitmap[c / 32] |= (1U << (c % 32));
            }
        }
    }
    
    pthread_mutex_unlock(&backend->frame_mutex);
}

static void clear_dirty_tracking(ghostty_backend_t* backend) {
    if (backend->dirty_tracking.dirty_rows) {
        memset(backend->dirty_tracking.dirty_rows, 0,
               (backend->dirty_tracking.rows_capacity + 31) / 32 * sizeof(uint32_t));
    }
    if (backend->dirty_tracking.dirty_cols) {
        memset(backend->dirty_tracking.dirty_cols, 0,
               backend->dirty_tracking.rows_capacity * 
               (backend->dirty_tracking.cols_capacity + 31) / 32 * sizeof(uint32_t));
    }
}

// ============================================================================
// Command Callback Implementations (22 functions)
// ============================================================================

static void ghostty_cmd_cell(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    // Extract cell position and content from ctx
    uint32_t row = ctx->ocy;
    uint32_t col = ctx->ocx;
    
    // Mark single cell as dirty
    mark_dirty_region(gb, row, row + 1, col, col + 1);
    
    // Add to frame aggregator
    if (gb->aggregator && !gb->immediate_mode) {
        frame_aggregator_add_update(gb->aggregator, ctx);
        if (frame_aggregator_should_emit(gb->aggregator)) {
            ui_frame_t* frame = frame_aggregator_emit(gb->aggregator);
            ghostty_ffi_process_frame(gb->terminal, frame);
            atomic_fetch_add(&gb->frames_sent, 1);
        }
    } else {
        // Immediate mode: create single-cell frame
        ui_cell_t cell;
        ui_cell_from_grid(&cell, ctx->cell);
        
        ui_span_t span = {
            .row = row,
            .col_start = col,
            .col_end = col + 1,
            .cells = &cell,
            .flags = 0
        };
        
        ui_frame_t frame = {
            .size = sizeof(ui_frame_t),
            .frame_seq = atomic_fetch_add(&gb->frames_sent, 1),
            .timestamp_ns = get_time_ns(),
            .pane_id = ctx->wp ? ctx->wp->id : 0,
            .span_count = 1,
            .spans = &span,
            .flags = UI_FRAME_URGENT
        };
        
        ghostty_ffi_process_frame(gb->terminal, &frame);
    }
    
    atomic_fetch_add(&gb->cells_updated, 1);
}

static void ghostty_cmd_cells(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    uint32_t row = ctx->ocy;
    uint32_t col_start = ctx->ocx;
    uint32_t col_end = ctx->ocx + ctx->num;
    
    mark_dirty_region(gb, row, row + 1, col_start, col_end);
    
    if (gb->aggregator && !gb->immediate_mode) {
        frame_aggregator_add_update(gb->aggregator, ctx);
        if (frame_aggregator_should_emit(gb->aggregator)) {
            ui_frame_t* frame = frame_aggregator_emit(gb->aggregator);
            ghostty_ffi_process_frame(gb->terminal, frame);
            atomic_fetch_add(&gb->frames_sent, 1);
        }
    } else {
        // Immediate mode: send cells directly
        ghostty_ffi_flush_immediate(gb->terminal);
    }
    
    atomic_fetch_add(&gb->cells_updated, ctx->num);
}

static void ghostty_cmd_insertcharacter(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    // Insert moves cells to the right
    uint32_t row = ctx->ocy;
    uint32_t col_start = ctx->ocx;
    uint32_t col_end = ctx->sx; // To end of line
    
    mark_dirty_region(gb, row, row + 1, col_start, col_end);
    
    if (gb->aggregator && !gb->immediate_mode) {
        frame_aggregator_add_update(gb->aggregator, ctx);
    } else {
        ghostty_ffi_flush_immediate(gb->terminal);
    }
}

static void ghostty_cmd_deletecharacter(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    // Delete moves cells to the left
    uint32_t row = ctx->ocy;
    uint32_t col_start = ctx->ocx;
    uint32_t col_end = ctx->sx;
    
    mark_dirty_region(gb, row, row + 1, col_start, col_end);
    
    if (gb->aggregator && !gb->immediate_mode) {
        frame_aggregator_add_update(gb->aggregator, ctx);
    } else {
        ghostty_ffi_flush_immediate(gb->terminal);
    }
}

static void ghostty_cmd_clearcharacter(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    uint32_t row = ctx->ocy;
    uint32_t col_start = ctx->ocx;
    uint32_t col_end = ctx->ocx + ctx->num;
    
    mark_dirty_region(gb, row, row + 1, col_start, col_end);
    
    if (gb->aggregator && !gb->immediate_mode) {
        frame_aggregator_add_update(gb->aggregator, ctx);
    } else {
        ghostty_ffi_flush_immediate(gb->terminal);
    }
}

// Line operations
static void ghostty_cmd_insertline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    // Insert line affects all rows below
    uint32_t row_start = ctx->ocy;
    uint32_t row_end = ctx->orlower + 1;
    
    mark_dirty_region(gb, row_start, row_end, 0, ctx->sx);
    
    if (gb->aggregator && !gb->immediate_mode) {
        frame_aggregator_add_update(gb->aggregator, ctx);
    } else {
        ghostty_ffi_flush_immediate(gb->terminal);
    }
}

static void ghostty_cmd_deleteline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    uint32_t row_start = ctx->ocy;
    uint32_t row_end = ctx->orlower + 1;
    
    mark_dirty_region(gb, row_start, row_end, 0, ctx->sx);
    
    if (gb->aggregator && !gb->immediate_mode) {
        frame_aggregator_add_update(gb->aggregator, ctx);
    } else {
        ghostty_ffi_flush_immediate(gb->terminal);
    }
}

static void ghostty_cmd_clearline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    uint32_t row = ctx->ocy;
    mark_dirty_region(gb, row, row + 1, 0, ctx->sx);
    
    if (gb->aggregator && !gb->immediate_mode) {
        frame_aggregator_add_update(gb->aggregator, ctx);
    } else {
        ghostty_ffi_flush_immediate(gb->terminal);
    }
}

static void ghostty_cmd_clearendofline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    uint32_t row = ctx->ocy;
    uint32_t col_start = ctx->ocx;
    uint32_t col_end = ctx->sx;
    
    mark_dirty_region(gb, row, row + 1, col_start, col_end);
    
    if (gb->aggregator && !gb->immediate_mode) {
        frame_aggregator_add_update(gb->aggregator, ctx);
    } else {
        ghostty_ffi_flush_immediate(gb->terminal);
    }
}

static void ghostty_cmd_clearstartofline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    uint32_t row = ctx->ocy;
    uint32_t col_end = ctx->ocx + 1;
    
    mark_dirty_region(gb, row, row + 1, 0, col_end);
    
    if (gb->aggregator && !gb->immediate_mode) {
        frame_aggregator_add_update(gb->aggregator, ctx);
    } else {
        ghostty_ffi_flush_immediate(gb->terminal);
    }
}

// Screen operations
static void ghostty_cmd_clearscreen(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    // Full screen clear
    mark_dirty_region(gb, 0, ctx->sy, 0, ctx->sx);
    clear_dirty_tracking(gb); // Reset for fresh start
    
    // Force immediate flush for screen clear
    if (gb->aggregator) {
        ui_frame_t* frame = frame_aggregator_emit(gb->aggregator);
        if (frame) {
            frame->flags |= UI_FRAME_URGENT;
            ghostty_ffi_process_frame(gb->terminal, frame);
        }
    }
    ghostty_ffi_flush_immediate(gb->terminal);
}

static void ghostty_cmd_clearendofscreen(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    uint32_t row_start = ctx->ocy;
    uint32_t row_end = ctx->sy;
    
    mark_dirty_region(gb, row_start, row_end, 0, ctx->sx);
    
    if (gb->aggregator && !gb->immediate_mode) {
        frame_aggregator_add_update(gb->aggregator, ctx);
    } else {
        ghostty_ffi_flush_immediate(gb->terminal);
    }
}

static void ghostty_cmd_clearstartofscreen(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    uint32_t row_end = ctx->ocy + 1;
    
    mark_dirty_region(gb, 0, row_end, 0, ctx->sx);
    
    if (gb->aggregator && !gb->immediate_mode) {
        frame_aggregator_add_update(gb->aggregator, ctx);
    } else {
        ghostty_ffi_flush_immediate(gb->terminal);
    }
}

static void ghostty_cmd_alignmenttest(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    // Alignment test fills screen with 'E'
    mark_dirty_region(gb, 0, ctx->sy, 0, ctx->sx);
    
    if (gb->aggregator) {
        frame_aggregator_add_update(gb->aggregator, ctx);
        ui_frame_t* frame = frame_aggregator_emit(gb->aggregator);
        if (frame) {
            ghostty_ffi_process_frame(gb->terminal, frame);
        }
    }
}

// Scrolling operations
static void ghostty_cmd_reverseindex(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    // Reverse index affects scroll region
    uint32_t row_start = ctx->orupper;
    uint32_t row_end = ctx->orlower + 1;
    
    mark_dirty_region(gb, row_start, row_end, 0, ctx->sx);
    
    if (gb->aggregator && !gb->immediate_mode) {
        frame_aggregator_add_update(gb->aggregator, ctx);
    } else {
        ghostty_ffi_flush_immediate(gb->terminal);
    }
}

static void ghostty_cmd_linefeed(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    // Line feed may scroll
    uint32_t row_start = ctx->orupper;
    uint32_t row_end = ctx->orlower + 1;
    
    mark_dirty_region(gb, row_start, row_end, 0, ctx->sx);
    
    if (gb->aggregator && !gb->immediate_mode) {
        frame_aggregator_add_update(gb->aggregator, ctx);
    } else {
        ghostty_ffi_flush_immediate(gb->terminal);
    }
}

static void ghostty_cmd_scrollup(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    uint32_t row_start = ctx->orupper;
    uint32_t row_end = ctx->orlower + 1;
    
    mark_dirty_region(gb, row_start, row_end, 0, ctx->sx);
    
    if (gb->aggregator && !gb->immediate_mode) {
        frame_aggregator_add_update(gb->aggregator, ctx);
    } else {
        ghostty_ffi_flush_immediate(gb->terminal);
    }
}

static void ghostty_cmd_scrolldown(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    uint32_t row_start = ctx->orupper;
    uint32_t row_end = ctx->orlower + 1;
    
    mark_dirty_region(gb, row_start, row_end, 0, ctx->sx);
    
    if (gb->aggregator && !gb->immediate_mode) {
        frame_aggregator_add_update(gb->aggregator, ctx);
    } else {
        ghostty_ffi_flush_immediate(gb->terminal);
    }
}

// Special operations
static void ghostty_cmd_setselection(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    // Selection doesn't affect display directly
    // Pass through to Ghostty for clipboard handling
    if (gb->terminal) {
        ghostty_ffi_flush_immediate(gb->terminal);
    }
}

static void ghostty_cmd_rawstring(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    // Raw string bypass - send immediately
    ghostty_ffi_flush_immediate(gb->terminal);
}

static void ghostty_cmd_sixelimage(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    // Sixel images require special handling
    // Mark affected region as dirty
    mark_dirty_region(gb, ctx->ocy, ctx->ocy + ctx->sy, 0, ctx->sx);
    
    // Force immediate processing for images
    ghostty_ffi_flush_immediate(gb->terminal);
}

static void ghostty_cmd_syncstart(struct ui_backend* backend, const struct tty_ctx* ctx) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    // Sync start begins batching
    if (gb->aggregator) {
        // Reset aggregator for new sync sequence
        frame_aggregator_reset(gb->aggregator);
    }
}

// ============================================================================
// Backend Operations Table
// ============================================================================

static const ui_backend_ops_t ghostty_ops = {
    .size = sizeof(ui_backend_ops_t),
    .version = UI_BACKEND_ABI_VERSION,
    
    // Character/cell operations
    .cmd_cell = ghostty_cmd_cell,
    .cmd_cells = ghostty_cmd_cells,
    .cmd_insertcharacter = ghostty_cmd_insertcharacter,
    .cmd_deletecharacter = ghostty_cmd_deletecharacter,
    .cmd_clearcharacter = ghostty_cmd_clearcharacter,
    
    // Line operations
    .cmd_insertline = ghostty_cmd_insertline,
    .cmd_deleteline = ghostty_cmd_deleteline,
    .cmd_clearline = ghostty_cmd_clearline,
    .cmd_clearendofline = ghostty_cmd_clearendofline,
    .cmd_clearstartofline = ghostty_cmd_clearstartofline,
    
    // Screen operations
    .cmd_clearscreen = ghostty_cmd_clearscreen,
    .cmd_clearendofscreen = ghostty_cmd_clearendofscreen,
    .cmd_clearstartofscreen = ghostty_cmd_clearstartofscreen,
    .cmd_alignmenttest = ghostty_cmd_alignmenttest,
    
    // Scrolling operations
    .cmd_reverseindex = ghostty_cmd_reverseindex,
    .cmd_linefeed = ghostty_cmd_linefeed,
    .cmd_scrollup = ghostty_cmd_scrollup,
    .cmd_scrolldown = ghostty_cmd_scrolldown,
    
    // Special operations
    .cmd_setselection = ghostty_cmd_setselection,
    .cmd_rawstring = ghostty_cmd_rawstring,
    .cmd_sixelimage = ghostty_cmd_sixelimage,
    .cmd_syncstart = ghostty_cmd_syncstart,
};

// ============================================================================
// Callback Registration
// ============================================================================

static void on_ghostty_frame(const ui_frame_t* frame, void* user_data) {
    ghostty_backend_t* gb = (ghostty_backend_t*)user_data;
    
    // Process frame callback from Ghostty
    if (gb->base.on_frame) {
        gb->base.on_frame(frame, gb->base.user_data);
    }
}

static void on_ghostty_bell(uint32_t pane_id, void* user_data) {
    ghostty_backend_t* gb = (ghostty_backend_t*)user_data;
    
    if (gb->base.on_bell) {
        gb->base.on_bell(pane_id, gb->base.user_data);
    }
}

static void on_ghostty_title(uint32_t pane_id, const char* title, void* user_data) {
    ghostty_backend_t* gb = (ghostty_backend_t*)user_data;
    
    if (gb->base.on_title) {
        gb->base.on_title(pane_id, title, gb->base.user_data);
    }
}

// ============================================================================
// Backend Management Functions
// ============================================================================

struct ui_backend* ghostty_backend_create(const ui_capabilities_t* requested_caps) {
    ghostty_backend_t* gb = calloc(1, sizeof(ghostty_backend_t));
    if (!gb) return NULL;
    
    // Initialize base structure
    gb->base.size = sizeof(ui_backend_t);
    gb->base.version = UI_BACKEND_ABI_VERSION;
    gb->base.type = UI_BACKEND_GHOSTTY;
    gb->base.ops = &ghostty_ops;
    
    // Initialize thread safety
    pthread_mutex_init(&gb->frame_mutex, NULL);
    pthread_rwlock_init(&gb->state_lock, NULL);
    
    // Create Ghostty terminal instance
    gb->terminal = ghostty_ffi_create_terminal(gb);
    if (!gb->terminal) {
        free(gb);
        return NULL;
    }
    
    // Set up capabilities
    gb->base.capabilities.size = sizeof(ui_capabilities_t);
    gb->base.capabilities.version = UI_BACKEND_ABI_VERSION;
    gb->base.capabilities.supported = ghostty_ffi_get_capabilities();
    gb->base.capabilities.max_fps = 144;
    gb->base.capabilities.optimal_batch_size = 100;
    gb->base.capabilities.max_dirty_rects = 16;
    
    // Configure based on requested capabilities
    if (requested_caps) {
        if (requested_caps->supported & UI_CAP_FRAME_BATCH) {
            gb->aggregator = frame_aggregator_create(requested_caps->max_fps);
            gb->immediate_mode = false;
        } else {
            gb->immediate_mode = true;
        }
        
        gb->grid_optimization = true;
        gb->max_batch_size = requested_caps->optimal_batch_size;
    } else {
        // Default configuration
        gb->aggregator = frame_aggregator_create(60);
        gb->immediate_mode = false;
        gb->grid_optimization = true;
        gb->max_batch_size = 100;
    }
    
    // Initialize dirty tracking
    gb->dirty_tracking.rows_capacity = 1000;  // Default terminal rows
    gb->dirty_tracking.cols_capacity = 200;   // Default terminal cols
    gb->dirty_tracking.dirty_rows = calloc((gb->dirty_tracking.rows_capacity + 31) / 32,
                                           sizeof(uint32_t));
    gb->dirty_tracking.dirty_cols = calloc(gb->dirty_tracking.rows_capacity *
                                           (gb->dirty_tracking.cols_capacity + 31) / 32,
                                           sizeof(uint32_t));
    
    // Register callbacks with Ghostty
    struct {
        void (*on_frame)(const ui_frame_t*, void*);
        void (*on_bell)(uint32_t, void*);
        void (*on_title)(uint32_t, const char*, void*);
        void* user_data;
    } callbacks = {
        .on_frame = on_ghostty_frame,
        .on_bell = on_ghostty_bell,
        .on_title = on_ghostty_title,
        .user_data = gb
    };
    
    ghostty_ffi_register_callbacks(gb->terminal, &callbacks);
    
    return &gb->base;
}

void ghostty_backend_destroy(struct ui_backend* backend) {
    if (!backend) return;
    
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    // Destroy Ghostty terminal
    if (gb->terminal) {
        ghostty_ffi_destroy_terminal(gb->terminal);
    }
    
    // Clean up aggregator
    if (gb->aggregator) {
        frame_aggregator_destroy(gb->aggregator);
    }
    
    // Free dirty tracking
    free(gb->dirty_tracking.dirty_rows);
    free(gb->dirty_tracking.dirty_cols);
    
    // Destroy synchronization primitives
    pthread_mutex_destroy(&gb->frame_mutex);
    pthread_rwlock_destroy(&gb->state_lock);
    
    free(gb);
}

// ============================================================================
// Helper Functions
// ============================================================================

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// ============================================================================
// Public API
// ============================================================================

void ghostty_backend_set_immediate_mode(struct ui_backend* backend, bool immediate) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    pthread_rwlock_wrlock(&gb->state_lock);
    gb->immediate_mode = immediate;
    pthread_rwlock_unlock(&gb->state_lock);
}

void ghostty_backend_set_grid_optimization(struct ui_backend* backend, bool enabled) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    pthread_rwlock_wrlock(&gb->state_lock);
    gb->grid_optimization = enabled;
    pthread_rwlock_unlock(&gb->state_lock);
}

void ghostty_backend_get_statistics(struct ui_backend* backend,
                                    uint64_t* frames_sent,
                                    uint64_t* cells_updated,
                                    uint64_t* frames_batched) {
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    
    if (frames_sent) *frames_sent = atomic_load(&gb->frames_sent);
    if (cells_updated) *cells_updated = atomic_load(&gb->cells_updated);
    if (frames_batched) *frames_batched = atomic_load(&gb->frames_batched);
}