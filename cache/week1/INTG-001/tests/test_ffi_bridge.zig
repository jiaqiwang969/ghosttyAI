// test_ffi_bridge.zig - Test Zig-C FFI bridge functionality
// Purpose: Test FFI boundary, memory management, and type conversions
// Author: INTG-001 (Zig-Ghostty Integration Specialist)
// Date: 2025-08-25
// Version: 1.0.0

const std = @import("std");
const testing = std.testing;
const c = @cImport({
    @cInclude("../../ARCH-001/ui_backend.h");
    @cInclude("../../ARCH-001/tty_ctx_unified.h");
});

// Import our FFI bridge
const ffi = @import("../ghostty_ffi_bridge.zig");

// Test allocator
const test_allocator = std.testing.allocator;

// Test fixture for FFI tests
const TestFixture = struct {
    terminal: ?*ffi.GhosttyTerminal,
    callback_count: u32,
    last_frame: ?*c.ui_frame_t,
    
    fn init() TestFixture {
        return .{
            .terminal = null,
            .callback_count = 0,
            .last_frame = null,
        };
    }
    
    fn deinit(self: *TestFixture) void {
        if (self.terminal) |term| {
            ffi.ghostty_ffi_destroy_terminal(term);
        }
        if (self.last_frame) |frame| {
            test_allocator.free(frame);
        }
    }
};

// Test basic FFI functions
test "FFI create and destroy terminal" {
    const term = ffi.ghostty_ffi_create_terminal(null);
    try testing.expect(term != null);
    
    ffi.ghostty_ffi_destroy_terminal(term);
}

test "FFI null terminal handling" {
    // Should handle null gracefully
    ffi.ghostty_ffi_destroy_terminal(null);
    ffi.ghostty_ffi_register_callbacks(null, null);
    ffi.ghostty_ffi_process_frame(null, null);
    _ = ffi.ghostty_ffi_flush_immediate(null);
}

test "FFI capabilities" {
    const caps = ffi.ghostty_ffi_get_capabilities();
    
    // Check expected capabilities
    try testing.expect((caps & c.UI_CAP_FRAME_BATCH) != 0);
    try testing.expect((caps & c.UI_CAP_24BIT_COLOR) != 0);
    try testing.expect((caps & c.UI_CAP_UTF8_LINES) != 0);
}

// Test callback registration
test "FFI callback registration" {
    const term = ffi.ghostty_ffi_create_terminal(null);
    defer ffi.ghostty_ffi_destroy_terminal(term);
    
    // Create callback structure
    const Callbacks = struct {
        on_frame: ?*const fn ([*c]const c.ui_frame_t, ?*anyopaque) callconv(.C) void,
        on_bell: ?*const fn (u32, ?*anyopaque) callconv(.C) void,
        on_title: ?*const fn (u32, [*c]const u8, ?*anyopaque) callconv(.C) void,
        user_data: ?*anyopaque,
    };
    
    var callbacks = Callbacks{
        .on_frame = null,
        .on_bell = null,
        .on_title = null,
        .user_data = null,
    };
    
    ffi.ghostty_ffi_register_callbacks(term, &callbacks);
}

// Test frame processing
test "FFI frame processing" {
    const term = ffi.ghostty_ffi_create_terminal(null);
    defer ffi.ghostty_ffi_destroy_terminal(term);
    
    // Create a test frame
    var cell = c.ui_cell_t{
        .codepoint = 'A',
        .fg_rgb = 0xFFFFFF,
        .bg_rgb = 0x000000,
        .attrs = c.UI_ATTR_BOLD,
        .width = 1,
        .cluster_cont = 0,
    };
    
    var span = c.ui_span_t{
        .row = 0,
        .col_start = 0,
        .col_end = 1,
        .cells = &cell,
        .flags = 0,
    };
    
    var frame = c.ui_frame_t{
        .size = @sizeOf(c.ui_frame_t),
        .frame_seq = 1,
        .timestamp_ns = 0,
        .pane_id = 1,
        .span_count = 1,
        .spans = &span,
        .flags = c.UI_FRAME_COMPLETE,
        .updates_batched = 1,
        .cells_modified = 1,
        .frames_dropped = 0,
    };
    
    const result = ffi.ghostty_ffi_process_frame(term, &frame);
    try testing.expect(result == 0);
}

// Test memory management
test "FFI memory tracking" {
    var fixture = TestFixture.init();
    defer fixture.deinit();
    
    fixture.terminal = ffi.ghostty_ffi_create_terminal(&fixture);
    try testing.expect(fixture.terminal != null);
    
    // Get initial stats
    var frames: u64 = 0;
    var cells: u64 = 0;
    var memory: usize = 0;
    var errors: u32 = 0;
    
    ffi.ghostty_ffi_test_get_stats(fixture.terminal, &frames, &cells, &memory, &errors);
    
    const initial_memory = memory;
    
    // Do some operations
    for (0..10) |_| {
        const test_str = "Hello, Ghostty!";
        ffi.ghostty_ffi_test_write_string(fixture.terminal, test_str.ptr, test_str.len);
    }
    
    // Check memory increased
    ffi.ghostty_ffi_test_get_stats(fixture.terminal, &frames, &cells, &memory, &errors);
    try testing.expect(memory >= initial_memory);
    try testing.expect(errors == 0);
}

