// copy_mode_backend.c - Copy mode implementation with Ghostty integration
// Purpose: Extract and implement tmux copy mode with clipboard support
// Author: INTG-002 (integration-dev)
// Date: 2025-08-26
// Task: T-204 - Copy mode processing implementation
// Performance: <10ms selection, <50ms for 10MB copy

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>
#include "../include/copy_mode_callbacks.h"

// ============================================================================
// Internal structures
// ============================================================================

typedef struct {
    grid_callbacks_t* grid_callbacks;
    void* grid;
    
    // Word detection state
    char* word_separators;
    
    // URL detection patterns
    char* url_patterns[10];
    int url_pattern_count;
    
    // Performance optimization
    char* selection_cache;
    size_t cache_size;
    bool cache_valid;
    
    // Rendering optimization
    uint32_t* highlight_cells;
    size_t highlight_count;
    size_t highlight_capacity;
    
} copy_mode_private_t;

// ============================================================================
// Helper functions
// ============================================================================

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static void invalidate_cache(copy_mode_private_t* priv) {
    if (priv->selection_cache) {
        free(priv->selection_cache);
        priv->selection_cache = NULL;
    }
    priv->cache_size = 0;
    priv->cache_valid = false;
}

static bool is_word_separator(char c, const char* separators) {
    if (!separators) {
        separators = " \t\n.,;:!?()[]{}\"'`";
    }
    return strchr(separators, c) != NULL;
}

static void expand_to_word(copy_mode_backend_t* backend, 
                          uint32_t row, uint32_t col,
                          uint32_t* start_col, uint32_t* end_col) {
    copy_mode_private_t* priv = (copy_mode_private_t*)backend->priv;
    
    if (!priv->grid_callbacks || !priv->grid) {
        *start_col = col;
        *end_col = col;
        return;
    }
    
    size_t line_len;
    char* line = priv->grid_callbacks->get_line(priv->grid, row, &line_len);
    if (!line || col >= line_len) {
        *start_col = col;
        *end_col = col;
        if (line) free(line);
        return;
    }
    
    // Find word start
    uint32_t start = col;
    while (start > 0 && !is_word_separator(line[start - 1], priv->word_separators)) {
        start--;
    }
    
    // Find word end
    uint32_t end = col;
    while (end < line_len && !is_word_separator(line[end], priv->word_separators)) {
        end++;
    }
    
    *start_col = start;
    *end_col = end;
    free(line);
}

static void update_highlight_cells(copy_mode_backend_t* backend) {
    copy_mode_private_t* priv = (copy_mode_private_t*)backend->priv;
    selection_state_t* sel = &backend->selection;
    
    if (!sel->active) {
        priv->highlight_count = 0;
        return;
    }
    
    // Calculate cells to highlight
    uint32_t start_row = sel->start_row < sel->end_row ? sel->start_row : sel->end_row;
    uint32_t end_row = sel->start_row > sel->end_row ? sel->start_row : sel->end_row;
    uint32_t start_col = sel->start_col < sel->end_col ? sel->start_col : sel->end_col;
    uint32_t end_col = sel->start_col > sel->end_col ? sel->start_col : sel->end_col;
    
    size_t needed = (end_row - start_row + 1) * (end_col - start_col + 1);
    
    // Resize highlight array if needed
    if (needed > priv->highlight_capacity) {
        priv->highlight_capacity = needed * 2;
        priv->highlight_cells = realloc(priv->highlight_cells, 
                                       priv->highlight_capacity * sizeof(uint32_t));
    }
    
    // Fill highlight cells
    priv->highlight_count = 0;
    for (uint32_t row = start_row; row <= end_row; row++) {
        for (uint32_t col = start_col; col <= end_col; col++) {
            priv->highlight_cells[priv->highlight_count++] = (row << 16) | col;
        }
    }
}

// ============================================================================
// Lifecycle operations
// ============================================================================

static int copy_mode_enter_impl(copy_mode_backend_t* backend, struct tty_ctx* ctx) {
    (void)ctx;
    
    backend->active = true;
    backend->stats.selections_made = 0;
    backend->last_update_ns = get_time_ns();
    
    // Initialize cursor at current position
    backend->cursor_row = backend->view_top + backend->view_height / 2;
    backend->cursor_col = 0;
    
    return 0;
}

static int copy_mode_exit_impl(copy_mode_backend_t* backend, struct tty_ctx* ctx) {
    (void)ctx;
    
    copy_mode_private_t* priv = (copy_mode_private_t*)backend->priv;
    
    // Clear selection
    backend->selection.active = false;
    invalidate_cache(priv);
    
    // Clear search
    if (backend->search_pattern) {
        free(backend->search_pattern);
        backend->search_pattern = NULL;
    }
    
    backend->active = false;
    return 0;
}

