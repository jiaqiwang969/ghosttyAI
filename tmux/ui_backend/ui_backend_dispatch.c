// ui_backend_dispatch.c - Enhanced UI Backend Dispatch Implementation
// Purpose: Provide real command recognition and callback invocation for tmux tty operations
// Date: 2025-08-26
// Task: W4-INC-001 - Week 4 Incremental Enhancement

#include "ui_backend_minimal.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// Command ID system to avoid unreliable function pointer comparison
typedef enum {
    TTY_CMD_UNKNOWN = 0,
    TTY_CMD_CELL,
    TTY_CMD_CLEARLINE,
    TTY_CMD_CLEARSCREEN,
    TTY_CMD_INSERTLINE,
    TTY_CMD_DELETELINE,
    TTY_CMD_CLEARENDOFLINE,
    TTY_CMD_REVERSEINDEX,
    TTY_CMD_LINEFEED,
    TTY_CMD_SCROLLUP,
    TTY_CMD_SCROLLDOWN,
    TTY_CMD_MAX
} tty_cmd_id_t;

// Forward declarations for tty_cmd functions
extern void tty_cmd_cell(struct tty *, const struct tty_ctx *);
extern void tty_cmd_clearline(struct tty *, const struct tty_ctx *);
extern void tty_cmd_clearscreen(struct tty *, const struct tty_ctx *);
extern void tty_cmd_insertline(struct tty *, const struct tty_ctx *);
extern void tty_cmd_deleteline(struct tty *, const struct tty_ctx *);
extern void tty_cmd_clearendofline(struct tty *, const struct tty_ctx *);
extern void tty_cmd_reverseindex(struct tty *, const struct tty_ctx *);
extern void tty_cmd_linefeed(struct tty *, const struct tty_ctx *);
extern void tty_cmd_scrollup(struct tty *, const struct tty_ctx *);
extern void tty_cmd_scrolldown(struct tty *, const struct tty_ctx *);

// Command identification helper - with debug output
static tty_cmd_id_t identify_command(void (*cmdfn)(struct tty *, const struct tty_ctx *)) {
    // Get addresses of the actual functions for comparison
    void* cell_addr = (void*)&tty_cmd_cell;
    void* clearline_addr = (void*)&tty_cmd_clearline;
    void* clearscreen_addr = (void*)&tty_cmd_clearscreen;
    
    fprintf(stderr, "[IDENTIFY] Comparing function pointer %p against:\n", cmdfn);
    fprintf(stderr, "[IDENTIFY]   tty_cmd_cell: %p\n", cell_addr);
    fprintf(stderr, "[IDENTIFY]   tty_cmd_clearline: %p\n", clearline_addr);
    fprintf(stderr, "[IDENTIFY]   tty_cmd_clearscreen: %p\n", clearscreen_addr);
    
    // Try function pointer comparison first
    if (cmdfn == tty_cmd_cell || cmdfn == cell_addr) return TTY_CMD_CELL;
    if (cmdfn == tty_cmd_clearline || cmdfn == clearline_addr) return TTY_CMD_CLEARLINE;
    if (cmdfn == tty_cmd_clearscreen || cmdfn == clearscreen_addr) return TTY_CMD_CLEARSCREEN;
    if (cmdfn == tty_cmd_insertline) return TTY_CMD_INSERTLINE;
    if (cmdfn == tty_cmd_deleteline) return TTY_CMD_DELETELINE;
    if (cmdfn == tty_cmd_clearendofline) return TTY_CMD_CLEARENDOFLINE;
    if (cmdfn == tty_cmd_reverseindex) return TTY_CMD_REVERSEINDEX;
    if (cmdfn == tty_cmd_linefeed) return TTY_CMD_LINEFEED;
    if (cmdfn == tty_cmd_scrollup) return TTY_CMD_SCROLLUP;
    if (cmdfn == tty_cmd_scrolldown) return TTY_CMD_SCROLLDOWN;
    
    return TTY_CMD_UNKNOWN;
}

