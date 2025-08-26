// ui_backend.h - UI Backend Interface for tmux
// Purpose: Provide backend routing for tmux tty operations
// Date: 2025-08-26
// Task: T-301-R - tmux source integration

#ifndef UI_BACKEND_H
#define UI_BACKEND_H

#include <stdbool.h>

struct tty;
struct tty_ctx;

// Forward declarations for backend types
struct ui_backend;
typedef struct ui_backend ui_backend_t;

// Check if UI backend is enabled
bool ui_backend_enabled(void);

// Initialize UI backend
int ui_backend_init(void);

// Cleanup UI backend
void ui_backend_cleanup(void);

// Dispatch tty command through UI backend
int ui_backend_dispatch(ui_backend_t* backend, 
                        void (*cmdfn)(struct tty *, const struct tty_ctx *),
                        struct tty_ctx* ctx);

// Get the global UI backend instance
ui_backend_t* ui_backend_get_instance(void);

// Set backend mode (for runtime switching)
void ui_backend_set_mode(const char* mode);

#endif /* UI_BACKEND_H */