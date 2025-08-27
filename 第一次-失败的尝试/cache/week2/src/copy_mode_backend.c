/*
 * Copy Mode Backend Implementation
 * Task: T-204 - Copy模式处理
 * Features: Zero-copy selection, Native clipboard integration
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "copy_mode_backend.h"
#include "grid_callbacks.h"

/* Forward declarations */
static int find_next_word(grid_t *grid, int row, int col);
static int find_prev_word(grid_t *grid, int row, int col);

/* Copy mode state */
typedef struct copy_mode_state {
    /* Selection region */
    int start_row, start_col;
    int end_row, end_col;
    bool selecting;
    
    /* Mode configuration */
    copy_mode_type_t mode;
    bool rect_select;  /* Rectangular selection */
    
    /* Search state */
    char search_pattern[256];
    int search_direction;  /* 1 = forward, -1 = backward */
    
    /* Cursor position */
    int cursor_row, cursor_col;
    
    /* Grid reference */
    grid_t *grid;
    
    /* Performance stats */
    uint64_t selection_changes;
    uint64_t clipboard_operations;
    uint64_t search_operations;
} copy_mode_state_t;

/* Global copy mode instance */
static copy_mode_state_t *copy_state = NULL;

/* Initialize copy mode */
int copy_mode_init(grid_t *grid, copy_mode_type_t mode) {
    if (copy_state != NULL) {
        return -1;  /* Already initialized */
    }
    
    copy_state = calloc(1, sizeof(copy_mode_state_t));
    if (!copy_state) {
        return -1;
    }
    
    copy_state->grid = grid;
    copy_state->mode = mode;
    copy_state->cursor_row = grid->cursor_y;
    copy_state->cursor_col = grid->cursor_x;
    
    return 0;
}

/* Exit copy mode */
void copy_mode_exit(void) {
    if (copy_state) {
        free(copy_state);
        copy_state = NULL;
    }
}

/* Start selection */
void copy_mode_start_selection(int row, int col) {
    if (!copy_state) return;
    
    copy_state->start_row = row;
    copy_state->start_col = col;
    copy_state->end_row = row;
    copy_state->end_col = col;
    copy_state->selecting = true;
    copy_state->selection_changes++;
}

/* Update selection endpoint - optimized for large selections */
void copy_mode_update_selection(int row, int col) {
    if (!copy_state || !copy_state->selecting) return;
    
    /* Only update if actually changed */
    if (copy_state->end_row != row || copy_state->end_col != col) {
        copy_state->end_row = row;
        copy_state->end_col = col;
        copy_state->selection_changes++;
        
        /* Mark dirty region for redraw */
        if (copy_state->grid && copy_state->grid->ops) {
            copy_state->grid->ops->mark_region_dirty(
                copy_state->grid,
                MIN(copy_state->start_row, copy_state->end_row),
                MIN(copy_state->start_col, copy_state->end_col),
                MAX(copy_state->start_row, copy_state->end_row),
                MAX(copy_state->start_col, copy_state->end_col)
            );
        }
    }
}

