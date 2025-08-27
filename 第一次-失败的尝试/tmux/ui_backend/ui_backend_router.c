#include "ui_backend.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* External log function from stubs */
extern void log_debug(const char *fmt, ...);

/* Global vtable pointer */
struct ui_backend_vtable *ui_backend = NULL;

void
ui_backend_register(struct ui_backend_vtable *vtable)
{
    if (vtable == NULL) {
        log_debug("UI Backend: Attempting to register NULL vtable");
        return;
    }
    
    ui_backend = vtable;
    log_debug("UI Backend: Registered vtable at %p", vtable);
}

void
ui_backend_unregister(void)
{
    if (ui_backend) {
        log_debug("UI Backend: Unregistering vtable");
        ui_backend = NULL;
    }
}

int
ui_backend_is_active(void)
{
    return (ui_backend != NULL);
}

/* Create default vtable for testing */
struct ui_backend_vtable *
ui_backend_create_default(void)
{
    struct ui_backend_vtable *vtable = calloc(1, sizeof(*vtable));
    if (vtable) {
        /* Set basic callbacks */
        log_debug("UI Backend: Created default vtable");
    }
    return vtable;
}