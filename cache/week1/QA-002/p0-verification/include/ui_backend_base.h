// ui_backend.h - UI Backend Core Definitions
// Author: ARCH-001 (System Architect)
// Purpose: Core types and structures for UI backend
// Version: 2.0.0

#ifndef UI_BACKEND_H
#define UI_BACKEND_H

#include <stdint.h>
#include <stddef.h>

// Forward declarations
struct tty_ctx;
struct screen;
struct window_pane;

// ============================================================================
// Core Type Definitions
// ============================================================================

// Backend type enumeration
typedef enum {
    UI_BACKEND_TTY = 0,
    UI_BACKEND_GHOSTTY = 1,
    UI_BACKEND_TEST = 2,
    UI_BACKEND_MAX
} ui_backend_type_t;

// Capability flags
typedef enum {
    UI_CAP_FRAME_BATCH = (1 << 0),     // Supports frame batching
    UI_CAP_24BIT_COLOR = (1 << 1),     // Supports 24-bit color
    UI_CAP_BORDERS_BY_UI = (1 << 2),   // UI draws borders
    UI_CAP_SYNCHRONIZED = (1 << 3),     // Supports synchronized updates
} ui_capability_flags_t;

// UI capabilities structure
typedef struct ui_capabilities {
    uint32_t supported;             // Bitmask of ui_capability_flags_t
    uint32_t max_fps;              // Maximum FPS supported
    uint32_t optimal_batch_size;   // Optimal batch size for operations
} ui_capabilities_t;

// Cell attributes
typedef struct {
    uint32_t fg;        // Foreground color (RGB)
    uint32_t bg;        // Background color (RGB)
    uint32_t flags;     // Attribute flags (bold, italic, etc.)
} ui_cell_attr_t;

// UI cell (character + attributes)
typedef struct {
    uint8_t data[4];        // UTF-8 character data
    ui_cell_attr_t attr;    // Cell attributes
} ui_cell_t;

// UI span (continuous range of cells)
typedef struct {
    uint32_t row;           // Row position
    uint32_t col_start;     // Starting column
    uint32_t col_end;       // Ending column (exclusive)
    ui_cell_t* cells;       // Array of cells
    uint32_t flags;         // Span flags
} ui_span_t;

// Frame types
typedef enum {
    FRAME_TYPE_NORMAL = 0,
    FRAME_TYPE_CLEAR = 1,
    FRAME_TYPE_SCROLL = 2,
    FRAME_TYPE_URGENT = 3,
} ui_frame_type_t;

// UI frame (collection of spans)
typedef struct ui_frame {
    ui_frame_type_t type;
    uint32_t urgent;        // Non-zero for urgent frames
    uint64_t timestamp;     // Frame timestamp
    ui_span_t* spans;       // Array of spans
    size_t span_count;      // Number of spans
} ui_frame_t;

// Backend statistics
typedef struct {
    uint64_t commands_processed;
    uint64_t frames_emitted;
    uint64_t callbacks_invoked;
    uint64_t errors;
} ui_backend_stats_t;

// UI Backend structure
struct ui_backend {
    uint32_t size;                  // Structure size for ABI stability
    uint32_t version;               // Backend version
    ui_backend_type_t type;         // Backend type
    void* ops;                      // Operations table pointer
    ui_capabilities_t capabilities; // Backend capabilities
    ui_backend_stats_t stats;       // Statistics
    
    // Callbacks
    void (*on_frame)(void* context, const ui_frame_t* frame);
    void (*on_error)(void* context, int error_code, const char* message);
    
    // Private data
    void* callback_context;
    void* aggregator;
    void* private_data;
};

#endif /* UI_BACKEND_H */