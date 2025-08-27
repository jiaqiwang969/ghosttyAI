/* libtmuxcore_api.c - Core API implementation */
#include "libtmuxcore_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Version information */
#define LIBTMUXCORE_VERSION_MAJOR 1
#define LIBTMUXCORE_VERSION_MINOR 0
#define LIBTMUXCORE_VERSION_PATCH 0

/* External functions from stubs */
extern void log_open(const char *name);
extern void log_close(void);
extern void log_debug(const char *fmt, ...);

/* Global state */
static int tmc_initialized = 0;
static struct ui_backend_vtable *registered_vtable = NULL;

/* Initialize the library */
tmc_error_t tmc_init(void) {
    if (tmc_initialized) {
        return TMC_SUCCESS;
    }
    
    log_open("libtmuxcore");
    tmc_initialized = 1;
    
    printf("[TMC] Library initialized\n");
    return TMC_SUCCESS;
}

/* Cleanup */
void tmc_cleanup(void) {
    if (!tmc_initialized) {
        return;
    }
    
    log_close();
    tmc_initialized = 0;
    registered_vtable = NULL;
    
    printf("[TMC] Library cleaned up\n");
}

/* Forward declarations for real implementations */
extern tmc_error_t tmc_session_new_real(const char *name, tmc_session_t *session);
extern tmc_error_t tmc_session_attach_real(tmc_session_t session);
extern tmc_error_t tmc_session_detach_real(tmc_session_t session);
extern tmc_error_t tmc_session_destroy_real(tmc_session_t session);
extern tmc_session_t tmc_session_current_real(void);
extern tmc_error_t tmc_window_new_real(tmc_session_t session, const char *name, tmc_window_t *window);
extern tmc_window_t tmc_window_current_real(void);
extern tmc_error_t tmc_pane_split_real(tmc_window_t window, int horizontal, uint32_t size_percent, tmc_pane_t *new_pane);
extern tmc_pane_t tmc_pane_current_real(void);
extern tmc_error_t tmc_command_execute_real(const char *command);

/* Session management */
tmc_error_t tmc_session_new(const char *name, tmc_session_t *session) {
    if (!tmc_initialized) {
        return TMC_ERROR_NOT_INITIALIZED;
    }
    
    return tmc_session_new_real(name, session);
}

tmc_error_t tmc_session_attach(tmc_session_t session) {
    if (!tmc_initialized) {
        return TMC_ERROR_NOT_INITIALIZED;
    }
    
    return tmc_session_attach_real(session);
}

tmc_error_t tmc_session_detach(tmc_session_t session) {
    if (!tmc_initialized) {
        return TMC_ERROR_NOT_INITIALIZED;
    }
    
    return tmc_session_detach_real(session);
}

/* Window management */
tmc_error_t tmc_window_new(tmc_session_t session, const char *name, tmc_window_t *window) {
    if (!tmc_initialized) {
        return TMC_ERROR_NOT_INITIALIZED;
    }
    
    return tmc_window_new_real(session, name, window);
}

/* Pane management */
tmc_error_t tmc_pane_split(tmc_window_t window, int horizontal, 
                           uint32_t size_percent, tmc_pane_t *new_pane) {
    if (!tmc_initialized) {
        return TMC_ERROR_NOT_INITIALIZED;
    }
    
    return tmc_pane_split_real(window, horizontal, size_percent, new_pane);
}

/* Command execution */
tmc_error_t tmc_command_execute(const char *command) {
    if (!tmc_initialized) {
        return TMC_ERROR_NOT_INITIALIZED;
    }
    
    return tmc_command_execute_real(command);
}

tmc_error_t tmc_command_send_keys(tmc_pane_t pane, const char *keys) {
    if (!tmc_initialized) {
        return TMC_ERROR_NOT_INITIALIZED;
    }
    
    if (!pane || !keys) {
        return TMC_ERROR_INVALID_PARAM;
    }
    
    printf("[TMC] Sending keys to %s: %s\n", (char*)pane, keys);
    return TMC_SUCCESS;
}

/* Callback registration */
tmc_error_t tmc_callbacks_register(const tmc_callbacks_t *callbacks) {
    if (!tmc_initialized) {
        return TMC_ERROR_NOT_INITIALIZED;
    }
    
    printf("[TMC] Callbacks registered\n");
    return TMC_SUCCESS;
}

/* Get library version */
uint32_t tmc_get_version(void) {
    return (LIBTMUXCORE_VERSION_MAJOR << 16) |
           (LIBTMUXCORE_VERSION_MINOR << 8) |
           LIBTMUXCORE_VERSION_PATCH;
}