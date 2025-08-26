// FFI Bridge for Ghostty-tmux Integration
// Task: T-301/T-302 - Zig-C FFI绑定 & Ghostty集成层
// Performance: <100ns FFI overhead, Zero-copy design

const std = @import("std");
const c = @cImport({
    @cInclude("event_loop_backend.h");
    @cInclude("grid_callbacks.h");
    @cInclude("layout_callbacks.h");
});

// Type mappings for C structures
pub const EventBase = opaque {};
pub const Event = extern struct {
    fd: c_int,
    events: c_short,
    res: c_short,
    callback: ?*const fn (c_int, c_short, ?*anyopaque) callconv(.C) void,
    arg: ?*anyopaque,
    base: ?*EventBase,
    internal: ?*anyopaque,
    next: ?*Event,
    prev: ?*Event,
};

// Grid cell mapping - matches C structure exactly
pub const GridCell = extern struct {
    codepoint: u32,
    attr: u16,
    fg: u8,
    bg: u8,
};

// Ensure proper alignment and size
comptime {
    std.debug.assert(@sizeOf(GridCell) == 8);
    std.debug.assert(@alignOf(GridCell) == 4);
}

// Event backend operations vtable
pub const EventBackendOps = extern struct {
    name: [*c]const u8,
    version: u32,
    capabilities: u32,
    
    init: ?*const fn () callconv(.C) ?*EventBase,
    free: ?*const fn (*EventBase) callconv(.C) void,
    
    add: ?*const fn (*EventBase, *Event, ?*const c.struct_timeval) callconv(.C) c_int,
    del: ?*const fn (*EventBase, *Event) callconv(.C) c_int,
    modify: ?*const fn (*EventBase, *Event, ?*const c.struct_timeval) callconv(.C) c_int,
    
    dispatch: ?*const fn (*EventBase, c_int) callconv(.C) c_int,
    loop: ?*const fn (*EventBase, c_int) callconv(.C) c_int,
    loopexit: ?*const fn (*EventBase, ?*const c.struct_timeval) callconv(.C) void,
    loopbreak: ?*const fn (*EventBase) callconv(.C) void,
    
    get_ops_count: ?*const fn (*EventBase) callconv(.C) u64,
    get_avg_latency: ?*const fn (*EventBase) callconv(.C) f64,
    
    lock: ?*const fn (*EventBase) callconv(.C) c_int,
    unlock: ?*const fn (*EventBase) callconv(.C) c_int,
};

// Ghostty Terminal Backend Implementation
pub const GhosttyBackend = struct {
    allocator: std.mem.Allocator,
    event_base: ?*EventBase,
    grid: ?*Grid,
    layout_mgr: ?*LayoutManager,
    
    // Performance tracking
    ops_count: std.atomic.Atomic(u64),
    total_latency_ns: std.atomic.Atomic(u64),
    
    const Self = @This();
    
    pub fn init(allocator: std.mem.Allocator) !Self {
        return Self{
            .allocator = allocator,
            .event_base = null,
            .grid = null,
            .layout_mgr = null,
            .ops_count = std.atomic.Atomic(u64).init(0),
            .total_latency_ns = std.atomic.Atomic(u64).init(0),
        };
    }
    
    pub fn deinit(self: *Self) void {
        if (self.event_base) |base| {
            c.event_base_free(base);
        }
        if (self.grid) |grid| {
            self.allocator.destroy(grid);
        }
        if (self.layout_mgr) |mgr| {
            self.allocator.destroy(mgr);
        }
    }
    
    // Initialize event loop with our custom backend
    pub fn initEventLoop(self: *Self) !void {
        self.event_base = c.event_base_new_with_backend(c.BACKEND_GHOSTTY);
        if (self.event_base == null) {
            return error.EventLoopInitFailed;
        }
    }
    
    // Grid operations with SIMD optimization detection
    pub fn initGrid(self: *Self, rows: usize, cols: usize) !void {
        const use_simd = std.Target.x86.featureSetHas(
            std.Target.current.cpu.features, 
            .avx2
        );
        
        c.grid_init_backend(use_simd);
        
        const backend = c.grid_get_backend();
        if (backend.create) |create_fn| {
            const grid_ptr = create_fn(@intCast(c_int, rows), @intCast(c_int, cols));
            self.grid = @ptrCast(?*Grid, grid_ptr);
        }
    }
    
    // Zero-copy callback wrapper
    pub fn wrapCallback(comptime T: type, comptime func: fn (*T, c_int, c_short) anyerror!void) fn (c_int, c_short, ?*anyopaque) callconv(.C) void {
        return struct {
            fn callback(fd: c_int, events: c_short, arg: ?*anyopaque) callconv(.C) void {
                const timer_start = std.time.nanoTimestamp();
                defer {
                    const timer_end = std.time.nanoTimestamp();
                    const latency = timer_end - timer_start;
                    // Update performance metrics atomically
                    _ = GhosttyBackend.ops_count.fetchAdd(1, .Monotonic);
                    _ = GhosttyBackend.total_latency_ns.fetchAdd(@intCast(u64, latency), .Monotonic);
                }
                
                if (arg) |ptr| {
                    const obj = @ptrCast(*T, @alignCast(@alignOf(T), ptr));
                    func(obj, fd, events) catch |err| {
                        std.log.err("Callback error: {}", .{err});
                    };
                }
            }
        }.callback;
    }
    
    // High-performance event dispatching
    pub fn dispatch(self: *Self) !void {
        if (self.event_base) |base| {
            const result = c.event_base_dispatch(base);
            if (result < 0) {
                return error.DispatchFailed;
            }
        }
    }
    
    // Batch grid update with SIMD
    pub fn batchUpdateGrid(self: *Self, cells: []const GridCell, row: usize, col: usize) !void {
        if (self.grid) |grid| {
            const backend = c.grid_get_backend();
            if (backend.batch_update) |update_fn| {
                update_fn(
                    grid,
                    @intCast(c_int, row),
                    @intCast(c_int, col),
                    @ptrCast([*c]const c.grid_cell_t, cells.ptr),
                    cells.len
                );
            }
        }
    }
    
    // Layout management
    pub fn setLayout(self: *Self, layout_type: LayoutType) !void {
        if (self.layout_mgr) |mgr| {
            const ops = mgr.ops;
            if (ops.set_layout) |set_fn| {
                const result = set_fn(@enumToInt(layout_type));
                if (result < 0) {
                    return error.LayoutChangeFailed;
                }
            }
        }
    }
    
    // Performance monitoring
    pub fn getPerformanceStats(self: *Self) PerformanceStats {
        const ops = self.ops_count.load(.Monotonic);
        const latency = self.total_latency_ns.load(.Monotonic);
        
        return PerformanceStats{
            .total_ops = ops,
            .avg_latency_ns = if (ops > 0) latency / ops else 0,
            .throughput = if (ops > 0) @intToFloat(f64, ops) * 1e9 / @intToFloat(f64, latency) else 0,
        };
    }
};

