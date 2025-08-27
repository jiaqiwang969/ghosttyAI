#ifndef UI_BACKEND_H
#define UI_BACKEND_H

#include <stdint.h>

/* Forward declarations */
struct tty_ctx;
struct grid_cell;

/* Type definitions for compatibility */
typedef unsigned int u_int;

/* UI backend callback function table */
struct ui_backend_vtable {
    /* Basic output */
    void (*handle_output)(const struct tty_ctx *ctx);
    void (*write_cell)(u_int x, u_int y, const struct grid_cell *gc);
    
    /* Cursor control */
    void (*move_cursor)(u_int x, u_int y);
    void (*show_cursor)(int visible);
    
    /* Screen operations */
    void (*clear_screen)(void);
    void (*clear_line)(u_int y);
    void (*scroll_region)(u_int top, u_int bottom, int lines);
    
    /* Pane management */
    void (*split_pane)(int horizontal, u_int size);
    void (*resize_pane)(u_int id, u_int width, u_int height);
    void (*close_pane)(u_int id);
    
    /* Session management */
    void (*new_session)(const char *name);
    void (*attach_session)(u_int id);
    void (*detach_session)(void);
};

/* Global registration functions */
void ui_backend_register(struct ui_backend_vtable *vtable);
void ui_backend_unregister(void);

/* Helper functions */
int ui_backend_is_active(void);

/* Create default vtable for testing */
struct ui_backend_vtable *ui_backend_create_default(void);

/* Global vtable pointer (defined in ui_backend_router.c) */
extern struct ui_backend_vtable *ui_backend;

#endif /* UI_BACKEND_H */