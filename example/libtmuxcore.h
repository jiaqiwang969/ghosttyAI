// libtmuxcore.h — Embedding tmux core into GUI apps (Ghostty)
// SPDX-License-Identifier: ISC (same as tmux core files it wraps)
// Minimal C99 stable ABI proposal
#ifndef LIBTMUXCORE_H
#define LIBTMUXCORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#define TMC_API_VERSION 0x00010000u /* 1.0.0 */

#if defined(_WIN32) && defined(TMC_BUILD_SHARED)
#  ifdef TMC_EXPORTS
#    define TMC_API __declspec(dllexport)
#  else
#    define TMC_API __declspec(dllimport)
#  endif
#else
#  define TMC_API
#endif

/* ----------- Opaque handles ----------- */
typedef struct tmc_server      tmc_server;
typedef struct tmc_client      tmc_client;   /* a GUI attachment (Ghostty view) */
typedef struct tmc_session     tmc_session;
typedef struct tmc_window      tmc_window;
typedef struct tmc_pane        tmc_pane;

/* ----------- Error codes ----------- */
typedef enum {
    TMC_OK = 0,
    TMC_ERR_UNKNOWN = -1,
    TMC_ERR_INVALID = -2,
    TMC_ERR_NOMEM = -3,
    TMC_ERR_BUSY = -4,
    TMC_ERR_NOTFOUND = -5,
    TMC_ERR_PERM = -6,
} tmc_err;

/* ----------- Event-loop bridge (host provided) ----------- */
typedef struct tmc_io tmc_io;
typedef void (*tmc_io_fd_cb)(int fd, int events, void* user); /* events: bitmask 1=read 2=write 4=error */
typedef void (*tmc_io_timer_cb)(void* user);

typedef struct {
    /* Add an fd watcher; return a handle (>=0) or negative error. */
    int   (*add_fd)(tmc_io* io, int fd, int events, tmc_io_fd_cb cb, void* user);
    int   (*mod_fd)(tmc_io* io, int handle, int events);
    void  (*del_fd)(tmc_io* io, int handle);

    /* One-shot or repeating timers; if interval_ms==0, treat as one-shot. Returns handle */
    int   (*add_timer)(tmc_io* io, uint64_t interval_ms, int repeat, tmc_io_timer_cb cb, void* user);
    void  (*del_timer)(tmc_io* io, int handle);

    /* Post a function into host loop thread (optional, may be NULL) */
    void  (*post)(tmc_io* io, void (*fn)(void*), void* user);
} tmc_loop_vtable;

/* ----------- UI callbacks (core -> host) ----------- */

typedef enum {
    TMC_ATTR_BOLD   = 1<<0,
    TMC_ATTR_ITALIC = 1<<1,
    TMC_ATTR_UNDER  = 1<<2,
    TMC_ATTR_DIM    = 1<<3,
    TMC_ATTR_REVERSE= 1<<4,
    TMC_ATTR_BLINK  = 1<<5,
    TMC_ATTR_STRIKE = 1<<6,
} tmc_attr;

typedef struct {
    uint32_t ch;       /* Unicode codepoint, U+FFFD for invalid. */
    uint32_t fg_rgb;   /* 0xRRGGBB, or 0xFFFFFFFF for default */
    uint32_t bg_rgb;   /* 0xRRGGBB, or 0xFFFFFFFF for default */
    uint16_t attrs;    /* tmc_attr bitset */
    uint8_t  width;    /* 1 or 2 */
    uint8_t  _pad;
} tmc_cell;

typedef struct {
    uint32_t pane_id;
    uint32_t y;            /* row */
    uint32_t x_start;      /* column start */
    uint32_t len;          /* number of cells */
    const tmc_cell* cells; /* view into core-owned ring buffer; valid until next callback returns */
} tmc_span;

typedef struct {
    uint32_t pane_id;
    uint32_t rows, cols;
    uint32_t span_count;
    const tmc_span* spans; /* spans sorted by row then column */
    int      full;         /* nonzero => full snapshot instead of incremental */
} tmc_grid_update;

typedef struct {
    int type;            /* 0=vertical,1=horizontal,2=leaf */
    uint32_t pane_id;    /* valid when type==leaf */
    uint32_t x, y, w, h; /* in cells */
} tmc_layout_node;

typedef struct {
    uint32_t window_id;
    uint32_t node_count;
    const tmc_layout_node* nodes;
} tmc_layout_update;

typedef struct {
    uint32_t pane_id;
    int      active; /* 0/1 */
    uint32_t cursor_x, cursor_y;
    /* selection (if any), inclusive [x0,y0]..[x1,y1] in pane coordinates */
    int      has_sel;
    uint32_t sx0, sy0, sx1, sy1;
} tmc_copy_mode_state;

typedef struct {
    void (*on_grid)(tmc_server*, const tmc_grid_update*, void* user);
    void (*on_layout)(tmc_server*, const tmc_layout_update*, void* user);
    void (*on_title)(tmc_server*, uint32_t pane_id, const char* title, void* user);
    void (*on_renamed)(tmc_server*, uint32_t window_id, const char* name, void* user);
    void (*on_session)(tmc_server*, uint32_t session_id, const char* name, int created, void* user);
    void (*on_copy_mode)(tmc_server*, const tmc_copy_mode_state*, void* user);
    void (*on_process_exit)(tmc_server*, uint32_t pane_id, int status, void* user);
    void (*on_message)(tmc_server*, const char* msg, void* user);
} tmc_ui_vtable;

