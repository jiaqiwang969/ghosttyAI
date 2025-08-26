// selection_renderer.c - Visual selection highlighting for copy mode
// Purpose: Render visual selection with dirty tracking optimization
// Author: INTG-002 (integration-dev)  
// Date: 2025-08-26
// Task: T-204 - Selection rendering with scrollback support
// Performance: <10ms selection update latency

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "../include/copy_mode_callbacks.h"

// ============================================================================
// Rendering structures
// ============================================================================

typedef struct {
    uint32_t row;
    uint32_t col;
    uint8_t style;  // Highlight style (inverse, underline, etc.)
} highlight_cell_t;

typedef struct {
    // Dirty tracking
    uint32_t* dirty_rows;
    size_t dirty_count;
    size_t dirty_capacity;
    bool full_redraw;
    
    // Highlight buffer
    highlight_cell_t* highlights;
    size_t highlight_count;
    size_t highlight_capacity;
    
    // Scrollback state
    uint32_t scrollback_offset;
    uint32_t visible_rows;
    uint32_t visible_cols;
    
    // Performance metrics
    uint64_t last_render_ns;
    uint64_t total_render_time_ns;
    uint32_t render_count;
    
    // Selection colors
    uint32_t selection_fg;
    uint32_t selection_bg;
    uint8_t selection_attr;
    
} selection_renderer_t;

// Highlight styles
enum {
    HIGHLIGHT_INVERSE = 0x01,
    HIGHLIGHT_UNDERLINE = 0x02,
    HIGHLIGHT_BOLD = 0x04,
    HIGHLIGHT_DIM = 0x08,
    HIGHLIGHT_BLINK = 0x10,
};

// ============================================================================
// Helper functions
// ============================================================================

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static void mark_row_dirty(selection_renderer_t* renderer, uint32_t row) {
    // Check if already marked
    for (size_t i = 0; i < renderer->dirty_count; i++) {
        if (renderer->dirty_rows[i] == row) {
            return;
        }
    }
    
    // Expand capacity if needed
    if (renderer->dirty_count >= renderer->dirty_capacity) {
        renderer->dirty_capacity *= 2;
        renderer->dirty_rows = realloc(renderer->dirty_rows,
                                      renderer->dirty_capacity * sizeof(uint32_t));
    }
    
    renderer->dirty_rows[renderer->dirty_count++] = row;
}

static void clear_dirty(selection_renderer_t* renderer) {
    renderer->dirty_count = 0;
    renderer->full_redraw = false;
}

// ============================================================================
// Selection rendering
// ============================================================================

selection_renderer_t* selection_renderer_init(void) {
    selection_renderer_t* renderer = calloc(1, sizeof(selection_renderer_t));
    if (!renderer) {
        return NULL;
    }
    
    // Initialize dirty tracking
    renderer->dirty_capacity = 32;
    renderer->dirty_rows = malloc(renderer->dirty_capacity * sizeof(uint32_t));
    
    // Initialize highlight buffer
    renderer->highlight_capacity = 1024;
    renderer->highlights = malloc(renderer->highlight_capacity * sizeof(highlight_cell_t));
    
    // Default colors
    renderer->selection_fg = 0xFFFFFF; // White
    renderer->selection_bg = 0x0080FF; // Blue
    renderer->selection_attr = HIGHLIGHT_INVERSE;
    
    // Default dimensions
    renderer->visible_rows = 24;
    renderer->visible_cols = 80;
    
    return renderer;
}

void selection_renderer_cleanup(selection_renderer_t* renderer) {
    if (!renderer) {
        return;
    }
    
    free(renderer->dirty_rows);
    free(renderer->highlights);
    free(renderer);
}

