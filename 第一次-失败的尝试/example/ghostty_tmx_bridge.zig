// ghostty_tmx_bridge.zig — minimal FFI glue to libtmuxcore
const std = @import("std");

// ===== libtmuxcore ABI mirroring (keep in sync with libtmuxcore.h) =====

pub const TMCCell = extern struct {
    ch: u32,
    fg_rgb: u32,
    bg_rgb: u32,
    attrs: u16,
    width: u8,
    _pad: u8,
};

pub const TMCSpan = extern struct {
    pane_id: u32,
    y: u32,
    x_start: u32,
    len: u32,
    cells: [*]const TMCCell,
};

pub const TMCGridUpdate = extern struct {
    pane_id: u32,
    rows: u32,
    cols: u32,
    span_count: u32,
    spans: [*]const TMCSpan,
    full: c_int,
};

pub const TMCCopyModeState = extern struct {
    pane_id: u32,
    active: c_int,
    cursor_x: u32,
    cursor_y: u32,
    has_sel: c_int,
    sx0: u32, sy0: u32, sx1: u32, sy1: u32,
};

pub const TMCLayoutNode = extern struct {
    ty: c_int, // 0=v, 1=h, 2=leaf
    pane_id: u32,
    x: u32, y: u32, w: u32, h: u32, // in cells
};

pub const TMCLayoutUpdate = extern struct {
    window_id: u32,
    node_count: u32,
    nodes: [*]const TMCLayoutNode,
};

pub const TmcServer = opaque {};
pub const TmcClient = opaque {};

pub extern fn tmc_version() [*]const u8;
pub extern fn tmc_api_version() u32;

pub const TMCUiVTable = extern struct {
    on_grid:   ?*const fn (*TmcServer, *const TMCGridUpdate, ?*anyopaque) callconv(.C) void,
    on_layout: ?*const fn (*TmcServer, *const TMCLayoutUpdate, ?*anyopaque) callconv(.C) void,
    on_title:  ?*const fn (*TmcServer, u32, [*]const u8, ?*anyopaque) callconv(.C) void,
    on_renamed:?*const fn (*TmcServer, u32, [*]const u8, ?*anyopaque) callconv(.C) void,
    on_session:?*const fn (*TmcServer, u32, [*]const u8, c_int, ?*anyopaque) callconv(.C) void,
    on_copy_mode: ?*const fn (*TmcServer, *const TMCCopyModeState, ?*anyopaque) callconv(.C) void,
    on_process_exit: ?*const fn (*TmcServer, u32, c_int, ?*anyopaque) callconv(.C) void,
    on_message: ?*const fn (*TmcServer, [*]const u8, ?*anyopaque) callconv(.C) void,
};

pub const TMCLoopVTable = extern struct {
    add_fd:   ?*const fn (?*anyopaque, c_int, c_int, ?*const fn (c_int, c_int, ?*anyopaque) callconv(.C) void, ?*anyopaque) callconv(.C) c_int,
    mod_fd:   ?*const fn (?*anyopaque, c_int, c_int) callconv(.C) c_int,
    del_fd:   ?*const fn (?*anyopaque, c_int) callconv(.C) void,

    add_timer:?*const fn (?*anyopaque, u64, c_int, ?*const fn (?*anyopaque) callconv(.C) void, ?*anyopaque) callconv(.C) c_int,
    del_timer:?*const fn (?*anyopaque, c_int) callconv(.C) void,

    post:     ?*const fn (?*anyopaque, ?*const fn (?*anyopaque) callconv(.C) void, ?*anyopaque) callconv(.C) void,
};

pub const TmcServerConfig = extern struct {
    default_shell:   ?[*:0]const u8,
    default_command: ?[*:0]const u8,
    default_cwd:     ?[*:0]const u8,
    socket_path:     ?[*:0]const u8,
    history_limit:   c_int,
    mouse:           c_int,
};

pub extern fn tmc_server_new(cfg: *const TmcServerConfig, loop_vt: *const TMCLoopVTable, loop_instance: ?*anyopaque, ui_vt: *const TMCUiVTable, ui_user: ?*anyopaque) *TmcServer;
pub extern fn tmc_server_free(s: *TmcServer) void;

