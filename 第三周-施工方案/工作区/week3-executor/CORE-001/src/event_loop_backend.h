// event_loop_backend.h - Event Loop Backend Interface
// Purpose: Abstract libevent dependency with callback-based vtable
// Author: CORE-001 (c-tmux-specialist)
// Date: 2025-08-25
// Task: T-201 - Event Loop vtable abstraction
// Version: 1.0.0

#ifndef EVENT_LOOP_BACKEND_H
#define EVENT_LOOP_BACKEND_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>

// Forward declarations
struct event_loop_backend;
struct event_loop_router;
typedef struct event_loop_backend event_loop_backend_t;
typedef struct event_loop_router event_loop_router_t;

// Event types (matching libevent's flags)
typedef enum {
    EL_EVENT_READ     = 0x02,  // EV_READ
    EL_EVENT_WRITE    = 0x04,  // EV_WRITE
    EL_EVENT_SIGNAL   = 0x08,  // EV_SIGNAL
    EL_EVENT_TIMEOUT  = 0x01,  // EV_TIMEOUT
    EL_EVENT_PERSIST  = 0x10,  // EV_PERSIST
    EL_EVENT_ET       = 0x20   // EV_ET (edge-triggered)
} event_loop_event_type_t;

// Event handle (opaque to users)
typedef struct event_handle {
    void* backend_data;      // Backend-specific data (e.g., struct event*)
    int fd;                  // File descriptor (-1 for timers/signals)
    int signal;              // Signal number (for signal events)
    uint32_t events;         // Event mask
    void (*callback)(int, short, void*);  // User callback
    void* user_data;         // User data for callback
    struct timeval timeout;  // Timeout value
    bool active;            // Is event active?
    bool pending;           // Is event pending?
} event_handle_t;

// Event loop statistics
typedef struct {
    uint64_t events_added;
    uint64_t events_deleted;
    uint64_t events_dispatched;
    uint64_t loop_iterations;
    uint64_t total_latency_ns;
    uint64_t min_latency_ns;
    uint64_t max_latency_ns;
} event_loop_stats_t;

// Event loop backend vtable
typedef struct event_loop_vtable {
    // Backend name (e.g., "libevent", "ghostty", "epoll")
    const char* name;
    
    // Initialize backend
    void* (*init)(void);
    
    // Cleanup backend
    void (*cleanup)(void* base);
    
    // Event management
    int (*event_add)(void* base, event_handle_t* handle, const struct timeval* timeout);
    int (*event_del)(void* base, event_handle_t* handle);
    int (*event_pending)(void* base, event_handle_t* handle, short events, struct timeval* tv);
    int (*event_active)(void* base, event_handle_t* handle, int res, short ncalls);
    
    // Event loop control
    int (*loop)(void* base, int flags);
    int (*loop_once)(void* base);
    int (*loop_break)(void* base);
    int (*loop_exit)(void* base, const struct timeval* timeout);
    int (*loop_continue)(void* base);
    
    // Signal handling
    int (*signal_add)(void* base, event_handle_t* handle, int signal);
    int (*signal_del)(void* base, event_handle_t* handle);
    
    // Timer handling
    int (*timer_add)(void* base, event_handle_t* handle, const struct timeval* timeout);
    int (*timer_del)(void* base, event_handle_t* handle);
    
    // I/O handling
    int (*io_add)(void* base, event_handle_t* handle, int fd, short events);
    int (*io_del)(void* base, event_handle_t* handle);
    
    // Backend-specific optimizations
    void (*set_priority)(void* base, event_handle_t* handle, int priority);
    int (*get_method)(void* base);  // Returns backend method (poll/select/epoll/kqueue)
    
    // Statistics
    void (*get_stats)(void* base, event_loop_stats_t* stats);
    void (*reset_stats)(void* base);
} event_loop_vtable_t;

// Router modes
typedef enum {
    ROUTER_MODE_LIBEVENT,   // Use original libevent
    ROUTER_MODE_GHOSTTY,    // Use Ghostty callbacks
    ROUTER_MODE_HYBRID      // Use both (for transition)
} router_mode_t;

