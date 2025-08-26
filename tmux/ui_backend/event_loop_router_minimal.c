// event_loop_router_minimal.c - Minimal event loop router for Week 4 testing
// Purpose: Provide stub implementation without libevent dependency
// Date: 2025-08-26

#include "event_loop_backend.h"
#include <stdlib.h>
#include <string.h>

// Minimal implementation of event_loop_router functions
event_loop_router_t* event_loop_router_init(router_mode_t mode) {
    event_loop_router_t* router = (event_loop_router_t*)calloc(1, sizeof(event_loop_router_t));
    if (router) {
        router->mode = mode;
        router->vtable = NULL;  // No real vtable for minimal version
        router->backend_base = NULL;
        router->total_events = 0;
    }
    return router;
}

void event_loop_router_cleanup(event_loop_router_t* router) {
    if (router) {
        free(router);
    }
}

int event_loop_router_switch_mode(event_loop_router_t* router, router_mode_t new_mode) {
    if (router) {
        router->mode = new_mode;
        return 0;
    }
    return -1;
}