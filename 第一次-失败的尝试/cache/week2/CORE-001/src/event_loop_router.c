// event_loop_router.c - Event Loop Router Implementation
// Purpose: Route event operations through vtable for backend abstraction
// Author: CORE-001 (c-tmux-specialist)
// Date: 2025-08-25
// Task: T-201 - Event Loop vtable abstraction
// Version: 1.0.0

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <event2/event.h>  // For libevent backend
#include "event_loop_backend.h"

// ============================================================================
// Thread Safety
// ============================================================================

typedef struct {
    pthread_mutex_t mutex;
    pthread_rwlock_t mode_lock;
} router_locks_t;

// ============================================================================
// Libevent Backend Implementation
// ============================================================================

static void* libevent_init(void) {
    return event_base_new();
}

static void libevent_cleanup(void* base) {
    if (base) {
        event_base_free((struct event_base*)base);
    }
}

static int libevent_event_add(void* base, event_handle_t* handle, const struct timeval* timeout) {
    struct event* ev = (struct event*)handle->backend_data;
    if (!ev) {
        ev = event_new((struct event_base*)base, handle->fd, handle->events,
                      handle->callback, handle->user_data);
        handle->backend_data = ev;
    }
    return event_add(ev, timeout);
}

static int libevent_event_del(void* base, event_handle_t* handle) {
    struct event* ev = (struct event*)handle->backend_data;
    if (!ev) return -1;
    return event_del(ev);
}

static int libevent_loop(void* base, int flags) {
    return event_base_loop((struct event_base*)base, flags);
}

static int libevent_loop_once(void* base) {
    return event_base_loop((struct event_base*)base, EVLOOP_ONCE);
}

static int libevent_loop_break(void* base) {
    return event_base_loopbreak((struct event_base*)base);
}

static int libevent_loop_exit(void* base, const struct timeval* timeout) {
    return event_base_loopexit((struct event_base*)base, timeout);
}

static int libevent_signal_add(void* base, event_handle_t* handle, int signal) {
    struct event* ev = evsignal_new((struct event_base*)base, signal,
                                    handle->callback, handle->user_data);
    handle->backend_data = ev;
    handle->signal = signal;
    return event_add(ev, NULL);
}

static int libevent_timer_add(void* base, event_handle_t* handle, const struct timeval* timeout) {
    struct event* ev = evtimer_new((struct event_base*)base,
                                  handle->callback, handle->user_data);
    handle->backend_data = ev;
    handle->timeout = *timeout;
    return evtimer_add(ev, timeout);
}

static int libevent_io_add(void* base, event_handle_t* handle, int fd, short events) {
    struct event* ev = event_new((struct event_base*)base, fd, events,
                                 handle->callback, handle->user_data);
    handle->backend_data = ev;
    handle->fd = fd;
    handle->events = events;
    return event_add(ev, NULL);
}

// Libevent vtable
static event_loop_vtable_t libevent_vtable = {
    .name = "libevent",
    .init = libevent_init,
    .cleanup = libevent_cleanup,
    .event_add = libevent_event_add,
    .event_del = libevent_event_del,
    .loop = libevent_loop,
    .loop_once = libevent_loop_once,
    .loop_break = libevent_loop_break,
    .loop_exit = libevent_loop_exit,
    .signal_add = libevent_signal_add,
    .timer_add = libevent_timer_add,
    .io_add = libevent_io_add
};

// ============================================================================
// Ghostty Backend Stub (to be implemented)
// ============================================================================

typedef struct ghostty_event_base {
    // Ghostty-specific event loop data
    void* ghostty_context;
    int epoll_fd;  // For Linux epoll
    int kqueue_fd; // For BSD/macOS kqueue
    // Event list
    event_handle_t** events;
    size_t event_count;
    size_t event_capacity;
} ghostty_event_base_t;

static void* ghostty_init(void) {
    ghostty_event_base_t* base = calloc(1, sizeof(ghostty_event_base_t));
    if (!base) return NULL;
    
    base->event_capacity = 1024;
    base->events = calloc(base->event_capacity, sizeof(event_handle_t*));
    
#ifdef __linux__
    base->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
#elif defined(__APPLE__) || defined(__FreeBSD__)
    base->kqueue_fd = kqueue();
#endif
    
    return base;
}

static void ghostty_cleanup(void* base) {
    ghostty_event_base_t* gbase = (ghostty_event_base_t*)base;
    if (!gbase) return;
    
#ifdef __linux__
    if (gbase->epoll_fd >= 0) close(gbase->epoll_fd);
#elif defined(__APPLE__) || defined(__FreeBSD__)
    if (gbase->kqueue_fd >= 0) close(gbase->kqueue_fd);
#endif
    
    free(gbase->events);
    free(gbase);
}