// Event loop router structure
struct event_loop_router {
    event_loop_vtable_t* vtable;       // Current backend vtable
    void* backend_base;                 // Backend-specific base/context
    router_mode_t mode;                 // Current routing mode
    event_loop_stats_t stats;           // Aggregated statistics
    
    // Performance monitoring
    uint64_t last_dispatch_time_ns;
    uint64_t total_events;
    
    // Thread safety
    void* mutex;  // Platform-specific mutex
};

// ============================================================================
// Public API - These replace libevent calls in tmux
// ============================================================================

// Initialize router with specified backend
event_loop_router_t* event_loop_router_init(router_mode_t mode);

// Cleanup router
void event_loop_router_cleanup(event_loop_router_t* router);

// Switch backend mode at runtime
int event_loop_router_switch_mode(event_loop_router_t* router, router_mode_t new_mode);

// Event management wrappers
event_handle_t* event_loop_create_event(event_loop_router_t* router);
void event_loop_free_event(event_loop_router_t* router, event_handle_t* handle);
int event_loop_add(event_loop_router_t* router, event_handle_t* handle, const struct timeval* timeout);
int event_loop_del(event_loop_router_t* router, event_handle_t* handle);

// Set event parameters
void event_loop_set(event_loop_router_t* router, event_handle_t* handle,
                   int fd, short events,
                   void (*callback)(int, short, void*), void* user_data);

// Main loop operations
int event_loop_run(event_loop_router_t* router, int flags);
int event_loop_run_once(event_loop_router_t* router);
int event_loop_break(event_loop_router_t* router);
int event_loop_exit(event_loop_router_t* router, const struct timeval* timeout);

// Signal handling
event_handle_t* event_loop_signal_new(event_loop_router_t* router, int signal,
                                      void (*callback)(int, short, void*), void* user_data);
int event_loop_signal_add(event_loop_router_t* router, event_handle_t* handle);
int event_loop_signal_del(event_loop_router_t* router, event_handle_t* handle);

// Timer handling
event_handle_t* event_loop_timer_new(event_loop_router_t* router,
                                     void (*callback)(int, short, void*), void* user_data);
int event_loop_timer_add(event_loop_router_t* router, event_handle_t* handle, 
                        const struct timeval* timeout);
int event_loop_timer_del(event_loop_router_t* router, event_handle_t* handle);

// I/O handling
event_handle_t* event_loop_io_new(event_loop_router_t* router, int fd, short events,
                                  void (*callback)(int, short, void*), void* user_data);
int event_loop_io_add(event_loop_router_t* router, event_handle_t* handle);
int event_loop_io_del(event_loop_router_t* router, event_handle_t* handle);

// Statistics and monitoring
void event_loop_get_stats(event_loop_router_t* router, event_loop_stats_t* stats);
void event_loop_reset_stats(event_loop_router_t* router);
double event_loop_get_overhead_percent(event_loop_router_t* router);

// Backend registration (for adding new backends)
int event_loop_register_backend(const char* name, event_loop_vtable_t* vtable);

// ============================================================================
// Compatibility macros for easy tmux integration
// ============================================================================

#ifdef TMUX_EVENT_COMPAT
// Map old libevent calls to new router API
#define event_init()                     event_loop_router_init(ROUTER_MODE_LIBEVENT)
#define event_base_new()                 event_loop_router_init(ROUTER_MODE_LIBEVENT)
#define event_base_free(base)            event_loop_router_cleanup(base)
#define event_add(ev, tv)                event_loop_add(global_router, ev, tv)
#define event_del(ev)                    event_loop_del(global_router, ev)
#define event_loop(flags)                event_loop_run(global_router, flags)
#define event_base_loop(base, flags)    event_loop_run(base, flags)
#define event_base_loopbreak(base)      event_loop_break(base)

// Global router for compatibility mode
extern event_loop_router_t* global_router;
#endif

#endif // EVENT_LOOP_BACKEND_H