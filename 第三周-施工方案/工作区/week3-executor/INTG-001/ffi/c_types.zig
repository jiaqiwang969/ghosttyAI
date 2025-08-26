// c_types.zig - Zig FFI Type Mappings for tmux/Ghostty Integration
// Purpose: Zero-copy, type-safe FFI bridge between C and Zig
// Author: INTG-001 (zig-ghostty-integration)
// Date: 2025-08-25
// Version: 1.0.0
// Performance Target: <100ns FFI overhead per call

const std = @import("std");
const c = @cImport({
    @cInclude("stdint.h");
    @cInclude("stdbool.h");
    @cInclude("stddef.h");
});

// ============================================================================
// ABI Version Control
// ============================================================================

pub const UI_BACKEND_ABI_VERSION_MAJOR: u32 = 1;
pub const UI_BACKEND_ABI_VERSION_MINOR: u32 = 0;
pub const UI_BACKEND_ABI_VERSION_PATCH: u32 = 0;

pub fn getAbiVersion() u32 {
    return (UI_BACKEND_ABI_VERSION_MAJOR << 16) |
           (UI_BACKEND_ABI_VERSION_MINOR << 8) |
           UI_BACKEND_ABI_VERSION_PATCH;
}

// ============================================================================
// Color Constants (matching ui_backend.h)
// ============================================================================

pub const UI_COLOR_DEFAULT: u32 = 0xFFFFFFFE;
pub const UI_COLOR_INVALID: u32 = 0xFFFFFFFF;

// ============================================================================
// Enum Mappings
// ============================================================================

// Cell attributes - bitfield compatible with C
pub const UiAttrFlags = packed struct {
    bold: bool = false,
    italic: bool = false,
    underline: bool = false,
    dim: bool = false,
    reverse: bool = false,
    blink: bool = false,
    strike: bool = false,
    double_ul: bool = false,
    curly_ul: bool = false,
    dotted_ul: bool = false,
    dashed_ul: bool = false,
    _padding: u5 = 0,  // Ensure 16-bit alignment
    
    pub fn toU16(self: UiAttrFlags) u16 {
        return @bitCast(u16, self);
    }
    
    pub fn fromU16(value: u16) UiAttrFlags {
        return @bitCast(UiAttrFlags, value);
    }
};

// Frame flags
pub const UiFrameFlags = packed struct {
    complete: bool = false,
    partial: bool = false,
    urgent: bool = false,
    dropped: bool = false,
    snapshot: bool = false,
    cursor: bool = false,
    _padding: u26 = 0,  // 32-bit alignment
    
    pub fn toU32(self: UiFrameFlags) u32 {
        return @bitCast(u32, self);
    }
    
    pub fn fromU32(value: u32) UiFrameFlags {
        return @bitCast(UiFrameFlags, value);
    }
};

// Backend capabilities
pub const UiCapFlags = packed struct {
    frame_batch: bool = false,
    utf8_lines: bool = false,
    color_24bit: bool = false,
    borders_by_ui: bool = false,
    cursor_shapes: bool = false,
    underline_styles: bool = false,
    sixel: bool = false,
    synchronized: bool = false,
    _padding: u24 = 0,  // 32-bit alignment
    
    pub fn toU32(self: UiCapFlags) u32 {
        return @bitCast(u32, self);
    }
    
    pub fn fromU32(value: u32) UiCapFlags {
        return @bitCast(UiCapFlags, value);
    }
};

// Backend mode
pub const BackendMode = enum(c_int) {
    TTY = 0,
    UI = 1,
    HYBRID = 2,
};

// Backend type
pub const UiBackendType = enum(c_int) {
    TTY = 0,
    GHOSTTY = 1,
    TEST = 2,
};

// Command flags
pub const BackendCmdFlags = packed struct {
    batchable: bool = false,
    urgent: bool = false,
    stateful: bool = false,
    visual: bool = false,
    control: bool = false,
    _padding: u27 = 0,  // 32-bit alignment
};

// Error codes
pub const UiBackendError = enum(c_int) {
    OK = 0,
    INVALID_TYPE = -1,
    NOMEM = -2,
    ALREADY_REGISTERED = -3,
    NOT_REGISTERED = -4,
    INVALID_CAPS = -5,
    FRAME_OVERFLOW = -6,
};

