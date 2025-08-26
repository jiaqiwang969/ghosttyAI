// libtmuxcore.c - Main entry point for libtmuxcore
// Purpose: Provide public API for libtmuxcore library
// Date: 2025-08-26
// Task: T-302-R - Build libtmuxcore dynamic library

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "ui_backend/ui_backend.h"
#include "ui_backend/event_loop_backend.h"

// Version information
#define LIBTMUXCORE_VERSION_MAJOR 1
#define LIBTMUXCORE_VERSION_MINOR 0
#define LIBTMUXCORE_VERSION_PATCH 0

// Opaque handle for tmux core instance
typedef struct tmc_handle {
    void* internal_data;
    ui_backend_t* backend;
    bool initialized;
} tmc_handle_t;

// Initialize libtmuxcore
tmc_handle_t* tmc_init(void) {
    tmc_handle_t* handle = calloc(1, sizeof(tmc_handle_t));
    if (!handle) return NULL;
    
    // Initialize UI backend
    if (ui_backend_init() != 0) {
        free(handle);
        return NULL;
    }
    
    handle->backend = ui_backend_get_instance();
    handle->initialized = true;
    
    return handle;
}

// Cleanup libtmuxcore
void tmc_cleanup(tmc_handle_t* handle) {
    if (!handle) return;
    
    if (handle->initialized) {
        ui_backend_cleanup();
    }
    
    free(handle);
}

// Create a new tmux session
int tmc_create_session(tmc_handle_t* handle, const char* name) {
    if (!handle || !handle->initialized) return -1;
    
    // TODO: Implement actual session creation
    // This will be connected to tmux's session management in T-303-R
    (void)name;
    
    return 0;
}

// Destroy a tmux session
int tmc_destroy_session(tmc_handle_t* handle, const char* name) {
    if (!handle || !handle->initialized) return -1;
    
    // TODO: Implement actual session destruction
    (void)name;
    
    return 0;
}

// Register UI callbacks structure
typedef struct tmc_ui_callbacks {
    void (*on_redraw)(void* user_data);
    void (*on_cell_update)(int row, int col, const char* text, void* user_data);
    void (*on_cursor_move)(int row, int col, void* user_data);
    void (*on_resize)(int rows, int cols, void* user_data);
    void* user_data;
} tmc_ui_callbacks_t;

// Register callbacks for UI events
int tmc_register_callbacks(tmc_handle_t* handle, const tmc_ui_callbacks_t* callbacks) {
    if (!handle || !handle->initialized || !callbacks) return -1;
    
    // TODO: Store and use callbacks
    // This will be connected to the event dispatch in T-303-R
    
    return 0;
}

// Execute a tmux command
int tmc_execute_command(tmc_handle_t* handle, const char* command) {
    if (!handle || !handle->initialized || !command) return -1;
    
    // TODO: Parse and execute tmux command
    // This will be connected to tmux's command system
    
    return 0;
}

// Get version information
void tmc_get_version(int* major, int* minor, int* patch) {
    if (major) *major = LIBTMUXCORE_VERSION_MAJOR;
    if (minor) *minor = LIBTMUXCORE_VERSION_MINOR;
    if (patch) *patch = LIBTMUXCORE_VERSION_PATCH;
}

// Set backend mode (ghostty, libevent, or hybrid)
int tmc_set_backend_mode(tmc_handle_t* handle, const char* mode) {
    if (!handle || !handle->initialized || !mode) return -1;
    
    ui_backend_set_mode(mode);
    return 0;
}