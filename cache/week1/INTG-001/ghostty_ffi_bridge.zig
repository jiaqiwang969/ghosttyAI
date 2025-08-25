// ghostty_ffi_bridge.zig - Zig FFI Bridge for Ghostty Backend Integration
// Purpose: Bridge between C libtmuxcore and Zig Ghostty implementation
// Author: INTG-001 (Zig-Ghostty Integration Specialist)
// Date: 2025-08-25
// Version: 1.0.0

const std = @import("std");
const builtin = @import("builtin");
const assert = std.debug.assert;
const terminal = @import("../../ghostty/src/terminal/main.zig");
const termio = @import("../../ghostty/src/termio.zig");
const renderer = @import("../../ghostty/src/renderer.zig");
const config = @import("../../ghostty/src/config.zig");

const log = std.log.scoped(.ghostty_ffi);

// ============================================================================
// C Types Import
// ============================================================================

const c = @cImport({
    @cInclude("../ARCH-001/ui_backend.h");
    @cInclude("../CORE-001/tty_write_hooks.h");
});

// ============================================================================
// Ghostty Terminal Wrapper
// ============================================================================

pub const GhosttyTerminal = struct {
    allocator: std.mem.Allocator,
    terminal: *terminal.Terminal,
    termio_thread: ?std.Thread = null,
    
    // Synchronization
    mutex: std.Thread.Mutex,
    frame_queue: FrameQueue,
    
    // Callbacks from C
    callbacks: CallbackSet,
    
    // Statistics
    stats: Statistics,
    
    // Configuration
    config: Config,
    
    pub const Config = struct {
        rows: u32 = 24,
        cols: u32 = 80,
        scrollback: u32 = 10000,
        immediate_mode: bool = false,
        grid_optimization: bool = true,
    };
    
    pub const Statistics = struct {
        frames_processed: std.atomic.Value(u64) = std.atomic.Value(u64).init(0),
        cells_updated: std.atomic.Value(u64) = std.atomic.Value(u64).init(0),
        memory_allocated: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),
        errors: std.atomic.Value(u32) = std.atomic.Value(u32).init(0),
    };
    
    pub const CallbackSet = struct {
        on_frame: ?*const fn ([*c]const c.ui_frame_t, ?*anyopaque) callconv(.C) void = null,
        on_bell: ?*const fn (u32, ?*anyopaque) callconv(.C) void = null,
        on_title: ?*const fn (u32, [*c]const u8, ?*anyopaque) callconv(.C) void = null,
        user_data: ?*anyopaque = null,
    };
};

// ============================================================================
// Frame Queue for Thread-Safe Communication
// ============================================================================

const FrameQueue = struct {
    const Frame = struct {
        data: []u8,
        timestamp_ns: u64,
        pane_id: u32,
        urgent: bool,
    };
    
    queue: std.ArrayList(Frame),
    mutex: std.Thread.Mutex,
    condition: std.Thread.Condition,
    
    fn init(allocator: std.mem.Allocator) FrameQueue {
        return .{
            .queue = std.ArrayList(Frame).init(allocator),
            .mutex = .{},
            .condition = .{},
        };
    }
    
    fn deinit(self: *FrameQueue) void {
        for (self.queue.items) |frame| {
            self.queue.allocator.free(frame.data);
        }
        self.queue.deinit();
    }
    
    fn push(self: *FrameQueue, frame: Frame) !void {
        self.mutex.lock();
        defer self.mutex.unlock();
        
        try self.queue.append(frame);
        self.condition.signal();
    }
    
    fn pop(self: *FrameQueue) ?Frame {
        self.mutex.lock();
        defer self.mutex.unlock();
        
        if (self.queue.items.len == 0) return null;
        return self.queue.orderedRemove(0);
    }
    
    fn wait_and_pop(self: *FrameQueue) Frame {
        self.mutex.lock();
        defer self.mutex.unlock();
        
        while (self.queue.items.len == 0) {
            self.condition.wait(&self.mutex);
        }
        
        return self.queue.orderedRemove(0);
    }
};

// ============================================================================
// Memory Management Helpers
// ============================================================================