pub const RouterError = enum(c_int) {
    OK = 0,
    NO_BACKEND = -1,
    INVALID_MODE = -2,
    ALREADY_REGISTERED = -3,
    COMMAND_NOT_FOUND = -4,
    BACKEND_FAILED = -5,
};

// ============================================================================
// Core Data Structures (C-compatible layout)
// ============================================================================

// Single cell representation - matches ui_cell_t exactly
pub const UiCell = extern struct {
    codepoint: u32,
    fg_rgb: u32,
    bg_rgb: u32,
    attrs: u16,
    width: u8,
    cluster_cont: u8,
    
    // Zero-copy conversion
    pub fn fromC(c_ptr: *const c.ui_cell_t) *const UiCell {
        return @ptrCast(*const UiCell, c_ptr);
    }
    
    pub fn toC(self: *const UiCell) *const c.ui_cell_t {
        return @ptrCast(*const c.ui_cell_t, self);
    }
};

// Span of cells - matches ui_span_t
pub const UiSpan = extern struct {
    row: u32,
    col_start: u32,
    col_end: u32,
    cells: [*]const UiCell,  // Pointer to array
    flags: u32,
    
    // Helper methods
    pub fn getCellCount(self: *const UiSpan) u32 {
        return self.col_end - self.col_start;
    }
    
    pub fn getCellSlice(self: *const UiSpan) []const UiCell {
        const count = self.getCellCount();
        return self.cells[0..count];
    }
};

// Frame structure - matches ui_frame_t
pub const UiFrame = extern struct {
    size: u32,               // MUST be first field (ABI stability)
    frame_seq: u64,
    timestamp_ns: u64,
    pane_id: u32,
    span_count: u32,
    spans: [*]const UiSpan,
    flags: u32,              // UiFrameFlags as u32
    
    // Statistics
    updates_batched: u32,
    cells_modified: u32,
    frames_dropped: u32,
    
    pub fn getSpanSlice(self: *const UiFrame) []const UiSpan {
        return self.spans[0..self.span_count];
    }
    
    pub fn getFlags(self: *const UiFrame) UiFrameFlags {
        return UiFrameFlags.fromU32(self.flags);
    }
};

// Capabilities structure
pub const UiCapabilities = extern struct {
    size: u32,               // MUST be first field
    version: u32,
    supported: u32,          // UiCapFlags as u32
    
    // Performance hints
    max_fps: u32,
    optimal_batch_size: u32,
    max_dirty_rects: u32,
    
    pub fn getFlags(self: *const UiCapabilities) UiCapFlags {
        return UiCapFlags.fromU32(self.supported);
    }
};

// Frame aggregator
pub const FrameAggregator = extern struct {
    size: u32,               // MUST be first field
    
    // Configuration
    frame_interval_ns: u64,
    max_latency_ns: u64,
    max_spans: u32,
    
    // State
    last_frame_time_ns: u64,
    frame_seq_next: u64,
    
    // Accumulation buffer (opaque pointers)
    pending_spans: ?*UiSpan,
    pending_count: u32,
    pending_capacity: u32,
    
    // Dirty region
    dirty_min_row: u32,
    dirty_max_row: u32,
    dirty_min_col: u32,
    dirty_max_col: u32,
    full_refresh_needed: bool,
    
    // Statistics
    frames_emitted: u64,
    spans_merged: u64,
    cells_updated: u64,
    frames_dropped: u64,
};

// Hook statistics
pub const TtyHookStats = extern struct {
    call_count: [22]u64,
    total_calls: u64,
    intercepted_calls: u64,
    fallback_calls: u64,
};

// Router statistics
pub const BackendRouterStats = extern struct {
    size: u32,               // MUST be first field
    
    // Command routing counts
    commands_routed: u64,
    commands_to_tty: u64,
    commands_to_ui: u64,
    commands_dropped: u64,
    
    // Performance metrics
    total_routing_time_ns: u64,
    min_routing_time_ns: u64,
    max_routing_time_ns: u64,
    avg_routing_time_ns: u64,
    
    // Error counts
    routing_errors: u64,
    backend_errors: u64,
};

// Hybrid mode configuration
pub const HybridModeConfig = extern struct {
    prefer_ui: bool,
    sync_output: bool,
    ui_delay_ms: u32,
};