int selection_renderer_update(selection_renderer_t* renderer,
                             copy_mode_backend_t* backend) {
    if (!renderer || !backend) {
        return -1;
    }
    
    uint64_t start_ns = get_time_ns();
    
    // Clear previous highlights
    size_t old_highlight_count = renderer->highlight_count;
    renderer->highlight_count = 0;
    
    // Check if selection is active
    if (!backend->selection.active) {
        // Mark previously highlighted rows as dirty
        for (size_t i = 0; i < old_highlight_count; i++) {
            mark_row_dirty(renderer, renderer->highlights[i].row);
        }
        goto done;
    }
    
    // Get selection bounds
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
    
    // Account for scrollback
    start_row += renderer->scrollback_offset;
    end_row += renderer->scrollback_offset;
    
    // Clip to visible area
    uint32_t visible_start = renderer->scrollback_offset;
    uint32_t visible_end = visible_start + renderer->visible_rows;
    
    if (end_row < visible_start || start_row >= visible_end) {
        // Selection not visible
        goto done;
    }
    
    if (start_row < visible_start) {
        start_row = visible_start;
        start_col = 0;
    }
    if (end_row >= visible_end) {
        end_row = visible_end - 1;
        end_col = renderer->visible_cols - 1;
    }
    
    // Calculate highlight cells based on selection mode
    switch (backend->selection.mode) {
        case SEL_MODE_CHAR:
        case SEL_MODE_WORD:
        case SEL_MODE_URL: {
            // Stream selection
            for (uint32_t row = start_row; row <= end_row; row++) {
                uint32_t col_start = (row == start_row) ? start_col : 0;
                uint32_t col_end = (row == end_row) ? end_col : renderer->visible_cols - 1;
                
                for (uint32_t col = col_start; col <= col_end; col++) {
                    if (renderer->highlight_count >= renderer->highlight_capacity) {
                        renderer->highlight_capacity *= 2;
                        renderer->highlights = realloc(renderer->highlights,
                            renderer->highlight_capacity * sizeof(highlight_cell_t));
                    }
                    
                    renderer->highlights[renderer->highlight_count++] = (highlight_cell_t){
                        .row = row - renderer->scrollback_offset,
                        .col = col,
                        .style = renderer->selection_attr
                    };
                }
                
                mark_row_dirty(renderer, row - renderer->scrollback_offset);
            }
            break;
        }
        
        case SEL_MODE_LINE: {
            // Line selection - full lines
            for (uint32_t row = start_row; row <= end_row; row++) {
                for (uint32_t col = 0; col < renderer->visible_cols; col++) {
                    if (renderer->highlight_count >= renderer->highlight_capacity) {
                        renderer->highlight_capacity *= 2;
                        renderer->highlights = realloc(renderer->highlights,
                            renderer->highlight_capacity * sizeof(highlight_cell_t));
                    }
                    
                    renderer->highlights[renderer->highlight_count++] = (highlight_cell_t){
                        .row = row - renderer->scrollback_offset,
                        .col = col,
                        .style = renderer->selection_attr
                    };
                }
                
                mark_row_dirty(renderer, row - renderer->scrollback_offset);
            }
            break;
        }
        
        case SEL_MODE_RECT: {
            // Rectangular selection
            for (uint32_t row = start_row; row <= end_row; row++) {
                for (uint32_t col = start_col; col <= end_col && col < renderer->visible_cols; col++) {
                    if (renderer->highlight_count >= renderer->highlight_capacity) {
                        renderer->highlight_capacity *= 2;
                        renderer->highlights = realloc(renderer->highlights,
                            renderer->highlight_capacity * sizeof(highlight_cell_t));
                    }
                    
                    renderer->highlights[renderer->highlight_count++] = (highlight_cell_t){
                        .row = row - renderer->scrollback_offset,
                        .col = col,
                        .style = renderer->selection_attr
                    };
                }
                
                mark_row_dirty(renderer, row - renderer->scrollback_offset);
            }
            break;
        }
    }
    
    // Mark old highlight positions as dirty if they're not in new selection
    for (size_t i = 0; i < old_highlight_count; i++) {
        bool found = false;
        for (size_t j = 0; j < renderer->highlight_count; j++) {
            if (renderer->highlights[j].row == renderer->highlights[i].row &&
                renderer->highlights[j].col == renderer->highlights[i].col) {
                found = true;
                break;
            }
        }
        if (!found) {
            mark_row_dirty(renderer, renderer->highlights[i].row);
        }
    }

done:
    {
        // Update performance metrics
        uint64_t elapsed_ns = get_time_ns() - start_ns;
        renderer->last_render_ns = elapsed_ns;
        renderer->total_render_time_ns += elapsed_ns;
        renderer->render_count++;
    }
    
    return 0;
}

