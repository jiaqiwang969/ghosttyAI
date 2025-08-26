// ui_backend.c - UI Backend Implementation for tmux
// Purpose: Provide backend routing for tmux tty operations
// Date: 2025-08-26
// Task: T-301-R - tmux source integration

#include "ui_backend.h"
#include "event_loop_backend.h"
#include <stdlib.h>
#include <string.h>

// Global backend state
static struct ui_backend {
    bool enabled;
    event_loop_router_t* router;
    const char* mode;
} g_backend = {
    .enabled = false,
    .router = NULL,
    .mode = "libevent"
};

// Check if UI backend is enabled
bool ui_backend_enabled(void) {
    #ifdef LIBTMUXCORE_BUILD
    return g_backend.enabled;
    #else
    return false;
    #endif
}

// Initialize UI backend
int ui_backend_init(void) {
    #ifdef LIBTMUXCORE_BUILD
    router_mode_t mode;
    
    if (g_backend.router != NULL) {
        return 0; // Already initialized
    }
    
    // Default to libevent mode for compatibility
    mode = ROUTER_MODE_LIBEVENT;
    
    // Check environment variable for backend selection
    const char* backend_env = getenv("TMUX_UI_BACKEND");
    if (backend_env != NULL) {
        if (strcmp(backend_env, "ghostty") == 0) {
            mode = ROUTER_MODE_GHOSTTY;
            g_backend.mode = "ghostty";
        } else if (strcmp(backend_env, "hybrid") == 0) {
            mode = ROUTER_MODE_HYBRID;
            g_backend.mode = "hybrid";
        }
    }
    
    g_backend.router = event_loop_router_init(mode);
    if (g_backend.router == NULL) {
        return -1;
    }
    
    g_backend.enabled = true;
    return 0;
    #else
    return 0; // No-op when not building libtmuxcore
    #endif
}

// Cleanup UI backend
void ui_backend_cleanup(void) {
    #ifdef LIBTMUXCORE_BUILD
    if (g_backend.router != NULL) {
        event_loop_router_cleanup(g_backend.router);
        g_backend.router = NULL;
        g_backend.enabled = false;
    }
    #endif
}

// Declare the enhanced dispatch function from ui_backend_dispatch.c
extern int ui_backend_dispatch_enhanced(ui_backend_t* backend,
                                       void (*cmdfn)(struct tty *, const struct tty_ctx *),
                                       struct tty_ctx* ctx);

// Dispatch tty command through UI backend
int ui_backend_dispatch(ui_backend_t* backend, 
                        void (*cmdfn)(struct tty *, const struct tty_ctx *),
                        struct tty_ctx* ctx) {
    #ifdef LIBTMUXCORE_BUILD
    if (!g_backend.enabled || g_backend.router == NULL) {
        return -1;
    }
    
    // In GHOSTTY mode, use the enhanced dispatch that recognizes commands
    if (g_backend.router->mode == ROUTER_MODE_GHOSTTY) {
        // Week 4 enhancement: Use the real dispatch implementation
        return ui_backend_dispatch_enhanced(backend, cmdfn, ctx);
    }
    
    // In LIBEVENT or HYBRID mode, return -1 to use original path
    return -1;
    #else
    return -1; // Always use original path when not building libtmuxcore
    #endif
}

// Get the global UI backend instance
ui_backend_t* ui_backend_get_instance(void) {
    #ifdef LIBTMUXCORE_BUILD
    return &g_backend;
    #else
    return NULL;
    #endif
}

// Set backend mode (for runtime switching)
void ui_backend_set_mode(const char* mode) {
    #ifdef LIBTMUXCORE_BUILD
    router_mode_t new_mode;
    
    if (mode == NULL) return;
    
    new_mode = ROUTER_MODE_LIBEVENT;
    if (strcmp(mode, "ghostty") == 0) {
        new_mode = ROUTER_MODE_GHOSTTY;
    } else if (strcmp(mode, "hybrid") == 0) {
        new_mode = ROUTER_MODE_HYBRID;
    }
    
    if (g_backend.router != NULL) {
        event_loop_router_switch_mode(g_backend.router, new_mode);
        g_backend.mode = mode;
    }
    #endif
}