// ============================================================================
// Selection operations
// ============================================================================

static int select_start_impl(copy_mode_backend_t* backend, uint32_t row, uint32_t col) {
    copy_mode_private_t* priv = (copy_mode_private_t*)backend->priv;
    
    backend->selection.start_row = row;
    backend->selection.start_col = col;
    backend->selection.end_row = row;
    backend->selection.end_col = col;
    backend->selection.active = true;
    
    invalidate_cache(priv);
    update_highlight_cells(backend);
    
    backend->stats.selections_made++;
    backend->last_update_ns = get_time_ns();
    
    return 0;
}

static int select_update_impl(copy_mode_backend_t* backend, uint32_t row, uint32_t col) {
    if (!backend->selection.active) {
        return -1;
    }
    
    copy_mode_private_t* priv = (copy_mode_private_t*)backend->priv;
    
    backend->selection.end_row = row;
    backend->selection.end_col = col;
    
    // Handle different selection modes
    switch (backend->selection.mode) {
        case SEL_MODE_WORD: {
            uint32_t start_col, end_col;
            expand_to_word(backend, row, col, &start_col, &end_col);
            backend->selection.end_col = end_col;
            break;
        }
        case SEL_MODE_LINE:
            backend->selection.start_col = 0;
            backend->selection.end_col = backend->view_width - 1;
            break;
        case SEL_MODE_RECT:
            // Rectangular selection maintains column positions
            break;
        default:
            break;
    }
    
    invalidate_cache(priv);
    update_highlight_cells(backend);
    
    backend->last_update_ns = get_time_ns();
    backend->pending_renders++;
    
    return 0;
}

static int select_end_impl(copy_mode_backend_t* backend) {
    if (!backend->selection.active) {
        return -1;
    }
    
    uint64_t elapsed_ms = (get_time_ns() - backend->last_update_ns) / 1000000;
    backend->stats.total_selection_time_ms += elapsed_ms;
    
    return 0;
}

static int select_clear_impl(copy_mode_backend_t* backend) {
    copy_mode_private_t* priv = (copy_mode_private_t*)backend->priv;
    
    backend->selection.active = false;
    invalidate_cache(priv);
    priv->highlight_count = 0;
    
    return 0;
}

static int select_all_impl(copy_mode_backend_t* backend) {
    copy_mode_private_t* priv = (copy_mode_private_t*)backend->priv;
    
    if (!priv->grid_callbacks || !priv->grid) {
        return -1;
    }
    
    uint32_t rows, cols;
    priv->grid_callbacks->get_size(priv->grid, &rows, &cols);
    
    backend->selection.start_row = 0;
    backend->selection.start_col = 0;
    backend->selection.end_row = rows - 1;
    backend->selection.end_col = cols - 1;
    backend->selection.active = true;
    backend->selection.mode = SEL_MODE_CHAR;
    
    invalidate_cache(priv);
    update_highlight_cells(backend);
    
    return 0;
}

// ============================================================================
// Copy/paste operations
// ============================================================================

static int copy_selection_impl(copy_mode_backend_t* backend, clipboard_format_t format) {
    if (!backend->selection.active) {
        return -1;
    }
    
    // Get selection text
    size_t size;
    char* text = copy_mode_get_selection(backend, &size);
    if (!text) {
        return -1;
    }
    
    // Store in internal buffer
    if (backend->buffers[backend->buffer_index]) {
        free(backend->buffers[backend->buffer_index]->data);
        free(backend->buffers[backend->buffer_index]);
    }
    
    copy_buffer_entry_t* entry = malloc(sizeof(copy_buffer_entry_t));
    entry->data = text;
    entry->size = size;
    entry->format = format;
    entry->timestamp = time(NULL);
    entry->next = NULL;
    
    backend->buffers[backend->buffer_index] = entry;
    backend->buffer_index = (backend->buffer_index + 1) % 10;
    
    // Copy to system clipboard
    int result = clipboard_set(text, size, format);
    
    // Update statistics
    backend->stats.bytes_copied += size;
    if (size > backend->stats.largest_selection_bytes) {
        backend->stats.largest_selection_bytes = size;
    }
    
    return result;
}

static int paste_impl(copy_mode_backend_t* backend, uint32_t buffer_index) {
    if (buffer_index >= 10 || !backend->buffers[buffer_index]) {
        return -1;
    }
    
    // Here we would send the paste data to the terminal
    // For now, just update stats
    backend->stats.paste_operations++;
    
    return 0;
}

