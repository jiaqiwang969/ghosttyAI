// tmux_types.h - Type definitions for tmux structures
// Purpose: Define tmux structures needed by backend router
// Author: libtmux-core-developer
// Date: 2025-08-25

#ifndef TMUX_TYPES_H
#define TMUX_TYPES_H

#include <stdint.h>

// Forward declarations
struct tty;
struct grid_cell;
struct window_pane;

// TTY context structure (simplified from tmux)
struct tty_ctx {
    struct window_pane *wp;
    
    // Positioning
    uint32_t ocx;       // Original cursor X
    uint32_t ocy;       // Original cursor Y
    uint32_t orupper;   // Original region upper
    uint32_t orlower;   // Original region lower
    
    // Screen region
    uint32_t sx;        // Screen X
    uint32_t sy;        // Screen Y
    uint32_t ox;        // Offset X
    uint32_t oy;        // Offset Y
    
    // Cell data
    const struct grid_cell *cell;
    int wrapped;
    
    // Number of cells
    uint32_t num;
    
    // Pointers (can be NULL)
    void *ptr;
    const void *ptr2;
    
    // Flags
    int redraw_cb;
    int set_client_cb;
    
    // TTY pointer
    struct tty *tty;
};

// TTY structure (simplified)
struct tty {
    char *path;
    uint32_t sx;
    uint32_t sy;
    uint32_t cx;
    uint32_t cy;
    
    int fd;
    int log_fd;
    
    void *term;
    uint32_t flags;
    
    // Private data
    void *data;
};

// Grid cell structure (simplified)
struct grid_cell {
    uint32_t data;      // Character data
    uint16_t attr;      // Attributes
    uint8_t fg;         // Foreground color
    uint8_t bg;         // Background color
    uint8_t us;         // Underline style
    uint32_t link;      // Hyperlink
};

// Window pane structure (simplified)
struct window_pane {
    uint32_t id;
    uint32_t sx;
    uint32_t sy;
    uint32_t xoff;
    uint32_t yoff;
    
    // Private data
    void *data;
};

#endif /* TMUX_TYPES_H */