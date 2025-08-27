// ui_backend_minimal.h - Minimal headers for UI Backend without full tmux dependencies
// Purpose: Provide necessary type definitions without requiring libevent
// Date: 2025-08-26

#ifndef UI_BACKEND_MINIMAL_H
#define UI_BACKEND_MINIMAL_H

#include <stddef.h>
#include <stdint.h>

// Basic types matching tmux definitions
typedef unsigned int u_int;
typedef unsigned char u_char;
typedef unsigned short u_short;

// UTF8 data structure
#define UTF8_SIZE 9
struct utf8_data {
    u_char data[UTF8_SIZE];
    u_char have;
    u_char size;
};

// Grid cell structure
struct grid_cell {
    struct utf8_data data;
    u_short attr;
    u_char flags;
    int fg;
    int bg;
    int us;
    u_int link;
};

// TTY context structure (simplified)
struct tty_ctx {
    struct screen *s;
    void *redraw_cb;
    void *set_client_cb;
    void *arg;
    
    const struct grid_cell *cell;
    int wrapped;
    
    u_int num;
    void *ptr;
    void *ptr2;
    
    int allow_invisible_panes;
    
    // Cursor positions
    u_int ocx;
    u_int ocy;
    
    u_int orupper;
    u_int orlower;
    
    // Region offsets and sizes
    u_int xoff;
    u_int yoff;
    u_int rxoff;
    u_int ryoff;
    u_int sx;
    u_int sy;
    
    u_int bg;
    
    struct grid_cell defaults;
    void *palette;
    
    int bigger;
    u_int wox;
    u_int woy;
    u_int wsx;
    u_int wsy;
    
    // UI Backend enhancement - command ID for reliable dispatch
    int ui_cmd_id;  // Set by screen-write.c before calling tty_write
};

// Minimal tty structure
struct tty {
    void *client;
    // Additional fields not needed for dispatch
};

// UI Backend types
typedef struct ui_backend {
    int enabled;
    void *router;
    const char *mode;
} ui_backend_t;

// Function declarations
int ui_backend_enabled(void);
int ui_backend_init(void);
void ui_backend_cleanup(void);
ui_backend_t* ui_backend_get_instance(void);
int ui_backend_dispatch(ui_backend_t* backend, 
                        void (*cmdfn)(struct tty *, const struct tty_ctx *),
                        struct tty_ctx* ctx);

#endif // UI_BACKEND_MINIMAL_H