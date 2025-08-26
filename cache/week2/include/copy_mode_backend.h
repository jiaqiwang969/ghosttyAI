/*
 * Copy Mode Backend Header
 * Task: T-204 - Copy模式处理
 */

#ifndef COPY_MODE_BACKEND_H
#define COPY_MODE_BACKEND_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Copy mode types */
typedef enum {
    COPY_MODE_VI,
    COPY_MODE_EMACS
} copy_mode_type_t;

/* Movement directions */
typedef enum {
    COPY_MOVE_UP,
    COPY_MOVE_DOWN,
    COPY_MOVE_LEFT,
    COPY_MOVE_RIGHT,
    COPY_MOVE_START_LINE,
    COPY_MOVE_END_LINE,
    COPY_MOVE_TOP,
    COPY_MOVE_BOTTOM,
    COPY_MOVE_WORD_FORWARD,
    COPY_MOVE_WORD_BACKWARD,
    COPY_MOVE_PAGE_UP,
    COPY_MOVE_PAGE_DOWN
} copy_move_direction_t;

/* Forward declarations */
struct grid;

/* Copy mode operations */
int copy_mode_init(struct grid *grid, copy_mode_type_t mode);
void copy_mode_exit(void);

/* Selection management */
void copy_mode_start_selection(int row, int col);
void copy_mode_update_selection(int row, int col);
void copy_mode_clear_selection(void);
bool copy_mode_has_selection(void);

/* Get selected text - caller must free */
char* copy_mode_get_selection(size_t *len);

/* Clipboard operations */
int copy_mode_copy_to_clipboard(void);
int copy_mode_paste_from_clipboard(void);

/* Movement */
void copy_mode_move_cursor(copy_move_direction_t direction, int count);
void copy_mode_jump_to(int row, int col);

/* Search */
int copy_mode_search(const char *pattern, int direction);
int copy_mode_search_next(void);
int copy_mode_search_prev(void);

/* Rectangle selection */
void copy_mode_toggle_rect_select(void);
bool copy_mode_is_rect_select(void);

/* Performance stats */
typedef struct {
    uint64_t selection_changes;
    uint64_t clipboard_operations;
    uint64_t search_operations;
    size_t current_selection_size;
} copy_mode_stats_t;

void copy_mode_get_stats(copy_mode_stats_t *stats);

/* Helper macros */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#endif /* COPY_MODE_BACKEND_H */