pub extern fn tmc_client_attach(s: *TmcServer, session_id: u32, rows: u32, cols: u32) *TmcClient;
pub extern fn tmc_client_detach(c: *TmcClient) void;
pub extern fn tmc_client_resize(c: *TmcClient, rows: u32, cols: u32) void;

// (optional) more externs if needed in your bridge:
// pub extern fn tmc_send_keys(c: *TmcClient, keys: [*]const TmcKeyEvent, count: usize) c_int;
// pub extern fn tmc_send_text(c: *TmcClient, utf8: [*]const u8, len: usize) c_int;
// pub extern fn tmc_mouse(c: *TmcClient, ev: *const TmcMouseEvent) c_int;


// ===== Host-side example glue =====
//
// This shows how Ghostty might wire libtmuxcore into its event loop and renderer.
// Replace the TODOs with your actual buffer, layout, and render calls.

pub const GhosttyMux = struct {
    allocator: std.mem.Allocator,
    server: *TmcServer,
    client: *TmcClient,

    pub fn init(alloc: std.mem.Allocator) !GhosttyMux {
        // Provide loop + UI vtables (TODO: connect to Ghostty's actual loop and renderer)
        var loop_vt = TMCLoopVTable{}; // Fill with your fd/timer/post shims if you embed the loop
        var ui_vt = TMCUiVTable{
            .on_grid = onGrid,
            .on_layout = onLayout,
            .on_title = onTitle,
            .on_renamed = null,
            .on_session = null,
            .on_copy_mode = onCopyMode,
            .on_process_exit = null,
            .on_message = onMessage,
        };

        var cfg = TmcServerConfig{
            .default_shell = null,
            .default_command = null,
            .default_cwd = null,
            .socket_path = null,
            .history_limit = -1,
            .mouse = 1,
        };

        const s = tmc_server_new(&cfg, &loop_vt, null, &ui_vt, null);
        if (s == null) return error.InitFailed;

        const c = tmc_client_attach(s, 0, 80, 120);
        if (c == null) {
            tmc_server_free(s);
            return error.InitFailed;
        }

        return GhosttyMux{
            .allocator = alloc,
            .server = s,
            .client = c,
        };
    }

    pub fn deinit(self: *GhosttyMux) void {
        if (self.client) |cl| {
            tmc_client_detach(cl);
        }
        if (self.server) |sv| {
            tmc_server_free(sv);
        }
        self.* = undefined;
    }

    // ====== Callbacks from libtmuxcore into Ghostty ======

    extern fn onGrid(s: *TmcServer, upd: *const TMCGridUpdate, user: ?*anyopaque) callconv(.C) void {
        _ = s; _ = user;
        // TODO:
        // 1) Lookup pane by upd.pane_id -> your internal pane buffer
        // 2) For each span in upd.spans[0..span_count):
        //      - copy span.cells into your terminal cell buffer at (x_start..x_start+len, y)
        //      - mark dirty rect
        // 3) If upd.full != 0, treat as full snapshot
        // 4) Request a render on next vsync
    }

    extern fn onLayout(s: *TmcServer, lay: *const TMCLayoutUpdate, user: ?*anyopaque) callconv(.C) void {
        _ = s; _ = user;
        // TODO:
        // - Rebuild split tree for window lay.window_id using lay.nodes[0..node_count)
        // - Each node has type (v/h/leaf), geometry in cells, and pane_id for leaves
        // - Let Ghostty draw borders/labels; tmux core provides pure geometry
    }

    extern fn onTitle(s: *TmcServer, pane_id: u32, title: [*]const u8, user: ?*anyopaque) callconv(.C) void {
        _ = s; _ = user; _ = pane_id; _ = title;
        // TODO: update pane/tab title in Ghostty UI
    }

    extern fn onCopyMode(s: *TmcServer, st: *const TMCCopyModeState, user: ?*anyopaque) callconv(.C) void {
        _ = s; _ = user;
        // TODO:
        // - If st.active != 0, render selection box using (sx0,sy0)..(sx1,sy1)
        // - Show cursor at (cursor_x, cursor_y)
        // - Optionally route mouse/keys to tmux copy-mode until exit
    }

    extern fn onMessage(s: *TmcServer, msg: [*]const u8, user: ?*anyopaque) callconv(.C) void {
        _ = s; _ = user; _ = msg;
        // TODO: surface server messages to Ghostty's status/log UI
    }
};