// Grid wrapper for Zig
pub const Grid = extern struct {
    cells: [*][*]GridCell,
    rows: c_int,
    cols: c_int,
    cursor_x: c_int,
    cursor_y: c_int,
    dirty: DirtyRegion,
    simd_available: bool,
    use_avx2: bool,
    use_sse4: bool,
};

pub const DirtyRegion = extern struct {
    start_row: c_int,
    end_row: c_int,
    start_col: c_int,
    end_col: c_int,
    needs_redraw: bool,
};

pub const LayoutManager = extern struct {
    current_layout: c_int,
    root_cell: ?*anyopaque,
    pane_list: ?*anyopaque,
    active_pane: ?*anyopaque,
    ops: *const c.layout_ops_t,
    layout_switches: u64,
    total_switch_time_us: u64,
    pane_operations: u64,
};

pub const LayoutType = enum(c_int) {
    even_horizontal = 0,
    even_vertical = 1,
    main_horizontal = 2,
    main_vertical = 3,
    tiled = 4,
    custom = 5,
};

pub const PerformanceStats = struct {
    total_ops: u64,
    avg_latency_ns: u64,
    throughput: f64,
};

// Memory safety wrapper
pub fn SafeBuffer(comptime T: type) type {
    return struct {
        ptr: [*]T,
        len: usize,
        
        const Self = @This();
        
        pub fn init(ptr: [*]T, len: usize) Self {
            return Self{ .ptr = ptr, .len = len };
        }
        
        pub fn slice(self: Self) []T {
            return self.ptr[0..self.len];
        }
        
        pub fn at(self: Self, index: usize) !*T {
            if (index >= self.len) {
                return error.IndexOutOfBounds;
            }
            return &self.ptr[index];
        }
    };
}

// Export Ghostty backend operations for C
export fn ghostty_backend_init() ?*anyopaque {
    const allocator = std.heap.c_allocator;
    const backend = allocator.create(GhosttyBackend) catch return null;
    backend.* = GhosttyBackend.init(allocator) catch {
        allocator.destroy(backend);
        return null;
    };
    return @ptrCast(*anyopaque, backend);
}

export fn ghostty_backend_free(backend: ?*anyopaque) void {
    if (backend) |ptr| {
        const gb = @ptrCast(*GhosttyBackend, @alignCast(@alignOf(GhosttyBackend), ptr));
        gb.deinit();
        std.heap.c_allocator.destroy(gb);
    }
}

export fn ghostty_backend_dispatch(backend: ?*anyopaque) c_int {
    if (backend) |ptr| {
        const gb = @ptrCast(*GhosttyBackend, @alignCast(@alignOf(GhosttyBackend), ptr));
        gb.dispatch() catch return -1;
        return 0;
    }
    return -1;
}

// Test support
test "FFI type sizes match" {
    try std.testing.expectEqual(@sizeOf(GridCell), @sizeOf(c.grid_cell_t));
    try std.testing.expectEqual(@alignOf(GridCell), @alignOf(c.grid_cell_t));
}

test "Zero-copy buffer operations" {
    var cells: [100]GridCell = undefined;
    const buffer = SafeBuffer(GridCell).init(&cells, cells.len);
    
    const cell = try buffer.at(50);
    cell.codepoint = 'A';
    cell.attr = 0;
    cell.fg = 7;
    cell.bg = 0;
    
    try std.testing.expectEqual(cells[50].codepoint, 'A');
}

test "Performance tracking" {
    const backend = try GhosttyBackend.init(std.testing.allocator);
    defer backend.deinit();
    
    _ = backend.ops_count.fetchAdd(1000, .Monotonic);
    _ = backend.total_latency_ns.fetchAdd(280000, .Monotonic); // 280us for 1000 ops
    
    const stats = backend.getPerformanceStats();
    try std.testing.expectEqual(stats.total_ops, 1000);
    try std.testing.expectEqual(stats.avg_latency_ns, 280); // 280ns average
}