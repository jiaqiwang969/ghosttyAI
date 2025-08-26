// libtmuxcore.h - Public API for libtmuxcore
// Purpose: Define the public interface for libtmuxcore library
// Date: 2025-08-26
// Task: T-302-R - Build libtmuxcore dynamic library

#ifndef LIBTMUXCORE_H
#define LIBTMUXCORE_H

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle for tmux core instance
typedef struct tmc_handle tmc_handle_t;

// UI callback structure
typedef struct tmc_ui_callbacks {
    void (*on_redraw)(void* user_data);
    void (*on_cell_update)(int row, int col, const char* text, void* user_data);
    void (*on_cursor_move)(int row, int col, void* user_data);
    void (*on_resize)(int rows, int cols, void* user_data);
    void* user_data;
} tmc_ui_callbacks_t;

// Core API functions
tmc_handle_t* tmc_init(void);
void tmc_cleanup(tmc_handle_t* handle);

// Session management
int tmc_create_session(tmc_handle_t* handle, const char* name);
int tmc_destroy_session(tmc_handle_t* handle, const char* name);

// UI callbacks
int tmc_register_callbacks(tmc_handle_t* handle, const tmc_ui_callbacks_t* callbacks);

// Command execution
int tmc_execute_command(tmc_handle_t* handle, const char* command);

// Configuration
int tmc_set_backend_mode(tmc_handle_t* handle, const char* mode);

// Version information
void tmc_get_version(int* major, int* minor, int* patch);

#ifdef __cplusplus
}
#endif

#endif /* LIBTMUXCORE_H */