static int paste_system_impl(copy_mode_backend_t* backend) {
    char* data = NULL;
    size_t size = 0;
    
    if (clipboard_get(&data, &size, CLIPBOARD_TEXT) < 0) {
        return -1;
    }
    
    // Here we would send the paste data to the terminal
    backend->stats.paste_operations++;
    
    free(data);
    return 0;
}

// ============================================================================
// Movement operations
// ============================================================================

static int move_up_impl(copy_mode_backend_t* backend, uint32_t lines) {
    if (backend->cursor_row >= lines) {
        backend->cursor_row -= lines;
    } else {
        backend->cursor_row = 0;
    }
    
    if (backend->selection.active) {
        select_update_impl(backend, backend->cursor_row, backend->cursor_col);
    }
    
    return 0;
}

static int move_down_impl(copy_mode_backend_t* backend, uint32_t lines) {
    backend->cursor_row += lines;
    
    // Clamp to grid bounds
    if (backend->cursor_row >= backend->view_height) {
        backend->cursor_row = backend->view_height - 1;
    }
    
    if (backend->selection.active) {
        select_update_impl(backend, backend->cursor_row, backend->cursor_col);
    }
    
    return 0;
}

static int move_left_impl(copy_mode_backend_t* backend, uint32_t cols) {
    if (backend->cursor_col >= cols) {
        backend->cursor_col -= cols;
    } else {
        backend->cursor_col = 0;
    }
    
    if (backend->selection.active) {
        select_update_impl(backend, backend->cursor_row, backend->cursor_col);
    }
    
    return 0;
}

static int move_right_impl(copy_mode_backend_t* backend, uint32_t cols) {
    backend->cursor_col += cols;
    
    // Clamp to grid bounds
    if (backend->cursor_col >= backend->view_width) {
        backend->cursor_col = backend->view_width - 1;
    }
    
    if (backend->selection.active) {
        select_update_impl(backend, backend->cursor_row, backend->cursor_col);
    }
    
    return 0;
}

static int move_word_forward_impl(copy_mode_backend_t* backend) {
    copy_mode_private_t* priv = (copy_mode_private_t*)backend->priv;
    
    if (!priv->grid_callbacks || !priv->grid) {
        return -1;
    }
    
    size_t line_len;
    char* line = priv->grid_callbacks->get_line(priv->grid, backend->cursor_row, &line_len);
    if (!line) {
        return -1;
    }
    
    uint32_t col = backend->cursor_col;
    
    // Skip current word
    while (col < line_len && !is_word_separator(line[col], priv->word_separators)) {
        col++;
    }
    
    // Skip whitespace
    while (col < line_len && is_word_separator(line[col], priv->word_separators)) {
        col++;
    }
    
    backend->cursor_col = col;
    free(line);
    
    if (backend->selection.active) {
        select_update_impl(backend, backend->cursor_row, backend->cursor_col);
    }
    
    return 0;
}

static int move_word_backward_impl(copy_mode_backend_t* backend) {
    copy_mode_private_t* priv = (copy_mode_private_t*)backend->priv;
    
    if (!priv->grid_callbacks || !priv->grid) {
        return -1;
    }
    
    size_t line_len;
    char* line = priv->grid_callbacks->get_line(priv->grid, backend->cursor_row, &line_len);
    if (!line) {
        return -1;
    }
    
    uint32_t col = backend->cursor_col;
    
    // Skip whitespace backwards
    while (col > 0 && is_word_separator(line[col - 1], priv->word_separators)) {
        col--;
    }
    
    // Skip to word start
    while (col > 0 && !is_word_separator(line[col - 1], priv->word_separators)) {
        col--;
    }
    
    backend->cursor_col = col;
    free(line);
    
    if (backend->selection.active) {
        select_update_impl(backend, backend->cursor_row, backend->cursor_col);
    }
    
    return 0;
}

// ============================================================================
// Search operations
// ============================================================================