// ============================================================================
// Opaque Pointer Types (forward declarations)
// ============================================================================

// These are opaque C types we only handle as pointers
pub const Tty = opaque {};
pub const TtyCtx = opaque {};
pub const GridCell = opaque {};
pub const UiBackend = opaque {};
pub const BackendRouter = opaque {};

// ============================================================================
// Function Pointer Types (Callbacks)
// ============================================================================

// Command function pointers - zero overhead wrappers
pub const TtyCmdFn = ?fn(*Tty, *const TtyCtx) callconv(.C) void;
pub const UiCmdFn = ?fn(*UiBackend, *const TtyCtx) callconv(.C) void;

// Backend operation callbacks (vtable)
pub const UiBackendOps = extern struct {
    size: u32,               // MUST be first field
    version: u32,
    
    // 22 command callbacks - all optional
    cmd_cell: UiCmdFn,
    cmd_cells: UiCmdFn,
    cmd_insertcharacter: UiCmdFn,
    cmd_deletecharacter: UiCmdFn,
    cmd_clearcharacter: UiCmdFn,
    
    cmd_insertline: UiCmdFn,
    cmd_deleteline: UiCmdFn,
    cmd_clearline: UiCmdFn,
    cmd_clearendofline: UiCmdFn,
    cmd_clearstartofline: UiCmdFn,
    
    cmd_clearscreen: UiCmdFn,
    cmd_clearendofscreen: UiCmdFn,
    cmd_clearstartofscreen: UiCmdFn,
    cmd_alignmenttest: UiCmdFn,
    
    cmd_reverseindex: UiCmdFn,
    cmd_linefeed: UiCmdFn,
    cmd_scrollup: UiCmdFn,
    cmd_scrolldown: UiCmdFn,
    
    cmd_setselection: UiCmdFn,
    cmd_rawstring: UiCmdFn,
    cmd_sixelimage: UiCmdFn,
    cmd_syncstart: UiCmdFn,
};

// Host callbacks
pub const OnFrameFn = ?fn(*const UiFrame, ?*anyopaque) callconv(.C) void;
pub const OnBellFn = ?fn(u32, ?*anyopaque) callconv(.C) void;
pub const OnTitleFn = ?fn(u32, [*:0]const u8, ?*anyopaque) callconv(.C) void;
pub const OnOverflowFn = ?fn(u32, ?*anyopaque) callconv(.C) void;

// Performance monitoring callbacks
pub const OnMetricFn = ?fn([*:0]const u8, u64, ?*anyopaque) callconv(.C) void;
pub const OnErrorFn = ?fn([*:0]const u8, ?*anyopaque) callconv(.C) void;

// Backend structure
pub const UiBackendStruct = extern struct {
    size: u32,               // MUST be first field
    version: u32,
    type: UiBackendType,
    
    // Operations table
    ops: *const UiBackendOps,
    
    // Frame aggregation
    aggregator: ?*FrameAggregator,
    
    // Capabilities
    capabilities: UiCapabilities,
    
    // Callbacks to host
    on_frame: OnFrameFn,
    on_bell: OnBellFn,
    on_title: OnTitleFn,
    on_overflow: OnOverflowFn,
    
    // User data for callbacks
    user_data: ?*anyopaque,
    
    // Private implementation data
    priv: ?*anyopaque,
};

// Command mapping entry
pub const BackendCmdMapping = extern struct {
    name: [*:0]const u8,
    tty_fn: TtyCmdFn,
    ui_fn: UiCmdFn,
    flags: u32,
};

// Router structure
pub const BackendRouterStruct = extern struct {
    size: u32,               // MUST be first field
    version: u32,
    
    // Configuration
    mode: BackendMode,
    enabled: bool,
    
    // Backends
    ui_backend: ?*UiBackend,
    
    // Command mapping table
    cmd_map: [*]const BackendCmdMapping,
    cmd_map_size: u32,
    
    // Statistics
    stats: BackendRouterStats,
    
    // Performance monitoring
    collect_metrics: bool,
    on_metric: OnMetricFn,
    metric_user_data: ?*anyopaque,
    
    // Error handling
    on_error: OnErrorFn,
    error_user_data: ?*anyopaque,
};

