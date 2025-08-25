// backend_ghostty_minimal.c - Minimal Ghostty Backend for Testing
// Purpose: Minimal implementation for integration testing
// Date: 2025-08-25

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "ui_backend.h"
#include "backend_router.h"

// Simplified Ghostty backend structure
typedef struct {
    int initialized;
    int call_count;
} ghostty_backend_t;

// Create Ghostty backend
struct ui_backend* ghostty_backend_create(void* ghostty_handle) {
    (void)ghostty_handle;
    
    // Allocate the full ui_backend structure
    struct ui_backend* backend = calloc(1, sizeof(struct ui_backend));
    if (!backend) return NULL;
    
    // Allocate private data
    ghostty_backend_t* priv = calloc(1, sizeof(ghostty_backend_t));
    if (!priv) {
        free(backend);
        return NULL;
    }
    
    priv->initialized = 1;
    priv->call_count = 0;
    
    // Set up backend structure
    backend->size = sizeof(struct ui_backend);
    backend->version = 1;
    backend->type = UI_BACKEND_GHOSTTY;
    backend->priv = priv;
    
    // Set basic callbacks (can be NULL for minimal testing)
    backend->on_frame = NULL;
    backend->on_bell = NULL;
    backend->on_title = NULL;
    backend->on_overflow = NULL;
    backend->user_data = NULL;
    
    return backend;
}

// Destroy Ghostty backend
void ghostty_backend_destroy(struct ui_backend* backend) {
    if (backend) {
        if (backend->priv) {
            free(backend->priv);
        }
        free(backend);
    }
}

// Get statistics (for testing)
int ghostty_backend_get_call_count(struct ui_backend* backend) {
    if (!backend || !backend->priv) return 0;
    ghostty_backend_t* priv = (ghostty_backend_t*)backend->priv;
    return priv->call_count;
}

// Increment call count (for testing)
void ghostty_backend_increment_call_count(struct ui_backend* backend) {
    if (!backend || !backend->priv) return;
    ghostty_backend_t* priv = (ghostty_backend_t*)backend->priv;
    priv->call_count++;
}