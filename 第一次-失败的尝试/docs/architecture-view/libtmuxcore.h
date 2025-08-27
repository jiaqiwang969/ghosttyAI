// libtmuxcore.h — Enhanced ABI-stable tmux core embedding interface
// SPDX-License-Identifier: ISC
// Version 2.0 with comprehensive improvements based on architecture review

#ifndef LIBTMUXCORE_H
#define LIBTMUXCORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ABI Version Management
#define TMC_ABI_VERSION_MAJOR 2
#define TMC_ABI_VERSION_MINOR 0
#define TMC_ABI_VERSION_PATCH 0
#define TMC_ABI_VERSION ((TMC_ABI_VERSION_MAJOR << 16) | \
                        (TMC_ABI_VERSION_MINOR << 8) | \
                        TMC_ABI_VERSION_PATCH)

// Export macros
#if defined(_WIN32) && defined(TMC_BUILD_SHARED)
#  ifdef TMC_EXPORTS
#    define TMC_API __declspec(dllexport)
#  else
#    define TMC_API __declspec(dllimport)
#  endif
#else
#  define TMC_API __attribute__((visibility("default")))
#endif

// Deprecation marking
#ifdef __GNUC__
#  define TMC_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#  define TMC_DEPRECATED __declspec(deprecated)
#else
#  define TMC_DEPRECATED
#endif

/* ============= Opaque Handles ============= */
typedef struct tmc_server      tmc_server_t;
typedef struct tmc_client      tmc_client_t;
typedef struct tmc_session     tmc_session_t;
typedef struct tmc_window      tmc_window_t;
typedef struct tmc_pane        tmc_pane_t;
typedef struct tmc_io          tmc_io_t;

/* ============= Error Handling ============= */
typedef enum {
    TMC_OK = 0,
    TMC_ERR_UNKNOWN = -1,
    TMC_ERR_INVALID = -2,
    TMC_ERR_NOMEM = -3,
    TMC_ERR_BUSY = -4,
    TMC_ERR_NOTFOUND = -5,
    TMC_ERR_PERM = -6,
    TMC_ERR_OVERFLOW = -7,
    TMC_ERR_VERSION = -8,
    TMC_ERR_TIMEOUT = -9,
} tmc_err_t;

typedef enum {
    TMC_ERR_DOMAIN_SYSTEM,
    TMC_ERR_DOMAIN_PARSE,
    TMC_ERR_DOMAIN_EXEC,
    TMC_ERR_DOMAIN_TARGET,
    TMC_ERR_DOMAIN_NETWORK,
} tmc_err_domain_t;

typedef struct {
    uint32_t size;              // sizeof(tmc_error_info_t)
    tmc_err_t code;
    tmc_err_domain_t domain;
    const char* message;
    const char* file;
    uint32_t line;
} tmc_error_info_t;

/* ============= Capability Negotiation ============= */
typedef enum {
    TMC_CAP_FRAME_BATCH    = 1 << 0,
    TMC_CAP_UTF8_LINES     = 1 << 1,
    TMC_CAP_24BIT_COLOR    = 1 << 2,
    TMC_CAP_BORDERS_BY_UI  = 1 << 3,
    TMC_CAP_CURSOR_SHAPES  = 1 << 4,
    TMC_CAP_UNDERLINE_STYLES = 1 << 5,
    TMC_CAP_SHARED_MEMORY  = 1 << 6,
    TMC_CAP_ASYNC_COMMANDS = 1 << 7,
    TMC_CAP_METRICS_EXPORT = 1 << 8,
} tmc_cap_flags_t;

typedef struct {
    uint32_t size;              // MUST be first field
    uint32_t abi_version;
    uint64_t ui_capabilities;   // Bitmask of tmc_cap_flags_t
    uint64_t core_capabilities; // What core supports
    
    // Feature-specific versions
    uint16_t grid_api_version;
    uint16_t event_api_version;
    uint16_t command_api_version;
    uint16_t reserved;
    
    // Performance hints
    uint32_t max_fps;
    uint32_t buffer_frames;
    uint32_t max_span_size;
} tmc_capabilities_t;