int selection_renderer_scroll(selection_renderer_t* renderer,
                             int32_t delta) {
    if (!renderer) {
        return -1;
    }
    
    uint32_t old_offset = renderer->scrollback_offset;
    
    if (delta < 0 && (uint32_t)(-delta) > renderer->scrollback_offset) {
        renderer->scrollback_offset = 0;
    } else {
        renderer->scrollback_offset += delta;
    }
    
    if (renderer->scrollback_offset != old_offset) {
        renderer->full_redraw = true;
    }
    
    return 0;
}

int selection_renderer_resize(selection_renderer_t* renderer,
                             uint32_t rows, uint32_t cols) {
    if (!renderer) {
        return -1;
    }
    
    renderer->visible_rows = rows;
    renderer->visible_cols = cols;
    renderer->full_redraw = true;
    
    return 0;
}

bool selection_renderer_is_highlighted(selection_renderer_t* renderer,
                                      uint32_t row, uint32_t col) {
    if (!renderer) {
        return false;
    }
    
    for (size_t i = 0; i < renderer->highlight_count; i++) {
        if (renderer->highlights[i].row == row &&
            renderer->highlights[i].col == col) {
            return true;
        }
    }
    
    return false;
}

uint8_t selection_renderer_get_style(selection_renderer_t* renderer,
                                    uint32_t row, uint32_t col) {
    if (!renderer) {
        return 0;
    }
    
    for (size_t i = 0; i < renderer->highlight_count; i++) {
        if (renderer->highlights[i].row == row &&
            renderer->highlights[i].col == col) {
            return renderer->highlights[i].style;
        }
    }
    
    return 0;
}

// ============================================================================
// Optimized rendering functions
// ============================================================================

int selection_renderer_get_dirty_rows(selection_renderer_t* renderer,
                                     uint32_t** rows, size_t* count) {
    if (!renderer || !rows || !count) {
        return -1;
    }
    
    if (renderer->full_redraw) {
        // Return all visible rows
        *count = renderer->visible_rows;
        *rows = malloc(*count * sizeof(uint32_t));
        for (uint32_t i = 0; i < renderer->visible_rows; i++) {
            (*rows)[i] = i;
        }
    } else {
        // Return only dirty rows
        *count = renderer->dirty_count;
        if (*count > 0) {
            *rows = malloc(*count * sizeof(uint32_t));
            memcpy(*rows, renderer->dirty_rows, *count * sizeof(uint32_t));
        } else {
            *rows = NULL;
        }
    }
    
    return 0;
}

void selection_renderer_clear_dirty(selection_renderer_t* renderer) {
    if (renderer) {
        clear_dirty(renderer);
    }
}

// ============================================================================
// Cursor rendering
// ============================================================================

typedef struct {
    uint32_t row;
    uint32_t col;
    uint8_t style;
    bool visible;
    uint64_t blink_interval_ms;
    uint64_t last_blink_ms;
    bool blink_state;
} cursor_state_t;

static cursor_state_t cursor = {
    .style = HIGHLIGHT_INVERSE | HIGHLIGHT_BLINK,
    .visible = true,
    .blink_interval_ms = 500,
    .last_blink_ms = 0,
    .blink_state = true
};

