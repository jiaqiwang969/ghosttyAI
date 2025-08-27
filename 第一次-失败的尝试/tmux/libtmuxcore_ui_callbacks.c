/* libtmuxcore_ui_callbacks.c - UI Backend callback implementations */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui_backend/ui_backend.h"
#include "libtmuxcore_api.h"

/* External function declaration */
void ui_backend_register(struct ui_backend_vtable *vtable);

/* Define minimal grid_cell structure for compilation */
struct grid_cell {
    struct {
        size_t size;
        char data[8];
    } data;
    int fg;
    int bg;
    int attr;
};

/* Define minimal tty_ctx structure for compilation */
struct tty_ctx {
    u_int ocx;
    u_int ocy;
    const struct grid_cell *cell;
};

/* Grid buffer for rendering */
#define MAX_COLS 256
#define MAX_ROWS 100

typedef struct {
    char ch;
    int fg;
    int bg;
    int attrs;
} cell_t;

typedef struct {
    cell_t cells[MAX_ROWS][MAX_COLS];
    int cursor_x;
    int cursor_y;
    int cols;
    int rows;
    int dirty;
} grid_buffer_t;

/* Global grid buffer */
static grid_buffer_t g_grid = {
    .cols = 80,
    .rows = 24,
    .cursor_x = 0,
    .cursor_y = 0,
    .dirty = 0
};

/* Callback context */
typedef struct {
    void *user_data;
    void (*on_update)(void *user_data);
} callback_context_t;

static callback_context_t g_callback_ctx = {0};

/* Initialize grid */
void ui_grid_init(int cols, int rows) {
    g_grid.cols = cols > MAX_COLS ? MAX_COLS : cols;
    g_grid.rows = rows > MAX_ROWS ? MAX_ROWS : rows;
    
    /* Clear grid */
    for (int y = 0; y < g_grid.rows; y++) {
        for (int x = 0; x < g_grid.cols; x++) {
            g_grid.cells[y][x].ch = ' ';
            g_grid.cells[y][x].fg = 7;  /* Default white */
            g_grid.cells[y][x].bg = 0;  /* Default black */
            g_grid.cells[y][x].attrs = 0;
        }
    }
    
    printf("[UI] Grid initialized: %dx%d\n", g_grid.cols, g_grid.rows);
}

/* Write a cell to the grid */
static void ui_write_cell(u_int x, u_int y, const struct grid_cell *gc) {
    if (x >= g_grid.cols || y >= g_grid.rows) {
        return;
    }
    
    if (gc && gc->data.size > 0) {
        g_grid.cells[y][x].ch = gc->data.data[0];
        g_grid.cells[y][x].fg = gc->fg;
        g_grid.cells[y][x].bg = gc->bg;
        g_grid.cells[y][x].attrs = gc->attr;
        g_grid.dirty = 1;
    }
}

/* Move cursor */
static void ui_move_cursor(u_int x, u_int y) {
    if (x < g_grid.cols && y < g_grid.rows) {
        g_grid.cursor_x = x;
        g_grid.cursor_y = y;
        g_grid.dirty = 1;
    }
}

/* Show/hide cursor */
static void ui_show_cursor(int visible) {
    /* TODO: Implement cursor visibility */
    printf("[UI] Cursor %s\n", visible ? "shown" : "hidden");
}

/* Clear entire screen */
static void ui_clear_screen(void) {
    for (int y = 0; y < g_grid.rows; y++) {
        for (int x = 0; x < g_grid.cols; x++) {
            g_grid.cells[y][x].ch = ' ';
            g_grid.cells[y][x].attrs = 0;
        }
    }
    g_grid.cursor_x = 0;
    g_grid.cursor_y = 0;
    g_grid.dirty = 1;
    
    printf("[UI] Screen cleared\n");
}

/* Clear a line */
static void ui_clear_line(u_int y) {
    if (y >= g_grid.rows) {
        return;
    }
    
    for (int x = 0; x < g_grid.cols; x++) {
        g_grid.cells[y][x].ch = ' ';
        g_grid.cells[y][x].attrs = 0;
    }
    g_grid.dirty = 1;
}