/* ============= Event Loop Bridge (Host-Provided) ============= */
typedef void (*tmc_io_fd_cb_t)(int fd, int events, void* user);
typedef void (*tmc_io_timer_cb_t)(void* user);

typedef struct {
    uint32_t size;              // MUST be first field
    uint32_t version;
    
    // File descriptor management
    int   (*add_fd)(tmc_io_t* io, int fd, int events, tmc_io_fd_cb_t cb, void* user);
    int   (*mod_fd)(tmc_io_t* io, int handle, int events);
    void  (*del_fd)(tmc_io_t* io, int handle);
    
    // Timer management
    int   (*add_timer)(tmc_io_t* io, uint64_t interval_ms, int repeat, tmc_io_timer_cb_t cb, void* user);
    void  (*del_timer)(tmc_io_t* io, int handle);
    
    // Thread synchronization
    void  (*post)(tmc_io_t* io, void (*fn)(void*), void* user);
    void  (*wake)(tmc_io_t* io);  // Wake event loop from another thread
} tmc_loop_vtable_t;

/* ============= Enhanced Cell and Text Representation ============= */

// Color sentinel values
#define TMC_COLOR_DEFAULT   0xFFFFFFFE
#define TMC_COLOR_INVALID   0xFFFFFFFF

typedef enum {
    TMC_ATTR_BOLD       = 1 << 0,
    TMC_ATTR_ITALIC     = 1 << 1,
    TMC_ATTR_UNDERLINE  = 1 << 2,
    TMC_ATTR_DIM        = 1 << 3,
    TMC_ATTR_REVERSE    = 1 << 4,
    TMC_ATTR_BLINK      = 1 << 5,
    TMC_ATTR_STRIKE     = 1 << 6,
    TMC_ATTR_DOUBLE_UL  = 1 << 7,
    TMC_ATTR_CURLY_UL   = 1 << 8,
    TMC_ATTR_DOTTED_UL  = 1 << 9,
    TMC_ATTR_DASHED_UL  = 1 << 10,
} tmc_attr_flags_t;

// Enhanced cell structure for proper Unicode handling
typedef struct {
    uint32_t codepoint;         // Primary codepoint or cluster marker
    uint32_t fg_rgb;            // Foreground color (TMC_COLOR_DEFAULT for default)
    uint32_t bg_rgb;            // Background color
    uint16_t attrs;             // Attribute flags
    uint8_t  width;             // Display width (0, 1, 2)
    uint8_t  cluster_cont;      // Continuation of grapheme cluster
} tmc_cell_t;

// UTF-8 line representation for efficient updates
typedef struct {
    uint32_t row;
    const char* utf8_content;   // UTF-8 encoded line
    uint32_t byte_length;       // Total bytes
    const uint32_t* cell_byte_offsets; // Byte offset for each cell
    uint32_t cell_count;
    const tmc_attr_flags_t* cell_attrs; // Parallel attribute array
} tmc_line_utf8_t;

/* ============= Frame-Based Grid Updates ============= */

typedef enum {
    TMC_FRAME_COMPLETE  = 1 << 0,
    TMC_FRAME_PARTIAL   = 1 << 1,
    TMC_FRAME_URGENT    = 1 << 2,
    TMC_FRAME_DROPPED   = 1 << 3,
    TMC_FRAME_SNAPSHOT  = 1 << 4,
    TMC_FRAME_CURSOR    = 1 << 5,
} tmc_frame_flags_t;

typedef struct {
    uint32_t row;
    uint32_t col_start;
    uint32_t col_end;
    const tmc_cell_t* cells;    // View into core-owned buffer
} tmc_span_t;