void selection_renderer_update_cursor(selection_renderer_t* renderer,
                                     uint32_t row, uint32_t col) {
    if (!renderer) {
        return;
    }
    
    // Mark old and new cursor positions as dirty
    if (cursor.visible) {
        mark_row_dirty(renderer, cursor.row);
    }
    
    cursor.row = row;
    cursor.col = col;
    
    if (cursor.visible) {
        mark_row_dirty(renderer, cursor.row);
    }
}

void selection_renderer_cursor_blink(selection_renderer_t* renderer) {
    if (!renderer || !cursor.visible) {
        return;
    }
    
    uint64_t now_ms = get_time_ns() / 1000000;
    
    if (now_ms - cursor.last_blink_ms >= cursor.blink_interval_ms) {
        cursor.blink_state = !cursor.blink_state;
        cursor.last_blink_ms = now_ms;
        mark_row_dirty(renderer, cursor.row);
    }
}

bool selection_renderer_is_cursor(selection_renderer_t* renderer,
                                 uint32_t row, uint32_t col) {
    (void)renderer;
    return cursor.visible && cursor.blink_state && 
           cursor.row == row && cursor.col == col;
}

// ============================================================================
// Search match highlighting
// ============================================================================

typedef struct {
    uint32_t row;
    uint32_t col;
    uint32_t length;
} search_match_t;

typedef struct {
    search_match_t* matches;
    size_t match_count;
    size_t match_capacity;
    uint32_t current_match;
    uint8_t match_style;
    uint8_t current_match_style;
} search_highlights_t;

static search_highlights_t search = {
    .match_style = HIGHLIGHT_UNDERLINE,
    .current_match_style = HIGHLIGHT_INVERSE | HIGHLIGHT_BOLD
};

int selection_renderer_add_search_match(selection_renderer_t* renderer,
                                       uint32_t row, uint32_t col, uint32_t length) {
    if (!renderer) {
        return -1;
    }
    
    if (search.match_count >= search.match_capacity) {
        search.match_capacity = search.match_capacity ? search.match_capacity * 2 : 16;
        search.matches = realloc(search.matches,
                                search.match_capacity * sizeof(search_match_t));
    }
    
    search.matches[search.match_count++] = (search_match_t){
        .row = row,
        .col = col,
        .length = length
    };
    
    mark_row_dirty(renderer, row);
    
    return 0;
}

void selection_renderer_clear_search(selection_renderer_t* renderer) {
    if (!renderer) {
        return;
    }
    
    // Mark all match rows as dirty
    for (size_t i = 0; i < search.match_count; i++) {
        mark_row_dirty(renderer, search.matches[i].row);
    }
    
    search.match_count = 0;
    search.current_match = 0;
}

bool selection_renderer_is_search_match(selection_renderer_t* renderer,
                                       uint32_t row, uint32_t col,
                                       uint8_t* style) {
    (void)renderer;
    
    for (size_t i = 0; i < search.match_count; i++) {
        search_match_t* match = &search.matches[i];
        if (row == match->row && 
            col >= match->col && 
            col < match->col + match->length) {
            
            if (style) {
                *style = (i == search.current_match) ? 
                        search.current_match_style : search.match_style;
            }
            return true;
        }
    }
    
    return false;
}

// ============================================================================
// Performance statistics
// ============================================================================

void selection_renderer_get_stats(selection_renderer_t* renderer,
                                 uint32_t* avg_render_us,
                                 uint32_t* total_renders,
                                 uint32_t* dirty_rows) {
    if (!renderer) {
        return;
    }
    
    if (avg_render_us && renderer->render_count > 0) {
        *avg_render_us = (renderer->total_render_time_ns / renderer->render_count) / 1000;
    }
    
    if (total_renders) {
        *total_renders = renderer->render_count;
    }
    
    if (dirty_rows) {
        *dirty_rows = renderer->dirty_count;
    }
}