static int search_forward_impl(copy_mode_backend_t* backend, const char* pattern) {
    copy_mode_private_t* priv = (copy_mode_private_t*)backend->priv;
    
    if (!priv->grid_callbacks || !priv->grid || !pattern) {
        return -1;
    }
    
    // Store search pattern
    if (backend->search_pattern) {
        free(backend->search_pattern);
    }
    backend->search_pattern = strdup(pattern);
    backend->search_forward = true;
    
    // Search from current position
    uint32_t rows, cols;
    priv->grid_callbacks->get_size(priv->grid, &rows, &cols);
    
    for (uint32_t row = backend->cursor_row; row < rows; row++) {
        size_t line_len;
        char* line = priv->grid_callbacks->get_line(priv->grid, row, &line_len);
        if (!line) continue;
        
        char* match = strstr(line, pattern);
        if (match) {
            backend->search_match_row = row;
            backend->search_match_col = match - line;
            backend->cursor_row = row;
            backend->cursor_col = backend->search_match_col;
            free(line);
            
            backend->stats.search_operations++;
            return 0;
        }
        free(line);
    }
    
    return -1; // Not found
}

static int search_backward_impl(copy_mode_backend_t* backend, const char* pattern) {
    copy_mode_private_t* priv = (copy_mode_private_t*)backend->priv;
    
    if (!priv->grid_callbacks || !priv->grid || !pattern) {
        return -1;
    }
    
    // Store search pattern
    if (backend->search_pattern) {
        free(backend->search_pattern);
    }
    backend->search_pattern = strdup(pattern);
    backend->search_forward = false;
    
    // Search backwards from current position
    for (int row = backend->cursor_row; row >= 0; row--) {
        size_t line_len;
        char* line = priv->grid_callbacks->get_line(priv->grid, row, &line_len);
        if (!line) continue;
        
        char* match = strstr(line, pattern);
        if (match) {
            backend->search_match_row = row;
            backend->search_match_col = match - line;
            backend->cursor_row = row;
            backend->cursor_col = backend->search_match_col;
            free(line);
            
            backend->stats.search_operations++;
            return 0;
        }
        free(line);
    }
    
    return -1; // Not found
}

// ============================================================================
// Vtable initialization
// ============================================================================

static copy_mode_vtable_t copy_mode_vtable = {
    // Lifecycle
    .enter = copy_mode_enter_impl,
    .exit = copy_mode_exit_impl,
    
    // Selection operations
    .select_start = select_start_impl,
    .select_update = select_update_impl,
    .select_end = select_end_impl,
    .select_clear = select_clear_impl,
    .select_all = select_all_impl,
    
    // Copy/paste operations
    .copy_selection = copy_selection_impl,
    .paste = paste_impl,
    .paste_system = paste_system_impl,
    
    // Movement operations
    .move_up = move_up_impl,
    .move_down = move_down_impl,
    .move_left = move_left_impl,
    .move_right = move_right_impl,
    .move_word_forward = move_word_forward_impl,
    .move_word_backward = move_word_backward_impl,
    
    // Search operations
    .search_forward = search_forward_impl,
    .search_backward = search_backward_impl,
    
    // Other operations would be implemented similarly...
};

// ============================================================================
// Public API
// ============================================================================

copy_mode_backend_t* copy_mode_init(key_mode_t mode) {
    copy_mode_backend_t* backend = calloc(1, sizeof(copy_mode_backend_t));
    if (!backend) {
        return NULL;
    }
    
    copy_mode_private_t* priv = calloc(1, sizeof(copy_mode_private_t));
    if (!priv) {
        free(backend);
        return NULL;
    }
    
    // Initialize backend
    backend->vtable = &copy_mode_vtable;
    backend->priv = priv;
    backend->key_mode = mode;
    backend->view_height = 24;  // Default
    backend->view_width = 80;   // Default
    
    // Initialize private data
    priv->word_separators = strdup(" \t\n.,;:!?()[]{}\"'`");
    priv->highlight_capacity = 1024;
    priv->highlight_cells = malloc(priv->highlight_capacity * sizeof(uint32_t));
    
    // Initialize clipboard
    clipboard_init();
    
    return backend;
}

void copy_mode_cleanup(copy_mode_backend_t* backend) {
    if (!backend) {
        return;
    }
    
    copy_mode_private_t* priv = (copy_mode_private_t*)backend->priv;
    
    // Cleanup private data
    if (priv) {
        if (priv->word_separators) {
            free(priv->word_separators);
        }
        if (priv->selection_cache) {
            free(priv->selection_cache);
        }
        if (priv->highlight_cells) {
            free(priv->highlight_cells);
        }
        for (int i = 0; i < priv->url_pattern_count; i++) {
            if (priv->url_patterns[i]) {
                free(priv->url_patterns[i]);
            }
        }
        free(priv);
    }
    
    // Cleanup buffers
    for (int i = 0; i < 10; i++) {
        if (backend->buffers[i]) {
            free(backend->buffers[i]->data);
            free(backend->buffers[i]);
        }
    }
    
    // Cleanup search pattern
    if (backend->search_pattern) {
        free(backend->search_pattern);
    }
    
    clipboard_cleanup();
    free(backend);
}

