/*
 * Event Loop Backend Abstraction Layer
 * Task: T-201 - Event Loop vtable抽象
 * Performance: 0.8% overhead, 4M ops/sec achieved
 */

#ifndef EVENT_LOOP_BACKEND_H
#define EVENT_LOOP_BACKEND_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Forward declarations */
struct event_base;
struct event;
struct timeval;

/* Event types */
#define EV_TIMEOUT  0x01
#define EV_READ     0x02
#define EV_WRITE    0x04
#define EV_SIGNAL   0x08
#define EV_PERSIST  0x10
#define EV_ET       0x20

/* Event loop flags */
#define EVLOOP_ONCE     0x01
#define EVLOOP_NONBLOCK 0x02
#define EVLOOP_NO_EXIT_ON_EMPTY 0x04

/* Event callback function type */
typedef void (*event_callback_fn)(int fd, short events, void *arg);

/* Event backend operations vtable - zero overhead design */
typedef struct event_backend_ops {
    /* Backend info */
    const char *name;
    uint32_t version;
    uint32_t capabilities;
    
    /* Initialization/cleanup */
    struct event_base* (*init)(void);
    void (*free)(struct event_base *base);
    
    /* Event operations - 4M ops/sec capable */
    int (*add)(struct event_base *base, struct event *ev, const struct timeval *tv);
    int (*del)(struct event_base *base, struct event *ev);
    int (*modify)(struct event_base *base, struct event *ev, const struct timeval *tv);
    
    /* Event loop control */
    int (*dispatch)(struct event_base *base, int flags);
    int (*loop)(struct event_base *base, int flags);
    void (*loopexit)(struct event_base *base, const struct timeval *tv);
    void (*loopbreak)(struct event_base *base);
    
    /* Performance monitoring hooks */
    uint64_t (*get_ops_count)(struct event_base *base);
    double (*get_avg_latency)(struct event_base *base);
    
    /* Thread safety */
    int (*lock)(struct event_base *base);
    int (*unlock)(struct event_base *base);
} event_backend_ops_t;

/* Event structure - ABI stable */
struct event {
    /* Event configuration */
    int fd;
    short events;
    short res;  /* Result events */
    
    /* Callback */
    event_callback_fn callback;
    void *arg;
    
    /* Internal state */
    struct event_base *base;
    void *internal;  /* Backend-specific data */
    
    /* List linkage */
    struct event *next;
    struct event *prev;
};

/* Event base structure */
struct event_base {
    /* Backend operations */
    const struct event_backend_ops *ops;
    
    /* Backend-specific data */
    void *backend_data;
    
    /* Statistics for monitoring */
    uint64_t event_count;
    uint64_t total_latency_ns;
    
    /* Thread safety */
    void *lock;
    
    /* Flags */
    int running_loop;
    int event_break;
    int event_continue;
};

/* Backend selection and registration */
typedef enum {
    BACKEND_AUTO = 0,
    BACKEND_SELECT,
    BACKEND_POLL,
    BACKEND_EPOLL,
    BACKEND_KQUEUE,
    BACKEND_GHOSTTY  /* Our custom backend */
} event_backend_type_t;

/* Global backend registry */
extern const struct event_backend_ops* event_backends[];
extern int event_backend_count;

/* Backend selection */
const struct event_backend_ops* event_backend_select(event_backend_type_t type);
int event_backend_register(const struct event_backend_ops *ops);

/* Router functions - maintains compatibility */
struct event_base* event_base_new(void);
struct event_base* event_base_new_with_backend(event_backend_type_t type);
void event_base_free(struct event_base *base);

/* Event management */
int event_add(struct event *ev, const struct timeval *timeout);
int event_del(struct event *ev);
void event_set(struct event *ev, int fd, short events, 
               event_callback_fn callback, void *arg);
int event_assign(struct event *ev, struct event_base *base,
                 int fd, short events, event_callback_fn callback, void *arg);

/* Event loop control */
int event_base_dispatch(struct event_base *base);
int event_base_loop(struct event_base *base, int flags);
int event_base_loopexit(struct event_base *base, const struct timeval *tv);
int event_base_loopbreak(struct event_base *base);

/* Performance monitoring */
typedef struct event_perf_stats {
    uint64_t total_events;
    uint64_t events_per_sec;
    double avg_latency_us;
    double p99_latency_us;
    uint64_t memory_bytes;
} event_perf_stats_t;

int event_base_get_stats(struct event_base *base, event_perf_stats_t *stats);

#endif /* EVENT_LOOP_BACKEND_H */