// Test terminal operations
test "FFI terminal write and resize" {
    const term = ffi.ghostty_ffi_create_terminal(null);
    defer ffi.ghostty_ffi_destroy_terminal(term);
    
    // Write test string
    const test_str = "Testing FFI Bridge";
    ffi.ghostty_ffi_test_write_string(term, test_str.ptr, test_str.len);
    
    // Resize terminal
    ffi.ghostty_ffi_test_resize(term, 100, 40);
    
    // Get stats
    var frames: u64 = 0;
    var cells: u64 = 0;
    var memory: usize = 0;
    var errors: u32 = 0;
    
    ffi.ghostty_ffi_test_get_stats(term, &frames, &cells, &memory, &errors);
    try testing.expect(errors == 0);
}

// Test error conditions
test "FFI error handling" {
    const term = ffi.ghostty_ffi_create_terminal(null);
    defer ffi.ghostty_ffi_destroy_terminal(term);
    
    // Process null frame
    const result = ffi.ghostty_ffi_process_frame(term, null);
    try testing.expect(result != 0);
    
    // Process frame with invalid data
    var bad_frame = c.ui_frame_t{
        .size = 1, // Invalid size
        .frame_seq = 0,
        .timestamp_ns = 0,
        .pane_id = 0,
        .span_count = 999999, // Huge span count
        .spans = null, // But null spans
        .flags = 0,
        .updates_batched = 0,
        .cells_modified = 0,
        .frames_dropped = 0,
    };
    
    _ = ffi.ghostty_ffi_process_frame(term, &bad_frame);
    
    // Check error count increased
    var errors: u32 = 0;
    ffi.ghostty_ffi_test_get_stats(term, null, null, null, &errors);
    try testing.expect(errors > 0);
}

// Test type conversions
test "FFI type conversions" {
    const term = ffi.ghostty_ffi_create_terminal(null);
    defer ffi.ghostty_ffi_destroy_terminal(term);
    
    // Test color conversions
    const colors = [_]u32{
        c.UI_COLOR_DEFAULT,
        c.UI_COLOR_INVALID,
        0x00FF00, // Green
        0xFF0000, // Red
        0x0000FF, // Blue
    };
    
    for (colors) |color| {
        var cell = c.ui_cell_t{
            .codepoint = 'X',
            .fg_rgb = color,
            .bg_rgb = c.UI_COLOR_DEFAULT,
            .attrs = 0,
            .width = 1,
            .cluster_cont = 0,
        };
        
        var span = c.ui_span_t{
            .row = 0,
            .col_start = 0,
            .col_end = 1,
            .cells = &cell,
            .flags = 0,
        };
        
        var frame = c.ui_frame_t{
            .size = @sizeOf(c.ui_frame_t),
            .frame_seq = 1,
            .timestamp_ns = 0,
            .pane_id = 1,
            .span_count = 1,
            .spans = &span,
            .flags = 0,
            .updates_batched = 1,
            .cells_modified = 1,
            .frames_dropped = 0,
        };
        
        _ = ffi.ghostty_ffi_process_frame(term, &frame);
    }
}

// Test attribute conversions
test "FFI attribute conversions" {
    const term = ffi.ghostty_ffi_create_terminal(null);
    defer ffi.ghostty_ffi_destroy_terminal(term);
    
    const attrs = [_]u16{
        c.UI_ATTR_BOLD,
        c.UI_ATTR_ITALIC,
        c.UI_ATTR_UNDERLINE,
        c.UI_ATTR_DIM,
        c.UI_ATTR_REVERSE,
        c.UI_ATTR_BLINK,
        c.UI_ATTR_STRIKE,
        c.UI_ATTR_DOUBLE_UL,
        c.UI_ATTR_CURLY_UL,
        c.UI_ATTR_DOTTED_UL,
        c.UI_ATTR_DASHED_UL,
        c.UI_ATTR_BOLD | c.UI_ATTR_ITALIC, // Combined
    };
    
    for (attrs) |attr| {
        var cell = c.ui_cell_t{
            .codepoint = 'A',
            .fg_rgb = 0xFFFFFF,
            .bg_rgb = 0x000000,
            .attrs = attr,
            .width = 1,
            .cluster_cont = 0,
        };
        
        var span = c.ui_span_t{
            .row = 0,
            .col_start = 0,
            .col_end = 1,
            .cells = &cell,
            .flags = 0,
        };
        
        var frame = c.ui_frame_t{
            .size = @sizeOf(c.ui_frame_t),
            .frame_seq = 1,
            .timestamp_ns = 0,
            .pane_id = 1,
            .span_count = 1,
            .spans = &span,
            .flags = 0,
            .updates_batched = 1,
            .cells_modified = 1,
            .frames_dropped = 0,
        };
        
        _ = ffi.ghostty_ffi_process_frame(term, &frame);
    }
}