const MemoryTracker = struct {
    allocator: std.mem.Allocator,
    stats: *GhosttyTerminal.Statistics,
    
    fn alloc(self: MemoryTracker, size: usize) ![]u8 {
        const mem = try self.allocator.alloc(u8, size);
        _ = self.stats.memory_allocated.fetchAdd(size, .monotonic);
        return mem;
    }
    
    fn free(self: MemoryTracker, mem: []u8) void {
        _ = self.stats.memory_allocated.fetchSub(mem.len, .monotonic);
        self.allocator.free(mem);
    }
    
    fn dupeZ(self: MemoryTracker, str: []const u8) ![:0]u8 {
        const mem = try self.allocator.dupeZ(u8, str);
        _ = self.stats.memory_allocated.fetchAdd(mem.len + 1, .monotonic);
        return mem;
    }
};

// ============================================================================
// FFI Export Functions
// ============================================================================

export fn ghostty_ffi_create_terminal(user_data: ?*anyopaque) ?*GhosttyTerminal {
    const allocator = std.heap.c_allocator;
    
    const gt = allocator.create(GhosttyTerminal) catch |err| {
        log.err("Failed to allocate GhosttyTerminal: {}", .{err});
        return null;
    };
    
    gt.* = .{
        .allocator = allocator,
        .terminal = undefined,
        .mutex = .{},
        .frame_queue = FrameQueue.init(allocator),
        .callbacks = .{},
        .stats = .{},
        .config = .{},
    };
    
    // Create terminal instance
    gt.terminal = allocator.create(terminal.Terminal) catch |err| {
        log.err("Failed to allocate Terminal: {}", .{err});
        allocator.destroy(gt);
        return null;
    };
    
    // Initialize terminal with default size
    gt.terminal.* = terminal.Terminal.init(
        allocator,
        .{
            .rows = @intCast(gt.config.rows),
            .cols = @intCast(gt.config.cols),
            .scrollback_size = @intCast(gt.config.scrollback),
        },
    ) catch |err| {
        log.err("Failed to initialize Terminal: {}", .{err});
        allocator.destroy(gt.terminal);
        allocator.destroy(gt);
        return null;
    };
    
    log.info("Created GhosttyTerminal with {}x{} grid", .{ gt.config.cols, gt.config.rows });
    
    return gt;
}

export fn ghostty_ffi_destroy_terminal(term: ?*GhosttyTerminal) void {
    if (term) |gt| {
        gt.mutex.lock();
        defer gt.mutex.unlock();
        
        // Stop termio thread if running
        if (gt.termio_thread) |thread| {
            thread.join();
        }
        
        // Clean up terminal
        gt.terminal.deinit();
        gt.allocator.destroy(gt.terminal);
        
        // Clean up frame queue
        gt.frame_queue.deinit();
        
        // Free the wrapper
        gt.allocator.destroy(gt);
        
        log.info("Destroyed GhosttyTerminal", .{});
    }
}

export fn ghostty_ffi_register_callbacks(
    term: ?*GhosttyTerminal,
    callbacks: ?*const anyopaque,
) void {
    if (term) |gt| {
        if (callbacks) |cb_ptr| {
            // Cast to our expected callback structure
            const cb = @as(*const struct {
                on_frame: ?*const fn ([*c]const c.ui_frame_t, ?*anyopaque) callconv(.C) void,
                on_bell: ?*const fn (u32, ?*anyopaque) callconv(.C) void,
                on_title: ?*const fn (u32, [*c]const u8, ?*anyopaque) callconv(.C) void,
                user_data: ?*anyopaque,
            }, @ptrCast(@alignCast(cb_ptr)));
            
            gt.mutex.lock();
            defer gt.mutex.unlock();
            
            gt.callbacks = .{
                .on_frame = cb.on_frame,
                .on_bell = cb.on_bell,
                .on_title = cb.on_title,
                .user_data = cb.user_data,
            };
            
            log.info("Registered callbacks", .{});
        }
    }
}

export fn ghostty_ffi_process_frame(
    term: ?*GhosttyTerminal,
    frame: [*c]const c.ui_frame_t,
) void {
    if (term) |gt| {
        if (frame == null) return;
        
        _ = gt.stats.frames_processed.fetchAdd(1, .monotonic);
        
        // Process each span in the frame
        const span_count = frame.*.span_count;
        const spans = frame.*.spans;
        
        var cells_updated: u64 = 0;
        
        gt.mutex.lock();
        defer gt.mutex.unlock();
        
        var i: u32 = 0;
        while (i < span_count) : (i += 1) {
            const span = &spans[i];
            processSpan(gt, span) catch |err| {
                log.err("Failed to process span: {}", .{err});
                _ = gt.stats.errors.fetchAdd(1, .monotonic);
            };
            
            cells_updated += (span.col_end - span.col_start);
        }
        
        _ = gt.stats.cells_updated.fetchAdd(cells_updated, .monotonic);
        
        // Trigger callbacks if needed
        if (frame.*.flags & c.UI_FRAME_URGENT != 0) {
            flushToCallbacks(gt);
        }
    }
}