static int ghostty_event_add(void* base, event_handle_t* handle, const struct timeval* timeout) {
    ghostty_event_base_t* gbase = (ghostty_event_base_t*)base;
    
    // Add to event list
    if (gbase->event_count >= gbase->event_capacity) {
        size_t new_capacity = gbase->event_capacity * 2;
        event_handle_t** new_events = realloc(gbase->events, 
                                              new_capacity * sizeof(event_handle_t*));
        if (!new_events) return -1;
        gbase->events = new_events;
        gbase->event_capacity = new_capacity;
    }
    
    gbase->events[gbase->event_count++] = handle;
    handle->active = true;
    
    // TODO: Add to epoll/kqueue
    return 0;
}

static int ghostty_loop_once(void* base) {
    ghostty_event_base_t* gbase = (ghostty_event_base_t*)base;
    
    // TODO: Implement actual event polling
    // For now, this is a stub that processes callbacks
    
    struct timespec ts = {0, 1000000}; // 1ms
    nanosleep(&ts, NULL);
    
    return 0;
}

// Ghostty vtable (stub)
static event_loop_vtable_t ghostty_vtable = {
    .name = "ghostty",
    .init = ghostty_init,
    .cleanup = ghostty_cleanup,
    .event_add = ghostty_event_add,
    .loop_once = ghostty_loop_once
    // Other functions will be implemented as needed
};

// ============================================================================
// Router Implementation
// ============================================================================

// Global router for compatibility mode
event_loop_router_t* global_router = NULL;

// Get current time in nanoseconds
static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

event_loop_router_t* event_loop_router_init(router_mode_t mode) {
    event_loop_router_t* router = calloc(1, sizeof(event_loop_router_t));
    if (!router) return NULL;
    
    // Initialize locks
    router_locks_t* locks = calloc(1, sizeof(router_locks_t));
    pthread_mutex_init(&locks->mutex, NULL);
    pthread_rwlock_init(&locks->mode_lock, NULL);
    router->mutex = locks;
    
    // Set mode and vtable
    router->mode = mode;
    switch (mode) {
        case ROUTER_MODE_LIBEVENT:
            router->vtable = &libevent_vtable;
            break;
        case ROUTER_MODE_GHOSTTY:
            router->vtable = &ghostty_vtable;
            break;
        case ROUTER_MODE_HYBRID:
            // Start with libevent, can switch later
            router->vtable = &libevent_vtable;
            break;
    }
    
    // Initialize backend
    if (router->vtable && router->vtable->init) {
        router->backend_base = router->vtable->init();
        if (!router->backend_base) {
            free(locks);
            free(router);
            return NULL;
        }
    }
    
    // Initialize stats
    router->stats.min_latency_ns = UINT64_MAX;
    
    // Set as global if not already set
    if (!global_router) {
        global_router = router;
    }
    
    return router;
}

void event_loop_router_cleanup(event_loop_router_t* router) {
    if (!router) return;
    
    router_locks_t* locks = (router_locks_t*)router->mutex;
    
    // Cleanup backend
    if (router->vtable && router->vtable->cleanup) {
        router->vtable->cleanup(router->backend_base);
    }
    
    // Clear global if this was it
    if (global_router == router) {
        global_router = NULL;
    }
    
    // Cleanup locks
    pthread_mutex_destroy(&locks->mutex);
    pthread_rwlock_destroy(&locks->mode_lock);
    free(locks);
    
    free(router);
}

int event_loop_router_switch_mode(event_loop_router_t* router, router_mode_t new_mode) {
    if (!router) return -1;
    
    router_locks_t* locks = (router_locks_t*)router->mutex;
    pthread_rwlock_wrlock(&locks->mode_lock);
    
    // Save old backend
    void* old_base = router->backend_base;
    event_loop_vtable_t* old_vtable = router->vtable;
    
    // Switch vtable
    switch (new_mode) {
        case ROUTER_MODE_LIBEVENT:
            router->vtable = &libevent_vtable;
            break;
        case ROUTER_MODE_GHOSTTY:
            router->vtable = &ghostty_vtable;
            break;
        case ROUTER_MODE_HYBRID:
            // Keep current
            break;
    }
    
    // Initialize new backend
    if (router->vtable != old_vtable) {
        if (router->vtable && router->vtable->init) {
            router->backend_base = router->vtable->init();
            if (!router->backend_base) {
                // Rollback
                router->vtable = old_vtable;
                router->backend_base = old_base;
                pthread_rwlock_unlock(&locks->mode_lock);
                return -1;
            }
        }
        
        // Cleanup old backend
        if (old_vtable && old_vtable->cleanup) {
            old_vtable->cleanup(old_base);
        }
    }
    
    router->mode = new_mode;
    pthread_rwlock_unlock(&locks->mode_lock);
    
    return 0;
}

event_handle_t* event_loop_create_event(event_loop_router_t* router) {
    if (!router) return NULL;
    
    event_handle_t* handle = calloc(1, sizeof(event_handle_t));
    if (!handle) return NULL;
    
    handle->fd = -1;
    handle->signal = -1;
    
    return handle;
}

