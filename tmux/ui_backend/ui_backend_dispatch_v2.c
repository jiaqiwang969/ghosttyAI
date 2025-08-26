// ui_backend_dispatch_v2.c - Alternative dispatch implementation using registration
// Purpose: Use a registration-based approach to avoid function pointer issues
// Date: 2025-08-26
// Task: W4-INC-001 - Week 4 Incremental Enhancement Fix

#include "ui_backend_minimal.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// Command ID system
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

// Callback function table
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

// Global state
static ui_callbacks_t g_callbacks = {0};
static void* g_user_data = NULL;
static int g_callbacks_registered = 0;

// Command registry - maps function pointers to IDs
typedef struct {
    void (*func)(struct tty *, const struct tty_ctx *);
    tty_cmd_id_t id;
} cmd_registry_entry_t;

static cmd_registry_entry_t g_cmd_registry[TTY_CMD_MAX] = {0};
static int g_registry_count = 0;

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
    (void)format;
    #endif
}

// Register a tty command function with an ID
void ui_backend_register_command(void (*func)(struct tty *, const struct tty_ctx *), 
                                 tty_cmd_id_t id) {
    if (g_registry_count < TTY_CMD_MAX) {
        g_cmd_registry[g_registry_count].func = func;
        g_cmd_registry[g_registry_count].id = id;
        g_registry_count++;
        debug_log("Registered command %d at %p", id, func);
    }
}

// Initialize command registry with known functions
void ui_backend_init_commands(void) {
    // This will be called with the actual function addresses from the test
    g_registry_count = 0;
    debug_log("Command registry initialized");
}

// Identify command from registry
static tty_cmd_id_t identify_command_from_registry(void (*cmdfn)(struct tty *, const struct tty_ctx *)) {
    for (int i = 0; i < g_registry_count; i++) {
        if (g_cmd_registry[i].func == cmdfn) {
            return g_cmd_registry[i].id;
        }
    }
    return TTY_CMD_UNKNOWN;
}

// Set callbacks
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

// Enhanced dispatch function - uses registration
int ui_backend_dispatch_enhanced_v2(ui_backend_t* backend,
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
        return -1;
    }
    
    // Try command ID from context first (most reliable)
    tty_cmd_id_t cmd_id = TTY_CMD_UNKNOWN;
    
    if (ctx->ui_cmd_id > 0 && ctx->ui_cmd_id < TTY_CMD_MAX) {
        cmd_id = (tty_cmd_id_t)ctx->ui_cmd_id;
        debug_log("Using command ID from context: %d", cmd_id);
    } else {
        // Use registry lookup
        cmd_id = identify_command_from_registry(cmdfn);
        debug_log("Identified command ID from registry: %d", cmd_id);
    }
    
    fprintf(stderr, "[DISPATCH] Processing command ID: %d\n", cmd_id);
    
    // Process based on command ID
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
            return 0;
        }
        break;
        
    case TTY_CMD_CLEARSCREEN:
        debug_log("Processing tty_cmd_clearscreen");
        fprintf(stderr, "[DISPATCH] Processing tty_cmd_clearscreen\n");
        
        if (g_callbacks.on_clear_screen) {
            g_callbacks.on_clear_screen(g_user_data);
            return 0;
        }
        break;
        
    case TTY_CMD_INSERTLINE:
        debug_log("Processing tty_cmd_insertline");
        
        if (g_callbacks.on_insert_line) {
            g_callbacks.on_insert_line(ctx->ocy, g_user_data);
            return 0;
        }
        break;
        
    case TTY_CMD_DELETELINE:
        debug_log("Processing tty_cmd_deleteline");
        
        if (g_callbacks.on_delete_line) {
            g_callbacks.on_delete_line(ctx->ocy, g_user_data);
            return 0;
        }
        break;
        
    case TTY_CMD_CLEARENDOFLINE:
        debug_log("Processing tty_cmd_clearendofline");
        
        if (g_callbacks.on_clear_eol) {
            g_callbacks.on_clear_eol(ctx->ocy, ctx->ocx, g_user_data);
            return 0;
        }
        break;
        
    case TTY_CMD_REVERSEINDEX:
        debug_log("Processing tty_cmd_reverseindex");
        
        if (g_callbacks.on_reverse_index) {
            g_callbacks.on_reverse_index(g_user_data);
            return 0;
        }
        break;
        
    case TTY_CMD_LINEFEED:
        debug_log("Processing tty_cmd_linefeed");
        
        if (g_callbacks.on_line_feed) {
            g_callbacks.on_line_feed(g_user_data);
            return 0;
        }
        break;
        
    case TTY_CMD_SCROLLUP:
        debug_log("Processing tty_cmd_scrollup");
        
        if (g_callbacks.on_scroll_up) {
            g_callbacks.on_scroll_up(ctx->num, g_user_data);
            return 0;
        }
        break;
        
    case TTY_CMD_SCROLLDOWN:
        debug_log("Processing tty_cmd_scrolldown");
        
        if (g_callbacks.on_scroll_down) {
            g_callbacks.on_scroll_down(ctx->num, g_user_data);
            return 0;
        }
        break;
        
    case TTY_CMD_UNKNOWN:
    default:
        debug_log("Unknown tty command function: %p (ID: %d)", cmdfn, cmd_id);
        fprintf(stderr, "[DISPATCH] Failed to identify command at %p\n", cmdfn);
        break;
    }
    
    // Command not handled
    return -1;
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

// Simplified dispatch wrapper that can be called from ui_backend.c
int ui_backend_dispatch_enhanced(ui_backend_t* backend,
                                void (*cmdfn)(struct tty *, const struct tty_ctx *),
                                struct tty_ctx* ctx) {
    return ui_backend_dispatch_enhanced_v2(backend, cmdfn, ctx);
}