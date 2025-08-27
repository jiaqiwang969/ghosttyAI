#ifndef LIBTMUXCORE_API_H
#define LIBTMUXCORE_API_H

#include <stdint.h>
#include <stddef.h>

/* Error codes */
typedef enum {
    TMC_SUCCESS = 0,
    TMC_ERROR_INVALID_PARAM = -1,
    TMC_ERROR_NOT_FOUND = -2,
    TMC_ERROR_ALREADY_EXISTS = -3,
    TMC_ERROR_OUT_OF_MEMORY = -4,
    TMC_ERROR_NOT_INITIALIZED = -5
} tmc_error_t;

/* Handle types */
typedef void* tmc_session_t;
typedef void* tmc_window_t;
typedef void* tmc_pane_t;

/* UI Backend vtable (forward declaration) */
struct ui_backend_vtable;

/* Initialize and cleanup */
tmc_error_t tmc_init(void);
void tmc_cleanup(void);

/* Session management */
tmc_error_t tmc_session_new(const char *name, tmc_session_t *session);
tmc_error_t tmc_session_attach(tmc_session_t session);
tmc_error_t tmc_session_detach(tmc_session_t session);
tmc_error_t tmc_session_rename(tmc_session_t session, const char *new_name);
tmc_error_t tmc_session_destroy(tmc_session_t session);
tmc_session_t tmc_session_current(void);

/* Window management */
tmc_error_t tmc_window_new(tmc_session_t session, const char *name, tmc_window_t *window);
tmc_error_t tmc_window_close(tmc_window_t window);
tmc_error_t tmc_window_rename(tmc_window_t window, const char *new_name);
tmc_error_t tmc_window_select(tmc_window_t window);
tmc_error_t tmc_window_next(void);
tmc_error_t tmc_window_previous(void);
tmc_window_t tmc_window_current(void);

/* Pane management */
tmc_error_t tmc_pane_split(tmc_window_t window, int horizontal, 
                           uint32_t size_percent, tmc_pane_t *new_pane);
tmc_error_t tmc_pane_close(tmc_pane_t pane);
tmc_error_t tmc_pane_resize(tmc_pane_t pane, uint32_t width, uint32_t height);
tmc_error_t tmc_pane_select(tmc_pane_t pane);
tmc_error_t tmc_pane_zoom_toggle(tmc_pane_t pane);
tmc_pane_t tmc_pane_current(void);

/* Command execution */
tmc_error_t tmc_command_execute(const char *command);
tmc_error_t tmc_command_send_keys(tmc_pane_t pane, const char *keys);

/* Callbacks */
typedef struct tmc_callbacks {
    void (*on_output)(tmc_pane_t pane, const char *data, size_t len);
    void (*on_bell)(tmc_pane_t pane);
    void (*on_title_change)(tmc_pane_t pane, const char *title);
    void (*on_activity)(tmc_window_t window);
} tmc_callbacks_t;

tmc_error_t tmc_callbacks_register(const tmc_callbacks_t *callbacks);

#endif /* LIBTMUXCORE_API_H */