/* ----------- Server configuration ----------- */
typedef struct {
    const char* default_shell;   /* NULL => inherit */
    const char* default_command; /* NULL => login shell */
    const char* default_cwd;     /* NULL => inherit */
    const char* socket_path;     /* NULL => anonymous in-proc */
    int history_limit;           /* -1 => tmux default */
    int mouse;                   /* 0/1 */
} tmc_server_config;

/* ----------- Key and mouse events (host -> core) ----------- */
typedef enum {
    TMC_MOD_SHIFT = 1<<0,
    TMC_MOD_ALT   = 1<<1,
    TMC_MOD_CTRL  = 1<<2,
    TMC_MOD_SUPER = 1<<3,
} tmc_mods;

typedef enum {
    TMC_KEY_ASCII = 0,  /* use .ch and optional .mods */
    TMC_KEY_FUNC,       /* F1..F24, use .keycode */
    TMC_KEY_CODEPOINT,  /* Unicode codepoint (no text composition) */
    TMC_KEY_SPECIAL,    /* arrows, home, end, etc. */
} tmc_key_kind;

typedef enum {
    TMC_SPECIAL_UP=1, TMC_SPECIAL_DOWN, TMC_SPECIAL_LEFT, TMC_SPECIAL_RIGHT,
    TMC_SPECIAL_HOME, TMC_SPECIAL_END, TMC_SPECIAL_PGUP, TMC_SPECIAL_PGDN,
    TMC_SPECIAL_INS, TMC_SPECIAL_DEL, TMC_SPECIAL_BACKTAB
} tmc_special;

typedef struct {
    tmc_key_kind kind;
    uint32_t mods;
    union {
        struct { char ch; } ascii;
        struct { uint32_t codepoint; } cp;
        struct { uint8_t keycode; } func; /* 1..24 */
        struct { uint16_t special; } sp;
    } u;
} tmc_key_event;

typedef struct {
    uint32_t pane_id;
    uint16_t x, y;
    uint8_t  button; /* 0=move,1=left,2=middle,3=right,4=wheel-up,5=wheel-down */
    uint8_t  pressed; /* 0/1 for buttons; ignored for wheel */
    uint32_t mods;
} tmc_mouse_event;

/* ----------- Public API ----------- */
TMC_API const char* tmc_version(void); /* returns tmux version string */
TMC_API uint32_t    tmc_api_version(void); /* returns TMC_API_VERSION */

TMC_API tmc_server* tmc_server_new(const tmc_server_config* cfg,
                                   const tmc_loop_vtable* loop,
                                   tmc_io* loop_instance,
                                   const tmc_ui_vtable* ui,
                                   void* ui_user);

TMC_API void        tmc_server_free(tmc_server* s);

/* Create a GUI attachment (maps to tmux client internally) */
TMC_API tmc_client* tmc_client_attach(tmc_server* s, uint32_t session_id,
                                      uint32_t rows, uint32_t cols);
TMC_API void        tmc_client_detach(tmc_client* c);
TMC_API void        tmc_client_resize(tmc_client* c, uint32_t rows, uint32_t cols);

/* Run a tmux command like "split-window -h" against a target */
TMC_API tmc_err     tmc_command(tmc_server* s, const char* fmt, ...);

/* Input to focused pane of a client (or by pane id) */
TMC_API tmc_err     tmc_send_keys(tmc_client* c, const tmc_key_event* keys, size_t count);
TMC_API tmc_err     tmc_send_text(tmc_client* c, const char* utf8, size_t len);
TMC_API tmc_err     tmc_mouse(tmc_client* c, const tmc_mouse_event* ev);

/* Pane snapshot and history */
typedef struct {
    uint32_t rows, cols;
    const tmc_cell* /*nullable*/ cells; /* rows*cols; may be NULL on error */
} tmc_snapshot;

TMC_API tmc_snapshot tmc_pane_snapshot(tmc_server* s, uint32_t pane_id);
TMC_API void         tmc_snapshot_release(tmc_server* s, tmc_snapshot* snap);

/* Options (mirrors tmux set-option/show-options semantics) */
TMC_API tmc_err     tmc_set_option(tmc_server* s, const char* scope /*global/session/window*/, uint32_t id_or_0, const char* name, const char* value);
TMC_API int         tmc_get_option(tmc_server* s, const char* scope, uint32_t id_or_0, const char* name, /*out*/ char* buf, size_t buflen);

/* IDs and enumeration */
typedef struct { uint32_t id; const char* name; } tmc_id_name;
TMC_API size_t      tmc_list_sessions(tmc_server* s, tmc_id_name* out, size_t cap);
TMC_API size_t      tmc_list_windows (tmc_server* s, uint32_t session_id, tmc_id_name* out, size_t cap);
TMC_API size_t      tmc_list_panes   (tmc_server* s, uint32_t window_id,  tmc_id_name* out, size_t cap);

/* Logging hook (optional) */
typedef void (*tmc_log_fn)(int level, const char* msg, void* user);
TMC_API void        tmc_set_logger(tmc_server* s, tmc_log_fn fn, void* user);

#ifdef __cplusplus
}
#endif
#endif /* LIBTMUXCORE_H */