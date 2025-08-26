/*
 * monitoring_integration.h - Lightweight Monitoring API for tmux-in-Ghostty
 * 
 * Zero-overhead monitoring when disabled at compile time.
 * Lock-free operations for hot paths.
 * 
 * Performance target: <0.1% CPU overhead
 */

#ifndef MONITORING_INTEGRATION_H
#define MONITORING_INTEGRATION_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Compile-time toggle for monitoring
#ifdef ENABLE_MONITORING
    #define METRICS_ENABLED 1
#else
    #define METRICS_ENABLED 0
#endif

// Metric types
typedef enum {
    METRIC_COUNTER,     // Monotonically increasing value
    METRIC_GAUGE,       // Current value that can go up/down
    METRIC_HISTOGRAM,   // Distribution of values
    METRIC_TIMING       // Time measurements
} metric_type_t;

// Metric handle for efficient access
typedef uint32_t metric_handle_t;
#define INVALID_METRIC_HANDLE 0xFFFFFFFF

// Timing context for measuring durations
typedef struct {
    uint64_t start_ns;
    metric_handle_t handle;
} timing_context_t;

// Core API - all operations are lock-free

// Registration (call once at startup)
metric_handle_t metrics_register(const char* name, metric_type_t type);

// Counter operations
void metrics_increment(metric_handle_t handle);
void metrics_add(metric_handle_t handle, int64_t value);

// Gauge operations
void metrics_gauge_set(metric_handle_t handle, double value);
void metrics_gauge_inc(metric_handle_t handle, double delta);
void metrics_gauge_dec(metric_handle_t handle, double delta);

// Histogram operations
void metrics_observe(metric_handle_t handle, double value);

// Timing operations
timing_context_t metrics_timing_start(metric_handle_t handle);
void metrics_timing_end(timing_context_t* ctx);
uint64_t metrics_timing_elapsed_ns(const timing_context_t* ctx);

// Batch operations for efficiency
void metrics_batch_begin(void);
void metrics_batch_commit(void);

// Component-specific metric handles (predefined for performance)
extern metric_handle_t METRIC_EVENT_LOOP_OPS;
extern metric_handle_t METRIC_EVENT_LOOP_LATENCY;
extern metric_handle_t METRIC_GRID_BATCH_SIZE;
extern metric_handle_t METRIC_GRID_DIRTY_CELLS;
extern metric_handle_t METRIC_FFI_CALLS;
extern metric_handle_t METRIC_FFI_OVERHEAD;
extern metric_handle_t METRIC_LAYOUT_SWITCHES;
extern metric_handle_t METRIC_LAYOUT_RESIZE;
extern metric_handle_t METRIC_MEMORY_ALLOCS;
extern metric_handle_t METRIC_MEMORY_FREES;
extern metric_handle_t METRIC_MEMORY_USAGE;
extern metric_handle_t METRIC_ERRORS_TOTAL;

// Convenience macros for zero-overhead when disabled
#if METRICS_ENABLED
    #define METRICS_INCREMENT(handle) metrics_increment(handle)
    #define METRICS_ADD(handle, value) metrics_add(handle, value)
    #define METRICS_GAUGE_SET(handle, value) metrics_gauge_set(handle, value)
    #define METRICS_OBSERVE(handle, value) metrics_observe(handle, value)
    #define METRICS_TIMING_START(handle) metrics_timing_start(handle)
    #define METRICS_TIMING_END(ctx) metrics_timing_end(ctx)
    
    // Scoped timing helper
    #define METRICS_TIMED_SCOPE(handle, block) do { \
        timing_context_t __ctx = metrics_timing_start(handle); \
        block \
        metrics_timing_end(&__ctx); \
    } while(0)
#else
    // No-ops when monitoring is disabled
    #define METRICS_INCREMENT(handle) ((void)0)
    #define METRICS_ADD(handle, value) ((void)0)
    #define METRICS_GAUGE_SET(handle, value) ((void)0)
    #define METRICS_OBSERVE(handle, value) ((void)0)
    #define METRICS_TIMING_START(handle) ((timing_context_t){0})
    #define METRICS_TIMING_END(ctx) ((void)0)
    #define METRICS_TIMED_SCOPE(handle, block) block
#endif

// Instrumentation points for components

// Event loop instrumentation
#define METRICS_EVENT_LOOP_CALLBACK_START() \
    timing_context_t __evt_ctx = METRICS_TIMING_START(METRIC_EVENT_LOOP_LATENCY)

#define METRICS_EVENT_LOOP_CALLBACK_END() do { \
    METRICS_TIMING_END(&__evt_ctx); \
    METRICS_INCREMENT(METRIC_EVENT_LOOP_OPS); \
} while(0)

// Grid operations instrumentation
#define METRICS_GRID_BATCH_UPDATE(batch_size, dirty_cells) do { \
    METRICS_OBSERVE(METRIC_GRID_BATCH_SIZE, batch_size); \
    METRICS_ADD(METRIC_GRID_DIRTY_CELLS, dirty_cells); \
} while(0)

// FFI bridge instrumentation
#define METRICS_FFI_CALL_START() \
    timing_context_t __ffi_ctx = METRICS_TIMING_START(METRIC_FFI_OVERHEAD)

#define METRICS_FFI_CALL_END() do { \
    METRICS_TIMING_END(&__ffi_ctx); \
    METRICS_INCREMENT(METRIC_FFI_CALLS); \
} while(0)

// Layout manager instrumentation
#define METRICS_LAYOUT_SWITCH() METRICS_INCREMENT(METRIC_LAYOUT_SWITCHES)
#define METRICS_LAYOUT_RESIZE() METRICS_INCREMENT(METRIC_LAYOUT_RESIZE)

// Memory instrumentation
#define METRICS_MEMORY_ALLOC(size) do { \
    METRICS_INCREMENT(METRIC_MEMORY_ALLOCS); \
    METRICS_ADD(METRIC_MEMORY_USAGE, size); \
} while(0)

#define METRICS_MEMORY_FREE(size) do { \
    METRICS_INCREMENT(METRIC_MEMORY_FREES); \
    METRICS_ADD(METRIC_MEMORY_USAGE, -(int64_t)size); \
} while(0)

// Error tracking
#define METRICS_ERROR() METRICS_INCREMENT(METRIC_ERRORS_TOTAL)

// Initialization and cleanup
bool metrics_init(const char* export_path);
void metrics_shutdown(void);

// Export and reporting
void metrics_export_prometheus(int fd);
void metrics_export_json(int fd);
void metrics_reset_all(void);

// Runtime control
void metrics_enable(void);
void metrics_disable(void);
bool metrics_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif // MONITORING_INTEGRATION_H