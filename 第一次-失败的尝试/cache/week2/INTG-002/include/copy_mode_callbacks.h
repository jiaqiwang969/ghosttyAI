// copy_mode_callbacks.h - Copy Mode vtable interface
// Purpose: Define callback interface for tmux copy mode operations
// Author: INTG-002 (integration-dev)
// Date: 2025-08-26
// Task: T-204 - Copy mode processing with clipboard integration
// Performance: <10ms selection update, <50ms copy for 10MB

#ifndef COPY_MODE_CALLBACKS_H
#define COPY_MODE_CALLBACKS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
struct copy_mode_backend;
struct tty_ctx;
struct grid_cell;
typedef struct copy_mode_backend copy_mode_backend_t;

// Selection modes
typedef enum {
    SEL_MODE_CHAR,      // Character selection
    SEL_MODE_WORD,      // Word selection
    SEL_MODE_LINE,      // Line selection
    SEL_MODE_RECT,      // Rectangular/block selection
    SEL_MODE_URL,       // URL detection mode
} selection_mode_t;

// Selection state
typedef struct {
    uint32_t start_row;
    uint32_t start_col;
    uint32_t end_row;
    uint32_t end_col;
    selection_mode_t mode;
    bool active;
    bool backwards;     // Selection direction
} selection_state_t;

// Clipboard format types
typedef enum {
    CLIPBOARD_TEXT,     // Plain text
    CLIPBOARD_RTF,      // Rich text format
    CLIPBOARD_HTML,     // HTML formatted
    CLIPBOARD_ANSI,     // With ANSI colors
} clipboard_format_t;

// Copy buffer entry
typedef struct copy_buffer_entry {
    char* data;
    size_t size;
    clipboard_format_t format;
    uint64_t timestamp;
    struct copy_buffer_entry* next;
} copy_buffer_entry_t;

// Copy mode statistics
typedef struct {
    uint64_t selections_made;
    uint64_t bytes_copied;
    uint64_t paste_operations;
    uint64_t search_operations;
    uint64_t total_selection_time_ms;
    uint32_t largest_selection_bytes;
} copy_mode_stats_t;

// Copy mode vtable
typedef struct copy_mode_vtable {
    // Lifecycle
    int (*enter)(copy_mode_backend_t* backend, struct tty_ctx* ctx);
    int (*exit)(copy_mode_backend_t* backend, struct tty_ctx* ctx);
    
    // Selection operations
    int (*select_start)(copy_mode_backend_t* backend, uint32_t row, uint32_t col);
    int (*select_update)(copy_mode_backend_t* backend, uint32_t row, uint32_t col);
    int (*select_end)(copy_mode_backend_t* backend);
    int (*select_clear)(copy_mode_backend_t* backend);
    int (*select_all)(copy_mode_backend_t* backend);
    
    // Selection mode changes
    int (*set_mode)(copy_mode_backend_t* backend, selection_mode_t mode);
    int (*toggle_rect)(copy_mode_backend_t* backend);
    
    // Copy/paste operations
    int (*copy_selection)(copy_mode_backend_t* backend, clipboard_format_t format);
    int (*copy_pipe)(copy_mode_backend_t* backend, const char* command);
    int (*paste)(copy_mode_backend_t* backend, uint32_t buffer_index);
    int (*paste_system)(copy_mode_backend_t* backend);
    
    // Movement operations
    int (*move_up)(copy_mode_backend_t* backend, uint32_t lines);
    int (*move_down)(copy_mode_backend_t* backend, uint32_t lines);
    int (*move_left)(copy_mode_backend_t* backend, uint32_t cols);
    int (*move_right)(copy_mode_backend_t* backend, uint32_t cols);
    int (*move_word_forward)(copy_mode_backend_t* backend);
    int (*move_word_backward)(copy_mode_backend_t* backend);
    int (*move_start_of_line)(copy_mode_backend_t* backend);
    int (*move_end_of_line)(copy_mode_backend_t* backend);
    int (*move_top)(copy_mode_backend_t* backend);
    int (*move_bottom)(copy_mode_backend_t* backend);
    int (*move_middle)(copy_mode_backend_t* backend);
    
    // Page operations
    int (*page_up)(copy_mode_backend_t* backend);
    int (*page_down)(copy_mode_backend_t* backend);
    int (*halfpage_up)(copy_mode_backend_t* backend);
    int (*halfpage_down)(copy_mode_backend_t* backend);
    
    // Search operations
    int (*search_forward)(copy_mode_backend_t* backend, const char* pattern);
    int (*search_backward)(copy_mode_backend_t* backend, const char* pattern);
    int (*search_next)(copy_mode_backend_t* backend);
    int (*search_prev)(copy_mode_backend_t* backend);
    int (*search_clear)(copy_mode_backend_t* backend);
    
    // Jump operations
    int (*jump_forward)(copy_mode_backend_t* backend, char target);
    int (*jump_backward)(copy_mode_backend_t* backend, char target);
    int (*jump_again)(copy_mode_backend_t* backend);
    int (*jump_reverse)(copy_mode_backend_t* backend);
    
    // History buffer operations
    int (*history_top)(copy_mode_backend_t* backend);
    int (*history_bottom)(copy_mode_backend_t* backend);
    int (*scroll_up)(copy_mode_backend_t* backend, uint32_t lines);
    int (*scroll_down)(copy_mode_backend_t* backend, uint32_t lines);
    
    // Buffer management
    int (*append_selection)(copy_mode_backend_t* backend);
    int (*clear_buffer)(copy_mode_backend_t* backend, uint32_t buffer_index);
    int (*list_buffers)(copy_mode_backend_t* backend, copy_buffer_entry_t** list);
    int (*save_buffer)(copy_mode_backend_t* backend, const char* path);
    int (*load_buffer)(copy_mode_backend_t* backend, const char* path);
    
    // Rendering hints
    int (*get_selection_cells)(copy_mode_backend_t* backend, 
                              uint32_t* cells, size_t* count);
    int (*should_highlight)(copy_mode_backend_t* backend, 
                           uint32_t row, uint32_t col);
    
    // Statistics
    void (*get_stats)(copy_mode_backend_t* backend, copy_mode_stats_t* stats);
    void (*reset_stats)(copy_mode_backend_t* backend);
} copy_mode_vtable_t;