// Callback function table - this is the key missing part from Week 3
typedef struct {
    void (*on_cell)(char ch, int row, int col, int attr, int fg, int bg, void* user_data);
    void (*on_clear_line)(int row, void* user_data);
    void (*on_clear_screen)(void* user_data);
    void (*on_insert_line)(int row, void* user_data);
    void (*on_delete_line)(int row, void* user_data);
    void (*on_clear_eol)(int row, int col, void* user_data);
    void (*on_reverse_index)(void* user_data);
    void (*on_line_feed)(void* user_data);
    void (*on_scroll_up)(int count, void* user_data);
    void (*on_scroll_down)(int count, void* user_data);
    void (*on_flush)(void* user_data);
} ui_callbacks_t;

// Global callback storage
static ui_callbacks_t g_callbacks = {0};
static void* g_user_data = NULL;
static int g_callbacks_registered = 0;

// Debug output helper
static void debug_log(const char* format, ...) {
    #ifdef DEBUG_UI_BACKEND
    va_list args;
    va_start(args, format);
    fprintf(stderr, "[UI_BACKEND] ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    #else
    (void)format;  // Suppress unused warning
    #endif
}

// Set callbacks - to be called by Ghostty FFI bridge
void ui_backend_set_callbacks(ui_callbacks_t* callbacks, void* user_data) {
    if (callbacks) {
        g_callbacks = *callbacks;
        g_user_data = user_data;
        g_callbacks_registered = 1;
        debug_log("Callbacks registered successfully");
        fprintf(stderr, "[UI_BACKEND] Callbacks registered\n");
    }
}

// Check if callbacks are registered
int ui_backend_has_callbacks(void) {
    return g_callbacks_registered;
}

// Enhanced dispatch function with command ID - replaces the stub from Week 3
int ui_backend_dispatch_enhanced(ui_backend_t* backend,
                                void (*cmdfn)(struct tty *, const struct tty_ctx *),
                                struct tty_ctx* ctx) {
    // Validate inputs
    if (!backend || !ctx) {
        debug_log("Invalid parameters: backend=%p, ctx=%p", backend, ctx);
        return -1;
    }
    
    // Check if we have callbacks registered
    if (!g_callbacks_registered) {
        debug_log("No callbacks registered, falling back to original path");
        return -1;  // Use original rendering path
    }
    
    // Try command ID from context first (more reliable if set)
    tty_cmd_id_t cmd_id = TTY_CMD_UNKNOWN;
    
    if (ctx->ui_cmd_id > 0 && ctx->ui_cmd_id < TTY_CMD_MAX) {
        cmd_id = (tty_cmd_id_t)ctx->ui_cmd_id;
        debug_log("Using command ID from context: %d", cmd_id);
    } else {
        // Fall back to function pointer identification
        cmd_id = identify_command(cmdfn);
        debug_log("Identified command ID from pointer: %d", cmd_id);
    }
    
    fprintf(stderr, "[DISPATCH] Processing command ID: %d\n", cmd_id);
    
    // Process based on command ID instead of direct comparison
    switch (cmd_id) {
    case TTY_CMD_CELL:
        debug_log("Processing tty_cmd_cell");
        fprintf(stderr, "[DISPATCH] Processing tty_cmd_cell\n");
        
        if (ctx->cell && g_callbacks.on_cell) {
            const struct grid_cell* gc = ctx->cell;
            
            // Extract character from UTF8 data
            char ch = ' ';
            if (gc->data.size > 0 && gc->data.data[0] != 0) {
                ch = gc->data.data[0];
            }
            
            // Extract attributes
            int attr = gc->attr;
            int fg = gc->fg;
            int bg = gc->bg;
            
            // Call the callback with cell data
            g_callbacks.on_cell(ch, ctx->ocy, ctx->ocx, attr, fg, bg, g_user_data);
            fprintf(stderr, "[DISPATCH] Cell '%c' at (%d,%d) sent to callback\n", 
                   ch, ctx->ocy, ctx->ocx);
            
            return 0;  // Command handled
        }
        break;
        
    case TTY_CMD_CLEARLINE:
        debug_log("Processing tty_cmd_clearline");
        fprintf(stderr, "[DISPATCH] Processing tty_cmd_clearline\n");
        
        if (g_callbacks.on_clear_line) {
            g_callbacks.on_clear_line(ctx->ocy, g_user_data);
            return 0;  // Command handled
        }
        break;
        
    case TTY_CMD_CLEARSCREEN:
        debug_log("Processing tty_cmd_clearscreen");
        fprintf(stderr, "[DISPATCH] Processing tty_cmd_clearscreen\n");
        
        if (g_callbacks.on_clear_screen) {
            g_callbacks.on_clear_screen(g_user_data);
            return 0;  // Command handled
        }
        break;
        
    case TTY_CMD_INSERTLINE:
        debug_log("Processing tty_cmd_insertline");
        
        if (g_callbacks.on_insert_line) {
            g_callbacks.on_insert_line(ctx->ocy, g_user_data);
            return 0;  // Command handled
        }
        break;
        
    case TTY_CMD_DELETELINE:
        debug_log("Processing tty_cmd_deleteline");
        
        if (g_callbacks.on_delete_line) {
            g_callbacks.on_delete_line(ctx->ocy, g_user_data);
            return 0;  // Command handled
        }
        break;
        
    case TTY_CMD_CLEARENDOFLINE:
        debug_log("Processing tty_cmd_clearendofline");
        
        if (g_callbacks.on_clear_eol) {
            g_callbacks.on_clear_eol(ctx->ocy, ctx->ocx, g_user_data);
            return 0;  // Command handled
        }
        break;
        
    case TTY_CMD_REVERSEINDEX:
        debug_log("Processing tty_cmd_reverseindex");
        
        if (g_callbacks.on_reverse_index) {
            g_callbacks.on_reverse_index(g_user_data);
            return 0;  // Command handled
        }
        break;
        
    case TTY_CMD_LINEFEED:
        debug_log("Processing tty_cmd_linefeed");
        
        if (g_callbacks.on_line_feed) {
            g_callbacks.on_line_feed(g_user_data);
            return 0;  // Command handled
        }
        break;
        
    case TTY_CMD_SCROLLUP:
        debug_log("Processing tty_cmd_scrollup");
        
        if (g_callbacks.on_scroll_up) {
            g_callbacks.on_scroll_up(ctx->num, g_user_data);
            return 0;  // Command handled
        }
        break;
        
    case TTY_CMD_SCROLLDOWN:
        debug_log("Processing tty_cmd_scrolldown");
        
        if (g_callbacks.on_scroll_down) {
            g_callbacks.on_scroll_down(ctx->num, g_user_data);
            return 0;  // Command handled
        }
        break;
        
    case TTY_CMD_UNKNOWN:
    default:
        // Unknown command, log for debugging
        debug_log("Unknown tty command function: %p (ID: %d)", cmdfn, cmd_id);
        // Try to gather more information
        fprintf(stderr, "[DISPATCH] Failed to identify command at %p\n", cmdfn);
        break;
    }
    
    // Command not handled by UI backend
    return -1;  // Use original rendering path
}

// Request a screen flush
void ui_backend_flush(void) {
    if (g_callbacks_registered && g_callbacks.on_flush) {
        g_callbacks.on_flush(g_user_data);
        debug_log("Flush requested");
    }
}

// Get callback status (for testing)
const char* ui_backend_get_status(void) {
    if (g_callbacks_registered) {
        return "Callbacks registered and active";
    } else {
        return "No callbacks registered";
    }
}