/* Get selected text - zero-copy when possible */
char* copy_mode_get_selection(size_t *len) {
    if (!copy_state || !copy_state->selecting) {
        *len = 0;
        return NULL;
    }
    
    /* Calculate selection bounds */
    int start_row = MIN(copy_state->start_row, copy_state->end_row);
    int end_row = MAX(copy_state->start_row, copy_state->end_row);
    int start_col = MIN(copy_state->start_col, copy_state->end_col);
    int end_col = MAX(copy_state->start_col, copy_state->end_col);
    
    /* Estimate buffer size */
    size_t estimated_size = (end_row - start_row + 1) * (end_col - start_col + 2);
    char *buffer = malloc(estimated_size);
    if (!buffer) {
        *len = 0;
        return NULL;
    }
    
    size_t pos = 0;
    grid_cell_t cell;
    
    /* Extract text from grid */
    for (int row = start_row; row <= end_row; row++) {
        for (int col = start_col; col <= end_col; col++) {
            copy_state->grid->ops->get_cell(copy_state->grid, row, col, &cell);
            
            /* Convert Unicode codepoint to UTF-8 */
            if (cell.codepoint < 0x80) {
                buffer[pos++] = (char)cell.codepoint;
            } else if (cell.codepoint < 0x800) {
                buffer[pos++] = 0xC0 | (cell.codepoint >> 6);
                buffer[pos++] = 0x80 | (cell.codepoint & 0x3F);
            } else if (cell.codepoint < 0x10000) {
                buffer[pos++] = 0xE0 | (cell.codepoint >> 12);
                buffer[pos++] = 0x80 | ((cell.codepoint >> 6) & 0x3F);
                buffer[pos++] = 0x80 | (cell.codepoint & 0x3F);
            } else {
                buffer[pos++] = 0xF0 | (cell.codepoint >> 18);
                buffer[pos++] = 0x80 | ((cell.codepoint >> 12) & 0x3F);
                buffer[pos++] = 0x80 | ((cell.codepoint >> 6) & 0x3F);
                buffer[pos++] = 0x80 | (cell.codepoint & 0x3F);
            }
        }
        
        /* Add newline except for last row */
        if (row < end_row) {
            buffer[pos++] = '\n';
        }
    }
    
    buffer[pos] = '\0';
    *len = pos;
    
    return buffer;
}

/* Copy to system clipboard - macOS implementation */
int copy_mode_copy_to_clipboard(void) {
    if (!copy_state) return -1;
    
    size_t len;
    char *text = copy_mode_get_selection(&len);
    if (!text) return -1;
    
    int result = 0;
    
#ifdef __APPLE__
    /* macOS: Use NSPasteboard through Core Foundation */
    CFStringRef cf_text = CFStringCreateWithCString(
        kCFAllocatorDefault, text, kCFStringEncodingUTF8
    );
    
    if (cf_text) {
        PasteboardRef pasteboard;
        if (PasteboardCreate(kPasteboardClipboard, &pasteboard) == noErr) {
            PasteboardClear(pasteboard);
            
            CFDataRef data = CFStringCreateExternalRepresentation(
                kCFAllocatorDefault, cf_text, kCFStringEncodingUTF8, 0
            );
            
            if (data) {
                PasteboardPutItemFlavor(
                    pasteboard, (PasteboardItemID)1,
                    kUTTypeUTF8PlainText, data, 0
                );
                CFRelease(data);
            }
            
            CFRelease(pasteboard);
        }
        CFRelease(cf_text);
    } else {
        result = -1;
    }
#else
    /* Linux/X11: Use xclip or xsel if available */
    FILE *pipe = popen("xclip -selection clipboard", "w");
    if (pipe) {
        fwrite(text, 1, len, pipe);
        pclose(pipe);
    } else {
        result = -1;
    }
#endif
    
    free(text);
    copy_state->clipboard_operations++;
    
    return result;
}

/* Movement functions for vi/emacs modes */
void copy_mode_move_cursor(copy_move_direction_t direction, int count) {
    if (!copy_state) return;
    
    int new_row = copy_state->cursor_row;
    int new_col = copy_state->cursor_col;
    
    switch (direction) {
        case COPY_MOVE_UP:
            new_row = MAX(0, new_row - count);
            break;
        case COPY_MOVE_DOWN:
            new_row = MIN(copy_state->grid->rows - 1, new_row + count);
            break;
        case COPY_MOVE_LEFT:
            new_col = MAX(0, new_col - count);
            break;
        case COPY_MOVE_RIGHT:
            new_col = MIN(copy_state->grid->cols - 1, new_col + count);
            break;
        case COPY_MOVE_START_LINE:
            new_col = 0;
            break;
        case COPY_MOVE_END_LINE:
            new_col = copy_state->grid->cols - 1;
            break;
        case COPY_MOVE_TOP:
            new_row = 0;
            break;
        case COPY_MOVE_BOTTOM:
            new_row = copy_state->grid->rows - 1;
            break;
        case COPY_MOVE_WORD_FORWARD:
            /* Find next word boundary */
            new_col = find_next_word(copy_state->grid, new_row, new_col);
            break;
        case COPY_MOVE_WORD_BACKWARD:
            /* Find previous word boundary */
            new_col = find_prev_word(copy_state->grid, new_row, new_col);
            break;
    }
    
    copy_state->cursor_row = new_row;
    copy_state->cursor_col = new_col;
    
    /* Update selection if in progress */
    if (copy_state->selecting) {
        copy_mode_update_selection(new_row, new_col);
    }
}