export fn ghostty_ffi_flush_immediate(term: ?*GhosttyTerminal) void {
    if (term) |gt| {
        gt.mutex.lock();
        defer gt.mutex.unlock();
        
        flushToCallbacks(gt);
    }
}

export fn ghostty_ffi_get_capabilities() u32 {
    return c.UI_CAP_FRAME_BATCH |
           c.UI_CAP_UTF8_LINES |
           c.UI_CAP_24BIT_COLOR |
           c.UI_CAP_BORDERS_BY_UI |
           c.UI_CAP_CURSOR_SHAPES |
           c.UI_CAP_UNDERLINE_STYLES |
           c.UI_CAP_SIXEL |
           c.UI_CAP_SYNCHRONIZED;
}

// ============================================================================
// Internal Processing Functions
// ============================================================================

fn processSpan(gt: *GhosttyTerminal, span: [*c]const c.ui_span_t) !void {
    const row = span.*.row;
    const col_start = span.*.col_start;
    const col_end = span.*.col_end;
    const cells = span.*.cells;
    
    // Validate bounds
    if (row >= gt.config.rows) return error.RowOutOfBounds;
    if (col_end > gt.config.cols) return error.ColOutOfBounds;
    
    // Get the terminal screen
    const screen = gt.terminal.getActiveScreen();
    
    // Process each cell in the span
    var col = col_start;
    while (col < col_end) : (col += 1) {
        const ui_cell = &cells[col - col_start];
        
        // Convert ui_cell to terminal cell
        var term_cell = terminal.Cell{
            .char = .{ .codepoint = ui_cell.codepoint },
            .attrs = convertAttributes(ui_cell.attrs),
            .fg = convertColor(ui_cell.fg_rgb),
            .bg = convertColor(ui_cell.bg_rgb),
            .width = ui_cell.width,
        };
        
        // Write to terminal grid
        try screen.setCell(
            @intCast(row),
            @intCast(col),
            term_cell,
        );
    }
}

fn convertAttributes(attrs: u16) terminal.Cell.Attributes {
    var result = terminal.Cell.Attributes{};
    
    if (attrs & c.UI_ATTR_BOLD != 0) result.bold = true;
    if (attrs & c.UI_ATTR_ITALIC != 0) result.italic = true;
    if (attrs & c.UI_ATTR_UNDERLINE != 0) result.underline = .single;
    if (attrs & c.UI_ATTR_DIM != 0) result.dim = true;
    if (attrs & c.UI_ATTR_REVERSE != 0) result.reverse = true;
    if (attrs & c.UI_ATTR_BLINK != 0) result.blink = true;
    if (attrs & c.UI_ATTR_STRIKE != 0) result.strikethrough = true;
    if (attrs & c.UI_ATTR_DOUBLE_UL != 0) result.underline = .double;
    if (attrs & c.UI_ATTR_CURLY_UL != 0) result.underline = .curly;
    if (attrs & c.UI_ATTR_DOTTED_UL != 0) result.underline = .dotted;
    if (attrs & c.UI_ATTR_DASHED_UL != 0) result.underline = .dashed;
    
    return result;
}

fn convertColor(rgb: u32) terminal.Color {
    if (rgb == c.UI_COLOR_DEFAULT) {
        return .default;
    } else if (rgb == c.UI_COLOR_INVALID) {
        return .default;
    } else {
        // Extract RGB components
        const r = @as(u8, @intCast((rgb >> 16) & 0xFF));
        const g = @as(u8, @intCast((rgb >> 8) & 0xFF));
        const b = @as(u8, @intCast(rgb & 0xFF));
        
        return .{ .rgb = .{ .r = r, .g = g, .b = b } };
    }
}