/* Scroll region */
static void ui_scroll_region(u_int top, u_int bottom, int lines) {
    if (top >= g_grid.rows || bottom >= g_grid.rows || top > bottom) {
        return;
    }
    
    if (lines > 0) {
        /* Scroll up */
        for (int i = 0; i < lines && top <= bottom; i++) {
            /* Save top line */
            cell_t temp[MAX_COLS];
            memcpy(temp, g_grid.cells[top], sizeof(cell_t) * g_grid.cols);
            
            /* Move lines up */
            for (u_int y = top; y < bottom; y++) {
                memcpy(g_grid.cells[y], g_grid.cells[y + 1], sizeof(cell_t) * g_grid.cols);
            }
            
            /* Clear bottom line */
            for (int x = 0; x < g_grid.cols; x++) {
                g_grid.cells[bottom][x].ch = ' ';
                g_grid.cells[bottom][x].attrs = 0;
            }
        }
    } else if (lines < 0) {
        /* Scroll down */
        lines = -lines;
        for (int i = 0; i < lines && top <= bottom; i++) {
            /* Save bottom line */
            cell_t temp[MAX_COLS];
            memcpy(temp, g_grid.cells[bottom], sizeof(cell_t) * g_grid.cols);
            
            /* Move lines down */
            for (u_int y = bottom; y > top; y--) {
                memcpy(g_grid.cells[y], g_grid.cells[y - 1], sizeof(cell_t) * g_grid.cols);
            }
            
            /* Clear top line */
            for (int x = 0; x < g_grid.cols; x++) {
                g_grid.cells[top][x].ch = ' ';
                g_grid.cells[top][x].attrs = 0;
            }
        }
    }
    
    g_grid.dirty = 1;
}

/* Handle structured output from tmux */
static void ui_handle_output(const struct tty_ctx *ctx) {
    if (!ctx) return;
    
    /* Extract relevant information from context */
    if (ctx->cell) {
        ui_write_cell(ctx->ocx, ctx->ocy, ctx->cell);
    }
    
    /* Trigger update if we have a callback */
    if (g_grid.dirty && g_callback_ctx.on_update && g_callback_ctx.user_data) {
        g_callback_ctx.on_update(g_callback_ctx.user_data);
        g_grid.dirty = 0;
    }
}

/* Pane management callbacks */
static void ui_split_pane(int horizontal, u_int size) {
    printf("[UI] Pane split %s at %u%%\n", 
           horizontal ? "horizontally" : "vertically", size);
}

static void ui_resize_pane(u_int id, u_int width, u_int height) {
    printf("[UI] Pane %u resized to %ux%u\n", id, width, height);
}

static void ui_close_pane(u_int id) {
    printf("[UI] Pane %u closed\n", id);
}

/* Session management callbacks */
static void ui_new_session(const char *name) {
    printf("[UI] New session created: %s\n", name);
}

static void ui_attach_session(u_int id) {
    printf("[UI] Attached to session %u\n", id);
}

static void ui_detach_session(void) {
    printf("[UI] Detached from session\n");
}

/* Create and register the UI backend vtable */
void ui_callbacks_register(void *user_data, void (*on_update)(void *)) {
    static struct ui_backend_vtable vtable = {
        .handle_output = ui_handle_output,
        .write_cell = ui_write_cell,
        .move_cursor = ui_move_cursor,
        .show_cursor = ui_show_cursor,
        .clear_screen = ui_clear_screen,
        .clear_line = ui_clear_line,
        .scroll_region = ui_scroll_region,
        .split_pane = ui_split_pane,
        .resize_pane = ui_resize_pane,
        .close_pane = ui_close_pane,
        .new_session = ui_new_session,
        .attach_session = ui_attach_session,
        .detach_session = ui_detach_session
    };
    
    /* Save callback context */
    g_callback_ctx.user_data = user_data;
    g_callback_ctx.on_update = on_update;
    
    /* Register with tmux backend */
    ui_backend_register(&vtable);
    
    printf("[UI] Callbacks registered successfully\n");
}

/* Get current grid state (for rendering) */
void ui_get_grid_cell(int x, int y, char *ch, int *fg, int *bg, int *attrs) {
    if (x >= 0 && x < g_grid.cols && y >= 0 && y < g_grid.rows) {
        *ch = g_grid.cells[y][x].ch;
        *fg = g_grid.cells[y][x].fg;
        *bg = g_grid.cells[y][x].bg;
        *attrs = g_grid.cells[y][x].attrs;
    } else {
        *ch = ' ';
        *fg = 7;
        *bg = 0;
        *attrs = 0;
    }
}

/* Get cursor position */
void ui_get_cursor_pos(int *x, int *y) {
    *x = g_grid.cursor_x;
    *y = g_grid.cursor_y;
}

/* Debug: Print grid contents */
void ui_debug_print_grid(void) {
    printf("\n=== Grid Contents (%dx%d) ===\n", g_grid.cols, g_grid.rows);
    for (int y = 0; y < g_grid.rows && y < 10; y++) {  /* Print first 10 lines */
        printf("%2d: ", y);
        for (int x = 0; x < g_grid.cols && x < 40; x++) {  /* Print first 40 cols */
            char ch = g_grid.cells[y][x].ch;
            if (ch >= 32 && ch < 127) {
                putchar(ch);
            } else {
                putchar('.');
            }
        }
        printf("\n");
    }
    printf("Cursor at: (%d, %d)\n", g_grid.cursor_x, g_grid.cursor_y);
}