// Test boundary conditions
test "FFI boundary conditions" {
    const term = ffi.ghostty_ffi_create_terminal(null);
    defer ffi.ghostty_ffi_destroy_terminal(term);
    
    // Test with extreme dimensions
    ffi.ghostty_ffi_test_resize(term, 1, 1);     // Minimum
    ffi.ghostty_ffi_test_resize(term, 9999, 9999); // Large
    ffi.ghostty_ffi_test_resize(term, 0, 0);     // Zero (should handle)
    
    // Test with empty string
    ffi.ghostty_ffi_test_write_string(term, "", 0);
    
    // Test with very long string
    const long_str = "X" ** 1000;
    ffi.ghostty_ffi_test_write_string(term, long_str.ptr, long_str.len);
}

// Test concurrent access from Zig
test "FFI concurrent access" {
    const term = ffi.ghostty_ffi_create_terminal(null);
    defer ffi.ghostty_ffi_destroy_terminal(term);
    
    const Thread = struct {
        fn worker(terminal: ?*ffi.GhosttyTerminal) void {
            for (0..100) |i| {
                const str = std.fmt.allocPrint(test_allocator, "Thread {}", .{i}) catch return;
                defer test_allocator.free(str);
                
                ffi.ghostty_ffi_test_write_string(terminal, str.ptr, str.len);
            }
        }
    };
    
    // Start multiple threads
    var threads: [4]std.Thread = undefined;
    for (&threads) |*thread| {
        thread.* = try std.Thread.spawn(.{}, Thread.worker, .{term});
    }
    
    // Wait for completion
    for (threads) |thread| {
        thread.join();
    }
    
    // Check no errors
    var errors: u32 = 0;
    ffi.ghostty_ffi_test_get_stats(term, null, null, null, &errors);
    try testing.expect(errors == 0);
}

// Test frame queue functionality
test "FFI frame queue" {
    var fixture = TestFixture.init();
    defer fixture.deinit();
    
    fixture.terminal = ffi.ghostty_ffi_create_terminal(&fixture);
    try testing.expect(fixture.terminal != null);
    
    // Create and process multiple frames
    for (0..10) |i| {
        var cell = c.ui_cell_t{
            .codepoint = @intCast('0' + i),
            .fg_rgb = 0xFFFFFF,
            .bg_rgb = 0x000000,
            .attrs = 0,
            .width = 1,
            .cluster_cont = 0,
        };
        
        var span = c.ui_span_t{
            .row = @intCast(i),
            .col_start = 0,
            .col_end = 1,
            .cells = &cell,
            .flags = 0,
        };
        
        var frame = c.ui_frame_t{
            .size = @sizeOf(c.ui_frame_t),
            .frame_seq = i,
            .timestamp_ns = i * 1000000,
            .pane_id = 1,
            .span_count = 1,
            .spans = &span,
            .flags = if (i % 2 == 0) c.UI_FRAME_URGENT else 0,
            .updates_batched = 1,
            .cells_modified = 1,
            .frames_dropped = 0,
        };
        
        const result = ffi.ghostty_ffi_process_frame(fixture.terminal, &frame);
        try testing.expect(result == 0);
    }
    
    // Flush and check stats
    _ = ffi.ghostty_ffi_flush_immediate(fixture.terminal);
    
    var frames: u64 = 0;
    ffi.ghostty_ffi_test_get_stats(fixture.terminal, &frames, null, null, null);
    try testing.expect(frames > 0);
}

// Benchmark FFI overhead
test "FFI performance benchmark" {
    const term = ffi.ghostty_ffi_create_terminal(null);
    defer ffi.ghostty_ffi_destroy_terminal(term);
    
    const iterations = 10000;
    const start = std.time.nanoTimestamp();
    
    for (0..iterations) |i| {
        var cell = c.ui_cell_t{
            .codepoint = 'A',
            .fg_rgb = @intCast(i % 0xFFFFFF),
            .bg_rgb = 0x000000,
            .attrs = @intCast(i % 16),
            .width = 1,
            .cluster_cont = 0,
        };
        
        var span = c.ui_span_t{
            .row = @intCast(i % 24),
            .col_start = @intCast(i % 80),
            .col_end = @intCast((i % 80) + 1),
            .cells = &cell,
            .flags = 0,
        };
        
        var frame = c.ui_frame_t{
            .size = @sizeOf(c.ui_frame_t),
            .frame_seq = i,
            .timestamp_ns = @intCast(start + i),
            .pane_id = 1,
            .span_count = 1,
            .spans = &span,
            .flags = 0,
            .updates_batched = 1,
            .cells_modified = 1,
            .frames_dropped = 0,
        };
        
        _ = ffi.ghostty_ffi_process_frame(term, &frame);
    }
    
    const end = std.time.nanoTimestamp();
    const elapsed_ms = @as(f64, @floatFromInt(end - start)) / 1_000_000;
    const ops_per_sec = @as(f64, iterations) / (elapsed_ms / 1000);
    
    std.debug.print("\nFFI Benchmark: {} ops in {:.2}ms ({:.0} ops/sec)\n", 
                    .{ iterations, elapsed_ms, ops_per_sec });
    
    try testing.expect(ops_per_sec > 10000); // Should handle >10k ops/sec
}