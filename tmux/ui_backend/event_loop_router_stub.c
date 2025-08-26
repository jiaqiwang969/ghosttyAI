// event_loop_router_stub.c - Stub implementation for event loop router
// Purpose: Minimal implementation for initial build testing
// Date: 2025-08-26
// Task: T-301-R - tmux source integration

#include "event_loop_backend.h"
#include <stdlib.h>
#include <string.h>

// The router structure is already defined in event_loop_backend.h
// We just provide the implementation here

// Initialize router with specified backend
event_loop_router_t* event_loop_router_init(router_mode_t mode) {
    event_loop_router_t* router = calloc(1, sizeof(event_loop_router_t));
    if (!router) return NULL;
    
    router->mode = mode;
    router->backend_base = NULL;
    router->vtable = NULL;
    
    return router;
}

// Cleanup router
void event_loop_router_cleanup(event_loop_router_t* router) {
    if (router) {
        free(router);
    }
}

// Switch backend mode at runtime
int event_loop_router_switch_mode(event_loop_router_t* router, router_mode_t new_mode) {
    if (!router) return -1;
    router->mode = new_mode;
    return 0;
}

// Event management wrappers - all stubs for now
event_handle_t* event_loop_create_event(event_loop_router_t* router) {
    if (!router) return NULL;
    return calloc(1, sizeof(event_handle_t));
}

void event_loop_free_event(event_loop_router_t* router, event_handle_t* handle) {
    if (handle) free(handle);
}

int event_loop_add(event_loop_router_t* router, event_handle_t* handle, const struct timeval* timeout) {
    return 0; // Success
}

int event_loop_del(event_loop_router_t* router, event_handle_t* handle) {
    return 0; // Success
}

void event_loop_set(event_loop_router_t* router, event_handle_t* handle,
                   int fd, short events,
                   void (*callback)(int, short, void*), void* user_data) {
    if (!handle) return;
    handle->fd = fd;
    handle->events = events;
    handle->callback = callback;
    handle->user_data = user_data;
}

int event_loop_run(event_loop_router_t* router, int flags) {
    return 0; // Success
}

int event_loop_run_once(event_loop_router_t* router) {
    return 0; // Success
}

int event_loop_break(event_loop_router_t* router) {
    return 0; // Success
}