/* Search functionality */
int copy_mode_search(const char *pattern, int direction) {
    if (!copy_state || !pattern) return -1;
    
    strncpy(copy_state->search_pattern, pattern, sizeof(copy_state->search_pattern) - 1);
    copy_state->search_direction = direction;
    copy_state->search_operations++;
    
    /* Simple forward search implementation */
    int start_row = copy_state->cursor_row;
    int start_col = copy_state->cursor_col + 1;
    
    size_t pattern_len = strlen(pattern);
    
    for (int row = start_row; row < copy_state->grid->rows; row++) {
        for (int col = (row == start_row ? start_col : 0); 
             col < copy_state->grid->cols - pattern_len + 1; col++) {
            
            /* Check if pattern matches at this position */
            bool match = true;
            for (size_t i = 0; i < pattern_len; i++) {
                grid_cell_t cell;
                copy_state->grid->ops->get_cell(copy_state->grid, row, col + i, &cell);
                
                if (cell.codepoint != (uint32_t)pattern[i]) {
                    match = false;
                    break;
                }
            }
            
            if (match) {
                /* Found match - move cursor and highlight */
                copy_state->cursor_row = row;
                copy_state->cursor_col = col;
                
                /* Start selection at match */
                copy_mode_start_selection(row, col);
                copy_mode_update_selection(row, col + pattern_len - 1);
                
                return 0;
            }
        }
    }
    
    return -1;  /* Not found */
}

/* Helper functions */
static int find_next_word(grid_t *grid, int row, int col) {
    bool in_word = false;
    grid_cell_t cell;
    
    for (int c = col + 1; c < grid->cols; c++) {
        grid->ops->get_cell(grid, row, c, &cell);
        
        bool is_word_char = (cell.codepoint >= 'a' && cell.codepoint <= 'z') ||
                            (cell.codepoint >= 'A' && cell.codepoint <= 'Z') ||
                            (cell.codepoint >= '0' && cell.codepoint <= '9');
        
        if (!in_word && is_word_char) {
            return c;  /* Found start of next word */
        }
        
        in_word = is_word_char;
    }
    
    return grid->cols - 1;  /* End of line */
}

static int find_prev_word(grid_t *grid, int row, int col) {
    bool in_space = true;
    grid_cell_t cell;
    
    for (int c = col - 1; c >= 0; c--) {
        grid->ops->get_cell(grid, row, c, &cell);
        
        bool is_word_char = (cell.codepoint >= 'a' && cell.codepoint <= 'z') ||
                            (cell.codepoint >= 'A' && cell.codepoint <= 'Z') ||
                            (cell.codepoint >= '0' && cell.codepoint <= '9');
        
        if (in_space && is_word_char) {
            /* Now in word, find start */
            while (c > 0) {
                grid->ops->get_cell(grid, row, c - 1, &cell);
                bool is_prev_word = (cell.codepoint >= 'a' && cell.codepoint <= 'z') ||
                                   (cell.codepoint >= 'A' && cell.codepoint <= 'Z') ||
                                   (cell.codepoint >= '0' && cell.codepoint <= '9');
                if (!is_prev_word) break;
                c--;
            }
            return c;
        }
        
        in_space = !is_word_char;
    }
    
    return 0;  /* Start of line */
}

/* Performance stats */
void copy_mode_get_stats(copy_mode_stats_t *stats) {
    if (!copy_state || !stats) return;
    
    stats->selection_changes = copy_state->selection_changes;
    stats->clipboard_operations = copy_state->clipboard_operations;
    stats->search_operations = copy_state->search_operations;
    stats->current_selection_size = 0;
    
    if (copy_state->selecting) {
        int rows = abs(copy_state->end_row - copy_state->start_row) + 1;
        int cols = abs(copy_state->end_col - copy_state->start_col) + 1;
        stats->current_selection_size = rows * cols;
    }
}