int copy_mode_resize(copy_mode_backend_t* backend, uint32_t rows, uint32_t cols) {
    backend->view_height = rows;
    backend->view_width = cols;
    
    // Clamp cursor position
    if (backend->cursor_row >= rows) {
        backend->cursor_row = rows - 1;
    }
    if (backend->cursor_col >= cols) {
        backend->cursor_col = cols - 1;
    }
    
    return 0;
}

char* copy_mode_get_selection(copy_mode_backend_t* backend, size_t* size) {
    copy_mode_private_t* priv = (copy_mode_private_t*)backend->priv;
    
    if (!backend->selection.active || !priv->grid_callbacks || !priv->grid) {
        *size = 0;
        return NULL;
    }
    
    // Return cached if valid
    if (priv->cache_valid && priv->selection_cache) {
        *size = priv->cache_size;
        return strdup(priv->selection_cache);
    }
    
    // Calculate selection bounds
    uint32_t start_row = backend->selection.start_row;
    uint32_t end_row = backend->selection.end_row;
    uint32_t start_col = backend->selection.start_col;
    uint32_t end_col = backend->selection.end_col;
    
    // Normalize selection
    if (start_row > end_row || (start_row == end_row && start_col > end_col)) {
        uint32_t tmp = start_row;
        start_row = end_row;
        end_row = tmp;
        tmp = start_col;
        start_col = end_col;
        end_col = tmp;
    }
    
    // Calculate buffer size needed
    size_t buffer_size = 0;
    for (uint32_t row = start_row; row <= end_row; row++) {
        size_t line_len;
        char* line = priv->grid_callbacks->get_line(priv->grid, row, &line_len);
        if (line) {
            buffer_size += line_len + 1; // +1 for newline
            free(line);
        }
    }
    
    // Allocate buffer
    char* result = malloc(buffer_size + 1);
    if (!result) {
        *size = 0;
        return NULL;
    }
    
    // Copy selection text
    size_t pos = 0;
    for (uint32_t row = start_row; row <= end_row; row++) {
        size_t line_len;
        char* line = priv->grid_callbacks->get_line(priv->grid, row, &line_len);
        if (!line) continue;
        
        uint32_t col_start = (row == start_row) ? start_col : 0;
        uint32_t col_end = (row == end_row) ? end_col : line_len;
        
        if (backend->selection.mode == SEL_MODE_RECT) {
            // Rectangular selection
            col_start = start_col;
            col_end = end_col;
            if (col_end > line_len) {
                col_end = line_len;
            }
        }
        
        size_t copy_len = col_end - col_start;
        if (copy_len > 0) {
            memcpy(result + pos, line + col_start, copy_len);
            pos += copy_len;
        }
        
        if (row < end_row) {
            result[pos++] = '\n';
        }
        
        free(line);
    }
    
    result[pos] = '\0';
    *size = pos;
    
    // Cache the result
    invalidate_cache(priv);
    priv->selection_cache = strdup(result);
    priv->cache_size = pos;
    priv->cache_valid = true;
    
    return result;
}

bool copy_mode_in_selection(copy_mode_backend_t* backend, uint32_t row, uint32_t col) {
    if (!backend->selection.active) {
        return false;
    }
    
    uint32_t start_row = backend->selection.start_row;
    uint32_t end_row = backend->selection.end_row;
    uint32_t start_col = backend->selection.start_col;
    uint32_t end_col = backend->selection.end_col;
    
    // Normalize selection
    if (start_row > end_row || (start_row == end_row && start_col > end_col)) {
        uint32_t tmp = start_row;
        start_row = end_row;
        end_row = tmp;
        tmp = start_col;
        start_col = end_col;
        end_col = tmp;
    }
    
    if (row < start_row || row > end_row) {
        return false;
    }
    
    if (backend->selection.mode == SEL_MODE_RECT) {
        // Rectangular selection
        return col >= start_col && col <= end_col;
    } else {
        // Stream selection
        if (row == start_row && col < start_col) {
            return false;
        }
        if (row == end_row && col > end_col) {
            return false;
        }
        return true;
    }
}

void copy_mode_set_grid_callbacks(copy_mode_backend_t* backend, 
                                 grid_callbacks_t* callbacks, void* grid) {
    copy_mode_private_t* priv = (copy_mode_private_t*)backend->priv;
    priv->grid_callbacks = callbacks;
    priv->grid = grid;
}