void event_loop_free_event(event_loop_router_t* router, event_handle_t* handle) {
    if (!router || !handle) return;
    
    // Delete from backend if active
    if (handle->active && router->vtable && router->vtable->event_del) {
        router->vtable->event_del(router->backend_base, handle);
    }
    
    // Free backend data
    if (handle->backend_data) {
        if (router->vtable == &libevent_vtable) {
            event_free((struct event*)handle->backend_data);
        }
        // Add other backend cleanup as needed
    }
    
    free(handle);
}

int event_loop_add(event_loop_router_t* router, event_handle_t* handle, 
                  const struct timeval* timeout) {
    if (!router || !handle) return -1;
    
    router_locks_t* locks = (router_locks_t*)router->mutex;
    pthread_rwlock_rdlock(&locks->mode_lock);
    
    int result = -1;
    uint64_t start_time = get_time_ns();
    
    if (router->vtable && router->vtable->event_add) {
        result = router->vtable->event_add(router->backend_base, handle, timeout);
        
        if (result == 0) {
            router->stats.events_added++;
            handle->active = true;
            if (timeout) {
                handle->timeout = *timeout;
            }
        }
    }
    
    uint64_t latency = get_time_ns() - start_time;
    router->stats.total_latency_ns += latency;
    if (latency < router->stats.min_latency_ns) {
        router->stats.min_latency_ns = latency;
    }
    if (latency > router->stats.max_latency_ns) {
        router->stats.max_latency_ns = latency;
    }
    
    pthread_rwlock_unlock(&locks->mode_lock);
    
    return result;
}

int event_loop_del(event_loop_router_t* router, event_handle_t* handle) {
    if (!router || !handle) return -1;
    
    router_locks_t* locks = (router_locks_t*)router->mutex;
    pthread_rwlock_rdlock(&locks->mode_lock);
    
    int result = -1;
    
    if (router->vtable && router->vtable->event_del) {
        result = router->vtable->event_del(router->backend_base, handle);
        
        if (result == 0) {
            router->stats.events_deleted++;
            handle->active = false;
        }
    }
    
    pthread_rwlock_unlock(&locks->mode_lock);
    
    return result;
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
    if (!router) return -1;
    
    router_locks_t* locks = (router_locks_t*)router->mutex;
    pthread_rwlock_rdlock(&locks->mode_lock);
    
    int result = -1;
    
    if (router->vtable && router->vtable->loop) {
        uint64_t start_time = get_time_ns();
        result = router->vtable->loop(router->backend_base, flags);
        uint64_t latency = get_time_ns() - start_time;
        
        router->stats.loop_iterations++;
        router->stats.total_latency_ns += latency;
    }
    
    pthread_rwlock_unlock(&locks->mode_lock);
    
    return result;
}

int event_loop_run_once(event_loop_router_t* router) {
    if (!router) return -1;
    
    router_locks_t* locks = (router_locks_t*)router->mutex;
    pthread_rwlock_rdlock(&locks->mode_lock);
    
    int result = -1;
    
    if (router->vtable && router->vtable->loop_once) {
        uint64_t start_time = get_time_ns();
        result = router->vtable->loop_once(router->backend_base);
        uint64_t latency = get_time_ns() - start_time;
        
        router->stats.loop_iterations++;
        router->stats.total_latency_ns += latency;
        router->last_dispatch_time_ns = latency;
    }
    
    pthread_rwlock_unlock(&locks->mode_lock);
    
    return result;
}

void event_loop_get_stats(event_loop_router_t* router, event_loop_stats_t* stats) {
    if (!router || !stats) return;
    
    router_locks_t* locks = (router_locks_t*)router->mutex;
    pthread_mutex_lock(&locks->mutex);
    
    *stats = router->stats;
    
    pthread_mutex_unlock(&locks->mutex);
}

void event_loop_reset_stats(event_loop_router_t* router) {
    if (!router) return;
    
    router_locks_t* locks = (router_locks_t*)router->mutex;
    pthread_mutex_lock(&locks->mutex);
    
    memset(&router->stats, 0, sizeof(router->stats));
    router->stats.min_latency_ns = UINT64_MAX;
    
    pthread_mutex_unlock(&locks->mutex);
}

double event_loop_get_overhead_percent(event_loop_router_t* router) {
    if (!router) return 0.0;
    
    // Calculate overhead vs native libevent
    // This is a simplified calculation for demonstration
    if (router->stats.loop_iterations == 0) return 0.0;
    
    uint64_t avg_latency = router->stats.total_latency_ns / router->stats.loop_iterations;
    
    // Assume native libevent baseline of 100ns per operation
    uint64_t baseline_ns = 100;
    
    if (avg_latency <= baseline_ns) return 0.0;
    
    return ((double)(avg_latency - baseline_ns) / baseline_ns) * 100.0;
}