fn flushToCallbacks(gt: *GhosttyTerminal) void {
    // Create a frame snapshot and send to callbacks
    if (gt.callbacks.on_frame) |on_frame| {
        // Build frame from current terminal state
        var frame = c.ui_frame_t{
            .size = @sizeOf(c.ui_frame_t),
            .frame_seq = gt.stats.frames_processed.load(.monotonic),
            .timestamp_ns = std.time.nanoTimestamp(),
            .pane_id = 0,
            .span_count = 0,
            .spans = null,
            .flags = c.UI_FRAME_COMPLETE,
            .updates_batched = 1,
            .cells_modified = 0,
            .frames_dropped = 0,
        };
        
        on_frame(&frame, gt.callbacks.user_data);
    }
}

// ============================================================================
// Testing Support Functions
// ============================================================================

export fn ghostty_ffi_test_write_string(
    term: ?*GhosttyTerminal,
    str: [*c]const u8,
    len: usize,
) void {
    if (term) |gt| {
        const data = str[0..len];
        
        gt.mutex.lock();
        defer gt.mutex.unlock();
        
        // Write to terminal
        gt.terminal.write(data) catch |err| {
            log.err("Failed to write to terminal: {}", .{err});
            _ = gt.stats.errors.fetchAdd(1, .monotonic);
        };
    }
}

export fn ghostty_ffi_test_resize(
    term: ?*GhosttyTerminal,
    rows: u32,
    cols: u32,
) void {
    if (term) |gt| {
        gt.mutex.lock();
        defer gt.mutex.unlock();
        
        gt.config.rows = rows;
        gt.config.cols = cols;
        
        gt.terminal.resize(.{
            .rows = @intCast(rows),
            .cols = @intCast(cols),
        }) catch |err| {
            log.err("Failed to resize terminal: {}", .{err});
            _ = gt.stats.errors.fetchAdd(1, .monotonic);
        };
        
        log.info("Resized terminal to {}x{}", .{ cols, rows });
    }
}

export fn ghostty_ffi_test_get_stats(
    term: ?*GhosttyTerminal,
    frames: ?*u64,
    cells: ?*u64,
    memory: ?*usize,
    errors: ?*u32,
) void {
    if (term) |gt| {
        if (frames) |f| f.* = gt.stats.frames_processed.load(.monotonic);
        if (cells) |c| c.* = gt.stats.cells_updated.load(.monotonic);
        if (memory) |m| m.* = gt.stats.memory_allocated.load(.monotonic);
        if (errors) |e| e.* = gt.stats.errors.load(.monotonic);
    }
}

// ============================================================================
// Thread Worker Functions
// ============================================================================

fn termioWorker(gt: *GhosttyTerminal) void {
    log.info("TermIO worker thread started", .{});
    
    while (true) {
        const frame = gt.frame_queue.wait_and_pop();
        
        // Process frame data
        processFrameData(gt, frame) catch |err| {
            log.err("Failed to process frame data: {}", .{err});
            _ = gt.stats.errors.fetchAdd(1, .monotonic);
        };
        
        // Free frame data
        gt.allocator.free(frame.data);
        
        // Check for termination
        // TODO: Add proper termination signaling
    }
}

fn processFrameData(gt: *GhosttyTerminal, frame: FrameQueue.Frame) !void {
    // Parse frame data and apply to terminal
    // This is where we'd deserialize the frame and apply it
    _ = gt;
    _ = frame;
}

// ============================================================================
// Initialization
// ============================================================================

pub fn init() void {
    log.info("Ghostty FFI Bridge initialized", .{});
}

// ============================================================================
// Test Support
// ============================================================================

test "create and destroy terminal" {
    const term = ghostty_ffi_create_terminal(null);
    try std.testing.expect(term != null);
    
    ghostty_ffi_destroy_terminal(term);
}

test "capabilities" {
    const caps = ghostty_ffi_get_capabilities();
    try std.testing.expect(caps & c.UI_CAP_FRAME_BATCH != 0);
    try std.testing.expect(caps & c.UI_CAP_24BIT_COLOR != 0);
}

test "write and resize" {
    const term = ghostty_ffi_create_terminal(null);
    defer ghostty_ffi_destroy_terminal(term);
    
    // Write test string
    const test_str = "Hello, Ghostty!";
    ghostty_ffi_test_write_string(term, test_str.ptr, test_str.len);
    
    // Resize terminal
    ghostty_ffi_test_resize(term, 30, 100);
    
    // Check stats
    var frames: u64 = 0;
    var cells: u64 = 0;
    var memory: usize = 0;
    var errors: u32 = 0;
    
    ghostty_ffi_test_get_stats(term, &frames, &cells, &memory, &errors);
    try std.testing.expect(errors == 0);
}