// Recorded command for testing
pub const RecordedCommand = extern struct {
    cmd_fn: TtyCmdFn,
    ctx_copy: TtyCtx,        // Note: This is opaque, size must match C
    timestamp_ns: u64,
};

// ============================================================================
// External Function Declarations (C imports)
// ============================================================================

pub extern "c" fn ui_backend_create(
    type: UiBackendType,
    requested_caps: ?*const UiCapabilities
) ?*UiBackend;

pub extern "c" fn ui_backend_destroy(backend: *UiBackend) void;

pub extern "c" fn ui_backend_register(
    tty: *Tty,
    backend: *UiBackend
) c_int;

pub extern "c" fn ui_backend_unregister(tty: *Tty) void;

pub extern "c" fn ui_backend_process_command(
    backend: *UiBackend,
    cmd_fn: TtyCmdFn,
    ctx: *const TtyCtx
) void;

pub extern "c" fn ui_backend_flush_frame(backend: *UiBackend) void;

pub extern "c" fn ui_backend_get_capabilities(
    backend: *const UiBackend
) *const UiCapabilities;

pub extern "c" fn ui_backend_set_frame_rate(
    backend: *UiBackend,
    target_fps: u32
) void;

pub extern "c" fn ui_backend_error_string(err: UiBackendError) [*:0]const u8;

// Frame aggregator functions
pub extern "c" fn frame_aggregator_create(target_fps: u32) ?*FrameAggregator;
pub extern "c" fn frame_aggregator_destroy(agg: *FrameAggregator) void;
pub extern "c" fn frame_aggregator_add_update(
    agg: *FrameAggregator,
    ctx: *const TtyCtx
) void;
pub extern "c" fn frame_aggregator_should_emit(agg: *const FrameAggregator) bool;
pub extern "c" fn frame_aggregator_emit(agg: *FrameAggregator) ?*UiFrame;
pub extern "c" fn frame_aggregator_reset(agg: *FrameAggregator) void;

// Hook functions
pub extern "c" fn tty_hooks_init() void;
pub extern "c" fn tty_hooks_install(backend: *UiBackend) c_int;
pub extern "c" fn tty_hooks_uninstall() c_int;
pub extern "c" fn tty_hooks_get_stats(stats: *TtyHookStats) void;
pub extern "c" fn tty_hooks_reset_stats() void;
pub extern "c" fn tty_hooks_get_function_name(index: c_int) [*:0]const u8;
pub extern "c" fn tty_hooks_get_count() c_int;

// Router functions
pub extern "c" fn backend_router_create(initial_mode: BackendMode) ?*BackendRouter;
pub extern "c" fn backend_router_destroy(router: *BackendRouter) void;
pub extern "c" fn backend_router_set_mode(
    router: *BackendRouter,
    mode: BackendMode
) void;
pub extern "c" fn backend_router_register_ui(
    router: *BackendRouter,
    backend: *UiBackend
) c_int;
pub extern "c" fn backend_router_unregister_ui(router: *BackendRouter) void;
pub extern "c" fn backend_route_command(
    router: *BackendRouter,
    tty: *Tty,
    cmd_fn: TtyCmdFn,
    ctx: *const TtyCtx
) void;
pub extern "c" fn backend_should_route_to_ui(
    router: *const BackendRouter,
    cmd_fn: TtyCmdFn
) bool;
pub extern "c" fn backend_get_command_name(
    router: *const BackendRouter,
    cmd_fn: TtyCmdFn
) [*:0]const u8;
pub extern "c" fn backend_router_set_metrics(
    router: *BackendRouter,
    enable: bool
) void;
pub extern "c" fn backend_router_get_stats(
    router: *const BackendRouter
) *const BackendRouterStats;
pub extern "c" fn backend_router_reset_stats(router: *BackendRouter) void;
pub extern "c" fn backend_router_init_default_mappings(router: *BackendRouter) void;
pub extern "c" fn backend_router_add_mapping(
    router: *BackendRouter,
    name: [*:0]const u8,
    tty_fn: TtyCmdFn,
    ui_fn: UiCmdFn,
    flags: u32
) void;
pub extern "c" fn backend_router_remove_mapping(
    router: *BackendRouter,
    tty_fn: TtyCmdFn
) void;
pub extern "c" fn backend_router_init_global(mode: BackendMode) c_int;
pub extern "c" fn backend_router_cleanup_global() void;
pub extern "c" fn backend_router_configure_hybrid(
    router: *BackendRouter,
    config: *const HybridModeConfig
) void;
pub extern "c" fn backend_router_start_recording(
    router: *BackendRouter,
    max_commands: u32
) void;
pub extern "c" fn backend_router_stop_recording(
    router: *BackendRouter,
    count: *u32
) ?[*]RecordedCommand;
pub extern "c" fn backend_router_replay_commands(
    router: *BackendRouter,
    commands: [*]const RecordedCommand,
    count: u32
) void;
pub extern "c" fn backend_router_get_last_error(
    router: *const BackendRouter
) RouterError;
pub extern "c" fn backend_router_error_string(err: RouterError) [*:0]const u8;