// Frame-level batched update
typedef struct {
    uint32_t size;              // MUST be first field
    uint64_t frame_seq;         // Monotonic sequence number
    uint64_t timestamp_ns;      // Frame generation time
    uint32_t pane_id;
    uint32_t span_count;
    const tmc_span_t* spans;
    tmc_frame_flags_t flags;
    
    // Statistics
    uint32_t updates_batched;   // Updates merged into this frame
    uint32_t cells_modified;
    uint32_t frames_dropped;    // Frames dropped before this one
} tmc_frame_t;

/* ============= Layout and Window Management ============= */

typedef enum {
    TMC_LAYOUT_VERTICAL,
    TMC_LAYOUT_HORIZONTAL,
    TMC_LAYOUT_LEAF,
} tmc_layout_type_t;

typedef struct {
    tmc_layout_type_t type;
    uint32_t pane_id;           // Valid when type == LEAF
    uint32_t x, y, w, h;        // Position and size in cells
    float split_ratio;          // For non-leaf nodes
} tmc_layout_node_t;

typedef struct {
    uint32_t size;              // MUST be first field
    uint32_t window_id;
    uint32_t active_pane_id;
    uint32_t node_count;
    const tmc_layout_node_t* nodes;
} tmc_layout_update_t;

/* ============= UI Callbacks (Core -> Host) ============= */

typedef struct {
    uint32_t size;              // MUST be first field
    uint32_t version;
    
    // Frame-based grid updates (preferred)
    void (*on_frame)(tmc_client_t* client, const tmc_frame_t* frame, void* user);
    
    // Legacy callbacks (for compatibility)
    void (*on_grid)(tmc_server_t* server, const tmc_frame_t* update, void* user);
    void (*on_layout)(tmc_server_t* server, const tmc_layout_update_t* layout, void* user);
    void (*on_title)(tmc_server_t* server, uint32_t pane_id, const char* title, void* user);
    void (*on_renamed)(tmc_server_t* server, uint32_t window_id, const char* name, void* user);
    void (*on_session)(tmc_server_t* server, uint32_t session_id, const char* name, bool created, void* user);
    
    // Copy mode and selection
    void (*on_copy_mode)(tmc_server_t* server, uint32_t pane_id, bool active, void* user);
    void (*on_selection)(tmc_server_t* server, uint32_t pane_id, 
                        uint32_t start_row, uint32_t start_col,
                        uint32_t end_row, uint32_t end_col, void* user);
    
    // Process and system events
    void (*on_process_exit)(tmc_server_t* server, uint32_t pane_id, int status, void* user);
    void (*on_bell)(tmc_server_t* server, uint32_t pane_id, void* user);
    
    // Messages and notifications
    void (*on_message)(tmc_server_t* server, int level, const char* msg, void* user);
    void (*on_overflow)(tmc_client_t* client, uint32_t pane_id, 
                       uint32_t dropped_frames, uint32_t buffer_pct, void* user);
} tmc_ui_vtable_t;

/* ============= Server Configuration ============= */

typedef struct {
    uint32_t size;              // MUST be first field
    uint32_t version;
    
    // Basic configuration
    const char* default_shell;
    const char* default_command;
    const char* default_cwd;
    const char* socket_path;
    
    // Limits and performance
    int32_t history_limit;
    int32_t max_panes;
    uint32_t frame_buffer_size;
    uint32_t max_fps;
    
    // Features
    bool enable_mouse;
    bool enable_clipboard;
    bool enable_osc52;
    bool ui_draws_borders;
    
    // Backpressure configuration
    uint32_t buffer_high_watermark;  // Percentage (0-100)
    uint32_t buffer_low_watermark;
    uint32_t drop_threshold;
} tmc_server_config_t;

/* ============= Enhanced Command Interface ============= */

// Structured command for type safety
typedef struct {
    const char* name;
    const char* const* argv;
    int argc;
} tmc_command_t;

// Async command callback
typedef void (*tmc_command_callback_t)(tmc_err_t err, const char* json_result, void* user);

/* ============= Input Events ============= */