// Key binding modes
typedef enum {
    KEY_MODE_VI,        // Vi-style bindings
    KEY_MODE_EMACS,     // Emacs-style bindings
    KEY_MODE_CUSTOM,    // User-defined
} key_mode_t;

// Copy mode backend structure
struct copy_mode_backend {
    copy_mode_vtable_t* vtable;
    void* priv;                    // Private implementation data
    
    // Current state
    selection_state_t selection;
    key_mode_t key_mode;
    bool active;
    
    // Cursor position
    uint32_t cursor_row;
    uint32_t cursor_col;
    
    // View position (scrollback)
    uint32_t view_top;
    uint32_t view_height;
    uint32_t view_width;
    
    // Search state
    char* search_pattern;
    bool search_forward;
    uint32_t search_match_row;
    uint32_t search_match_col;
    
    // Jump state
    char jump_char;
    bool jump_forward;
    
    // Copy buffers (circular buffer history)
    copy_buffer_entry_t* buffers[10];
    uint32_t buffer_index;
    
    // Statistics
    copy_mode_stats_t stats;
    
    // Performance tracking
    uint64_t last_update_ns;
    uint32_t pending_renders;
};

// ============================================================================
// Public API
// ============================================================================

// Initialize copy mode backend
copy_mode_backend_t* copy_mode_init(key_mode_t mode);

// Cleanup copy mode backend
void copy_mode_cleanup(copy_mode_backend_t* backend);

// Handle key input in copy mode
int copy_mode_key(copy_mode_backend_t* backend, int key, int modifiers);

// Handle mouse input in copy mode
int copy_mode_mouse(copy_mode_backend_t* backend, 
                   uint32_t button, uint32_t row, uint32_t col, int modifiers);

// Update grid dimensions
int copy_mode_resize(copy_mode_backend_t* backend, 
                    uint32_t rows, uint32_t cols);

// Get current selection as string
char* copy_mode_get_selection(copy_mode_backend_t* backend, size_t* size);

// Check if position is in selection
bool copy_mode_in_selection(copy_mode_backend_t* backend, 
                           uint32_t row, uint32_t col);

// Integration with event loop
void copy_mode_set_event_handle(copy_mode_backend_t* backend, void* handle);

// ============================================================================
// Grid access callbacks (provided by tmux)
// ============================================================================

typedef struct grid_callbacks {
    // Get cell at position
    struct grid_cell* (*get_cell)(void* grid, uint32_t row, uint32_t col);
    
    // Get line text
    char* (*get_line)(void* grid, uint32_t row, size_t* len);
    
    // Get grid dimensions
    void (*get_size)(void* grid, uint32_t* rows, uint32_t* cols);
    
    // Get history size
    uint32_t (*get_history_size)(void* grid);
    
    // Mark region dirty for rendering
    void (*mark_dirty)(void* grid, uint32_t start_row, uint32_t end_row);
} grid_callbacks_t;

// Set grid access callbacks
void copy_mode_set_grid_callbacks(copy_mode_backend_t* backend, 
                                 grid_callbacks_t* callbacks, void* grid);

// ============================================================================
// System clipboard integration
// ============================================================================

// Clipboard operations
int clipboard_set(const char* data, size_t size, clipboard_format_t format);
int clipboard_get(char** data, size_t* size, clipboard_format_t format);
bool clipboard_has_data(clipboard_format_t format);

// Platform-specific initialization
int clipboard_init(void);
void clipboard_cleanup(void);

#endif // COPY_MODE_CALLBACKS_H