// Global router instance
pub extern var global_backend_router: ?*BackendRouter;

// ============================================================================
// Helper Functions for Safe FFI
// ============================================================================

pub fn createBackend(backend_type: UiBackendType, caps: ?*const UiCapabilities) !*UiBackend {
    const backend = ui_backend_create(backend_type, caps) orelse {
        return error.BackendCreationFailed;
    };
    return backend;
}

pub fn createRouter(mode: BackendMode) !*BackendRouter {
    const router = backend_router_create(mode) orelse {
        return error.RouterCreationFailed;
    };
    return router;
}

// Safe wrapper for callback registration
pub fn registerFrameCallback(
    backend: *UiBackend,
    comptime callback: fn(*const UiFrame, ?*anyopaque) void,
    user_data: ?*anyopaque
) void {
    const backend_struct = @ptrCast(*UiBackendStruct, backend);
    backend_struct.on_frame = callback;
    backend_struct.user_data = user_data;
}

// ============================================================================
// Compile-time Validation
// ============================================================================

comptime {
    // Ensure structures match C layout exactly
    std.debug.assert(@sizeOf(UiCell) == 16);
    std.debug.assert(@sizeOf(UiSpan) == 24);
    std.debug.assert(@alignOf(UiCell) == 4);
    std.debug.assert(@alignOf(UiSpan) == 8);
    
    // Ensure first field is always 'size' for ABI stability
    std.debug.assert(@offsetOf(UiFrame, "size") == 0);
    std.debug.assert(@offsetOf(UiCapabilities, "size") == 0);
    std.debug.assert(@offsetOf(FrameAggregator, "size") == 0);
    std.debug.assert(@offsetOf(BackendRouterStats, "size") == 0);
    std.debug.assert(@offsetOf(UiBackendOps, "size") == 0);
    std.debug.assert(@offsetOf(UiBackendStruct, "size") == 0);
    std.debug.assert(@offsetOf(BackendRouterStruct, "size") == 0);
}

// ============================================================================
// Testing Support
// ============================================================================

test "FFI type sizes match C" {
    try std.testing.expectEqual(@as(usize, 16), @sizeOf(UiCell));
    try std.testing.expectEqual(@as(usize, 24), @sizeOf(UiSpan));
    try std.testing.expectEqual(@as(usize, 2), @sizeOf(UiAttrFlags));
    try std.testing.expectEqual(@as(usize, 4), @sizeOf(UiFrameFlags));
    try std.testing.expectEqual(@as(usize, 4), @sizeOf(UiCapFlags));
}

test "Bitfield conversions" {
    const attrs = UiAttrFlags{ .bold = true, .italic = true };
    const u16_val = attrs.toU16();
    try std.testing.expectEqual(@as(u16, 0x0003), u16_val);
    
    const attrs2 = UiAttrFlags.fromU16(u16_val);
    try std.testing.expectEqual(true, attrs2.bold);
    try std.testing.expectEqual(true, attrs2.italic);
    try std.testing.expectEqual(false, attrs2.underline);
}

test "Zero-copy pointer casting" {
    var cell = UiCell{
        .codepoint = 'A',
        .fg_rgb = UI_COLOR_DEFAULT,
        .bg_rgb = UI_COLOR_DEFAULT,
        .attrs = 0,
        .width = 1,
        .cluster_cont = 0,
    };
    
    // These should compile without errors
    const c_ptr = cell.toC();
    const zig_ptr = UiCell.fromC(c_ptr);
    try std.testing.expectEqual(cell.codepoint, zig_ptr.codepoint);
}