typedef enum {
    TMC_MOD_SHIFT = 1 << 0,
    TMC_MOD_ALT   = 1 << 1,
    TMC_MOD_CTRL  = 1 << 2,
    TMC_MOD_SUPER = 1 << 3,
    TMC_MOD_HYPER = 1 << 4,
    TMC_MOD_META  = 1 << 5,
} tmc_mod_flags_t;

typedef struct {
    uint32_t size;              // MUST be first field
    uint32_t codepoint;
    uint32_t mods;
    bool is_repeat;
    bool is_keypad;
} tmc_key_event_t;

typedef struct {
    uint32_t size;              // MUST be first field
    uint32_t pane_id;
    uint16_t x, y;              // Cell coordinates
    uint8_t button;             // 0=move, 1=left, 2=middle, 3=right, 4=wheel-up, 5=wheel-down
    uint8_t clicks;             // Single, double, triple
    bool pressed;
    uint32_t mods;
} tmc_mouse_event_t;

/* ============= Metrics and Observability ============= */

typedef struct {
    uint32_t size;              // MUST be first field
    
    // Frame statistics
    uint64_t frames_emitted;
    uint64_t frames_dropped;
    uint64_t frames_coalesced;
    
    // Throughput
    double input_rate_mbps;
    double render_rate_mbps;
    
    // Latency
    uint64_t avg_frame_latency_ns;
    uint64_t p99_frame_latency_ns;
    
    // Buffer utilization
    uint32_t buffer_occupancy_pct;
    uint32_t peak_occupancy_pct;
    
    // Cell updates
    uint64_t cells_updated;
    uint64_t spans_merged;
} tmc_metrics_t;

/* ============= Logging ============= */

typedef enum {
    TMC_LOG_TRACE,
    TMC_LOG_DEBUG,
    TMC_LOG_INFO,
    TMC_LOG_WARN,
    TMC_LOG_ERROR,
    TMC_LOG_FATAL,
} tmc_log_level_t;

typedef enum {
    TMC_LOG_CAT_CORE    = 1 << 0,
    TMC_LOG_CAT_IO      = 1 << 1,
    TMC_LOG_CAT_GRID    = 1 << 2,
    TMC_LOG_CAT_CMD     = 1 << 3,
    TMC_LOG_CAT_LOOP    = 1 << 4,
    TMC_LOG_CAT_PROC    = 1 << 5,
} tmc_log_category_t;

typedef void (*tmc_log_callback_t)(tmc_log_level_t level, 
                                   tmc_log_category_t category,
                                   const char* msg, 
                                   void* user);

/* ============= Public API ============= */

// Version and capability queries
TMC_API const char* tmc_version_string(void);
TMC_API uint32_t tmc_get_abi_version(void);
TMC_API uint32_t tmc_get_abi_minimum(void);
TMC_API tmc_err_t tmc_negotiate_capabilities(tmc_server_t* server, tmc_capabilities_t* caps);

// Server lifecycle
TMC_API tmc_err_t tmc_server_new(tmc_server_t** server,
                                 const tmc_server_config_t* config,
                                 const tmc_loop_vtable_t* loop,
                                 tmc_io_t* loop_instance,
                                 const tmc_ui_vtable_t* ui,
                                 void* ui_user);
TMC_API void tmc_server_free(tmc_server_t* server);

// Client management
TMC_API tmc_err_t tmc_client_attach(tmc_client_t** client,
                                    tmc_server_t* server,
                                    uint32_t session_id,
                                    uint32_t rows, 
                                    uint32_t cols);
TMC_API void tmc_client_detach(tmc_client_t* client);
TMC_API tmc_err_t tmc_client_resize(tmc_client_t* client, uint32_t rows, uint32_t cols);

// Command execution
TMC_API tmc_err_t tmc_command(tmc_server_t* server, const char* fmt, ...);
TMC_API tmc_err_t tmc_command_structured(tmc_server_t* server, const tmc_command_t* cmd);
TMC_API tmc_err_t tmc_command_async(tmc_server_t* server, 
                                    const tmc_command_t* cmd,
                                    tmc_command_callback_t callback,
                                    void* user);

// Input handling
TMC_API tmc_err_t tmc_send_keys(tmc_client_t* client, 
                                const tmc_key_event_t* keys, 
                                size_t count);
TMC_API tmc_err_t tmc_send_text(tmc_client_t* client, 
                                const char* utf8, 
                                size_t len);
TMC_API tmc_err_t tmc_send_mouse(tmc_client_t* client, 
                                 const tmc_mouse_event_t* event);

// Pane operations
typedef struct {
    uint32_t size;              // MUST be first field
    uint32_t rows, cols;
    tmc_cell_t* cells;          // Caller owns memory
    uint64_t generation;        // Snapshot generation number
} tmc_snapshot_t;

TMC_API tmc_err_t tmc_pane_snapshot(tmc_server_t* server, 
                                    uint32_t pane_id,
                                    tmc_snapshot_t* snapshot);
TMC_API void tmc_snapshot_release(tmc_snapshot_t* snapshot);

// Options management
TMC_API tmc_err_t tmc_set_option(tmc_server_t* server, 
                                 const char* scope,
                                 uint32_t target_id,
                                 const char* name, 
                                 const char* value);
TMC_API tmc_err_t tmc_get_option(tmc_server_t* server,
                                 const char* scope,
                                 uint32_t target_id,
                                 const char* name,
                                 char* buffer,
                                 size_t buffer_size);

// Enumeration
typedef struct {
    uint32_t id;
    const char* name;
    uint32_t parent_id;
} tmc_object_info_t;

TMC_API tmc_err_t tmc_list_sessions(tmc_server_t* server,
                                    tmc_object_info_t* buffer,
                                    size_t* count);
TMC_API tmc_err_t tmc_list_windows(tmc_server_t* server,
                                   uint32_t session_id,
                                   tmc_object_info_t* buffer,
                                   size_t* count);
TMC_API tmc_err_t tmc_list_panes(tmc_server_t* server,
                                 uint32_t window_id,
                                 tmc_object_info_t* buffer,
                                 size_t* count);

// Logging and debugging
TMC_API void tmc_set_log_callback(tmc_server_t* server,
                                  tmc_log_callback_t callback,
                                  void* user);
TMC_API void tmc_set_log_level(tmc_server_t* server, 
                               tmc_log_level_t level);
TMC_API void tmc_set_log_categories(tmc_server_t* server, 
                                    uint32_t category_mask);

// Metrics
TMC_API tmc_err_t tmc_get_metrics(tmc_server_t* server, 
                                  tmc_metrics_t* metrics);
TMC_API tmc_err_t tmc_reset_metrics(tmc_server_t* server);

// Error handling
TMC_API const tmc_error_info_t* tmc_get_last_error(tmc_server_t* server);
TMC_API const char* tmc_error_string(tmc_err_t err);

// Recording and replay (for debugging)
TMC_API tmc_err_t tmc_start_recording(tmc_server_t* server, const char* filename);
TMC_API tmc_err_t tmc_stop_recording(tmc_server_t* server);
TMC_API tmc_err_t tmc_replay_recording(tmc_server_t* server, const char* filename);

// Memory management hints
TMC_API void tmc_hint_memory_pressure(tmc_server_t* server, uint32_t pressure_pct);
TMC_API void tmc_compact_memory(tmc_server_t* server);

#ifdef __cplusplus
}
#endif

// Static assertions for ABI stability
_Static_assert(sizeof(tmc_cell_t) == 16, "tmc_cell_t size changed");
_Static_assert(sizeof(tmc_span_t) == 24, "tmc_span_t size changed");
_Static_assert(offsetof(tmc_frame_t, size) == 0, "size must be first field");
_Static_assert(offsetof(tmc_capabilities_t, size) == 0, "size must be first field");

#endif /* LIBTMUXCORE_H */