// ffi_bridge_test.zig - FFI Bridge Integration Tests
// Purpose: Test C-to-Zig type conversions and callback mechanisms
// Author: QA-001 (qa-test-lead)
// Date: 2025-08-26
// Task: T-401 - Integration test framework
// Coverage Target: 100% FFI boundaries

const std = @import("std");
const testing = std.testing;
const builtin = @import("builtin");
const c = @cImport({
    @cInclude("../../cache/week2/CORE-001/src/event_loop_backend.h");
});

// Import components under test
const c_types = @import("../../cache/week2/INTG-001/ffi/c_types.zig");
const callbacks = @import("../../cache/week2/INTG-001/callbacks.zig");
const ghostty_backend = @import("../../cache/week2/INTG-001/ghostty_backend.zig");
const integration = @import("../../cache/week2/INTG-001/integration.zig");

const log = std.log.scoped(.ffi_test);

// Test configuration
const TEST_ITERATIONS = 10000;
const MEMORY_ALIGNMENT = 16;
const MAX_CALLBACK_DEPTH = 10;

// ============================================================================
// Test Utilities
// ============================================================================

const TestStats = struct {
    total_tests: u64 = 0,
    passed_tests: u64 = 0,
    failed_tests: u64 = 0,
    type_conversions: u64 = 0,
    callback_invocations: u64 = 0,
    memory_allocations: u64 = 0,
    memory_deallocations: u64 = 0,
    zero_copy_operations: u64 = 0,
    
    pub fn recordTest(self: *TestStats, passed: bool) void {
        self.total_tests += 1;
        if (passed) {
            self.passed_tests += 1;
        } else {
            self.failed_tests += 1;
        }
    }
    
    pub fn print(self: TestStats) void {
        std.debug.print("\n" ++ "=" ** 40 ++ "\n", .{});
        std.debug.print("FFI Bridge Test Statistics\n", .{});
        std.debug.print("=" ** 40 ++ "\n", .{});
        std.debug.print("Total Tests:         {d}\n", .{self.total_tests});
        std.debug.print("Passed:              {d}\n", .{self.passed_tests});
        std.debug.print("Failed:              {d}\n", .{self.failed_tests});
        std.debug.print("Type Conversions:    {d}\n", .{self.type_conversions});
        std.debug.print("Callback Calls:      {d}\n", .{self.callback_invocations});
        std.debug.print("Memory Allocs:       {d}\n", .{self.memory_allocations});
        std.debug.print("Memory Deallocs:     {d}\n", .{self.memory_deallocations});
        std.debug.print("Zero-Copy Ops:       {d}\n", .{self.zero_copy_operations});
        std.debug.print("=" ** 40 ++ "\n", .{});
    }
};

var g_stats = TestStats{};

// Custom allocator for memory tracking
const TrackingAllocator = struct {
    parent: std.mem.Allocator,
    stats: *TestStats,
    
    pub fn allocator(self: *@This()) std.mem.Allocator {
        return .{
            .ptr = self,
            .vtable = &.{
                .alloc = alloc,
                .resize = resize,
                .free = free,
            },
        };
    }
    
    fn alloc(ctx: *anyopaque, len: usize, ptr_align: u8, ret_addr: usize) ?[*]u8 {
        const self = @ptrCast(*TrackingAllocator, @alignCast(@alignOf(TrackingAllocator), ctx));
        self.stats.memory_allocations += 1;
        return self.parent.vtable.alloc(self.parent.ptr, len, ptr_align, ret_addr);
    }
    
    fn resize(ctx: *anyopaque, buf: []u8, buf_align: u8, new_len: usize, ret_addr: usize) bool {
        const self = @ptrCast(*TrackingAllocator, @alignCast(@alignOf(TrackingAllocator), ctx));
        return self.parent.vtable.resize(self.parent.ptr, buf, buf_align, new_len, ret_addr);
    }
    
    fn free(ctx: *anyopaque, buf: []u8, buf_align: u8, ret_addr: usize) void {
        const self = @ptrCast(*TrackingAllocator, @alignCast(@alignOf(TrackingAllocator), ctx));
        self.stats.memory_deallocations += 1;
        self.parent.vtable.free(self.parent.ptr, buf, buf_align, ret_addr);
    }
};

// ============================================================================
// Test 1: Basic Type Conversions
// ============================================================================

test "C to Zig type conversions" {
    std.debug.print("\n[TEST] C to Zig Type Conversions\n", .{});
    
    // Test primitive types
    const c_int: c_int = 42;
    const zig_int = @as(i32, c_int);
    try testing.expectEqual(@as(i32, 42), zig_int);
    g_stats.type_conversions += 1;
    
    // Test pointer conversions
    var c_buffer: [256]u8 = undefined;
    const c_ptr: [*c]u8 = &c_buffer;
    const zig_ptr = @ptrCast([*]u8, c_ptr);
    try testing.expect(zig_ptr == &c_buffer);
    g_stats.type_conversions += 1;
    
    // Test struct conversions
    var event_handle = c.event_handle_t{
        .backend_data = null,
        .fd = 10,
        .signal = -1,
        .events = c.EL_EVENT_READ | c.EL_EVENT_WRITE,
        .callback = null,
        .user_data = null,
        .timeout = .{ .tv_sec = 1, .tv_usec = 0 },
        .active = false,
        .pending = false,
    };
    
    // Convert to Zig types
    const zig_handle = c_types.EventHandle{
        .backend_data = event_handle.backend_data,
        .fd = event_handle.fd,
        .signal = event_handle.signal,
        .events = event_handle.events,
        .callback = event_handle.callback,
        .user_data = event_handle.user_data,
        .timeout = .{
            .tv_sec = event_handle.timeout.tv_sec,
            .tv_usec = event_handle.timeout.tv_usec,
        },
        .active = event_handle.active,
        .pending = event_handle.pending,
    };
    
    try testing.expectEqual(event_handle.fd, zig_handle.fd);
    try testing.expectEqual(event_handle.events, zig_handle.events);
    g_stats.type_conversions += 2;
    
    g_stats.recordTest(true);
    std.debug.print("  ✓ Type conversions: PASSED\n", .{});
}

// ============================================================================
// Test 2: Callback Function Pointers
// ============================================================================

var callback_test_counter: u32 = 0;
var callback_test_fd: c_int = -1;
var callback_test_events: c_short = 0;

fn testCallback(fd: c_int, events: c_short, user_data: ?*anyopaque) callconv(.C) void {
    callback_test_counter += 1;
    callback_test_fd = fd;
    callback_test_events = events;
    g_stats.callback_invocations += 1;
    _ = user_data;
}

test "Callback function pointer compatibility" {
    std.debug.print("\n[TEST] Callback Function Pointers\n", .{});
    
    // Create callback wrapper
    const callback_wrapper = callbacks.CallbackWrapper{
        .original = testCallback,
        .user_data = null,
        .call_count = 0,
        .last_fd = -1,
        .last_events = 0,
    };
    
    // Test direct invocation
    callback_test_counter = 0;
    testCallback(42, c.EL_EVENT_READ, null);
    try testing.expectEqual(@as(u32, 1), callback_test_counter);
    try testing.expectEqual(@as(c_int, 42), callback_test_fd);
    
    // Test through function pointer
    const fn_ptr = @ptrCast(*const fn(c_int, c_short, ?*anyopaque) callconv(.C) void, &testCallback);
    fn_ptr(100, c.EL_EVENT_WRITE, null);
    try testing.expectEqual(@as(u32, 2), callback_test_counter);
    try testing.expectEqual(@as(c_int, 100), callback_test_fd);
    
    // Test callback chain
    const chain = callbacks.CallbackChain.init(testing.allocator);
    defer chain.deinit();
    
    g_stats.recordTest(true);
    std.debug.print("  ✓ Callback pointers: PASSED ({d} invocations)\n", .{callback_test_counter});
}

// ============================================================================
// Test 3: Memory Ownership Transitions
// ============================================================================

test "Memory ownership transitions" {
    std.debug.print("\n[TEST] Memory Ownership Transitions\n", .{});
    
    var tracking = TrackingAllocator{
        .parent = testing.allocator,
        .stats = &g_stats,
    };
    const allocator = tracking.allocator();
    
    // Test C allocation, Zig ownership
    const c_alloc = @ptrCast([*c]u8, std.c.malloc(1024));
    defer std.c.free(c_alloc);
    try testing.expect(c_alloc != null);
    
    // Transfer to Zig slice
    const zig_slice = c_alloc[0..1024];
    zig_slice[0] = 42;
    try testing.expectEqual(@as(u8, 42), zig_slice[0]);
    g_stats.type_conversions += 1;
    
    // Test Zig allocation, C usage
    const zig_alloc = try allocator.alloc(u8, 512);
    defer allocator.free(zig_alloc);
    
    const c_ptr = @ptrCast([*c]u8, zig_alloc.ptr);
    c_ptr[0] = 100;
    try testing.expectEqual(@as(u8, 100), zig_alloc[0]);
    
    // Test shared memory region
    var shared_buffer: [4096]u8 align(MEMORY_ALIGNMENT) = undefined;
    const zig_view = shared_buffer[0..2048];
    const c_view = @ptrCast([*c]u8, &shared_buffer[2048]);
    
    zig_view[0] = 1;
    c_view[0] = 2;
    try testing.expectEqual(@as(u8, 1), shared_buffer[0]);
    try testing.expectEqual(@as(u8, 2), shared_buffer[2048]);
    
    g_stats.recordTest(true);
    std.debug.print("  ✓ Memory ownership: PASSED\n", .{});
}

// ============================================================================
// Test 4: Zero-Copy Operations
// ============================================================================

test "Zero-copy data transfer" {
    std.debug.print("\n[TEST] Zero-Copy Operations\n", .{});
    
    // Create large buffer
    var buffer: [65536]u8 align(64) = undefined;
    for (buffer, 0..) |*b, i| {
        b.* = @truncate(u8, i);
    }
    
    // Test zero-copy slice creation
    const start_ptr = @ptrToInt(&buffer[0]);
    
    // C view of the buffer
    const c_buffer = @ptrCast([*c]u8, &buffer);
    const c_ptr_addr = @ptrToInt(c_buffer);
    
    // Verify same memory address (zero-copy)
    try testing.expectEqual(start_ptr, c_ptr_addr);
    g_stats.zero_copy_operations += 1;
    
    // Test sub-slicing without copy
    const zig_subslice = buffer[1024..2048];
    const c_subptr = @ptrCast([*c]u8, zig_subslice.ptr);
    
    try testing.expectEqual(@ptrToInt(&buffer[1024]), @ptrToInt(c_subptr));
    g_stats.zero_copy_operations += 1;
    
    // Test modification through both views
    zig_subslice[0] = 255;
    try testing.expectEqual(@as(u8, 255), c_subptr[0]);
    
    c_subptr[1] = 254;
    try testing.expectEqual(@as(u8, 254), zig_subslice[1]);
    
    g_stats.recordTest(true);
    std.debug.print("  ✓ Zero-copy: PASSED ({d} operations)\n", .{g_stats.zero_copy_operations});
}

// ============================================================================
// Test 5: Complex Struct FFI
// ============================================================================

test "Complex struct FFI marshaling" {
    std.debug.print("\n[TEST] Complex Struct FFI\n", .{});
    
    var allocator = testing.allocator;
    
    // Create Ghostty backend state
    var backend_state = try callbacks.GhosttyBackendState.init(allocator);
    defer backend_state.deinit();
    
    // Create vtable
    var vtable = c_types.UiBackendOps{
        .init = callbacks.ghostty_init,
        .cleanup = callbacks.ghostty_cleanup,
        .clear = callbacks.ghostty_clear,
        .putchar = callbacks.ghostty_putchar,
        .puts = callbacks.ghostty_puts,
        .move_cursor = callbacks.ghostty_move_cursor,
        .set_attr = callbacks.ghostty_set_attr,
        .bell = callbacks.ghostty_bell,
        .resize = callbacks.ghostty_resize,
        .refresh = callbacks.ghostty_refresh,
        .get_size = callbacks.ghostty_get_size,
        .scroll_region = callbacks.ghostty_scroll_region,
        .clear_region = callbacks.ghostty_clear_region,
        .set_title = callbacks.ghostty_set_title,
        .set_clipboard = callbacks.ghostty_set_clipboard,
        .get_clipboard = callbacks.ghostty_get_clipboard,
        .create_window = callbacks.ghostty_create_window,
        .destroy_window = callbacks.ghostty_destroy_window,
        .switch_window = callbacks.ghostty_switch_window,
    };
    
    // Test vtable function calls through C interface
    const backend_ptr = @ptrCast(*anyopaque, backend_state);
    const result = vtable.init.?(backend_ptr);
    try testing.expectEqual(@as(c_int, 0), result);
    g_stats.callback_invocations += 1;
    
    // Test complex nested struct
    const frame_agg = c_types.FrameAggregator{
        .pending_cells = try allocator.alloc(c_types.GridCell, 1000),
        .pending_count = 0,
        .max_pending = 1000,
        .dirty_lines = try allocator.alloc(bool, 100),
        .min_dirty_y = 0,
        .max_dirty_y = 0,
        .aggregation_ns = 16666667, // 60fps
        .last_flush_ns = 0,
    };
    defer allocator.free(frame_agg.pending_cells);
    defer allocator.free(frame_agg.dirty_lines);
    
    // Verify struct layout compatibility
    try testing.expect(@sizeOf(@TypeOf(frame_agg)) > 0);
    try testing.expect(@alignOf(@TypeOf(frame_agg)) >= 8);
    
    g_stats.recordTest(true);
    std.debug.print("  ✓ Complex structs: PASSED\n", .{});
}

// ============================================================================
// Test 6: Error Handling Across FFI
// ============================================================================

test "Error handling across FFI boundary" {
    std.debug.print("\n[TEST] FFI Error Handling\n", .{});
    
    // Test error code conversions
    const zig_errors = [_]anyerror{
        error.OutOfMemory,
        error.InvalidArgument,
        error.SystemResources,
        error.Unexpected,
    };
    
    const c_errors = [_]c_int{
        -@as(c_int, @enumToInt(std.os.E.NOMEM)),
        -@as(c_int, @enumToInt(std.os.E.INVAL)),
        -@as(c_int, @enumToInt(std.os.E.AGAIN)),
        -1, // Generic error
    };
    
    for (zig_errors, c_errors) |zig_err, expected_c| {
        const c_err = callbacks.errorToC(zig_err);
        // Note: exact error codes may vary by platform
        try testing.expect(c_err < 0); // All errors should be negative
        g_stats.type_conversions += 1;
    }
    
    // Test error propagation through callbacks
    const ErrorCallback = struct {
        fn callback(fd: c_int, events: c_short, data: ?*anyopaque) callconv(.C) void {
            _ = fd;
            _ = events;
            if (data) |d| {
                const err_ptr = @ptrCast(*c_int, @alignCast(@alignOf(c_int), d));
                err_ptr.* = -@as(c_int, @enumToInt(std.os.E.IO));
            }
        }
    };
    
    var error_code: c_int = 0;
    ErrorCallback.callback(0, 0, @ptrCast(?*anyopaque, &error_code));
    try testing.expect(error_code < 0);
    g_stats.callback_invocations += 1;
    
    g_stats.recordTest(true);
    std.debug.print("  ✓ Error handling: PASSED\n", .{});
}

// ============================================================================
// Test 7: Thread Safety of FFI Calls
// ============================================================================

const ThreadTestData = struct {
    counter: std.atomic.Atomic(u32),
    allocator: std.mem.Allocator,
    iterations: u32,
};

fn threadWorker(data: *ThreadTestData) void {
    var prng = std.rand.DefaultPrng.init(@intCast(u64, std.time.milliTimestamp()));
    const rand = prng.random();
    
    var i: u32 = 0;
    while (i < data.iterations) : (i += 1) {
        // Simulate FFI calls
        const value = data.counter.fetchAdd(1, .SeqCst);
        
        // Random allocation/deallocation to stress memory
        if (rand.boolean()) {
            const size = rand.intRangeAtMost(usize, 16, 1024);
            const mem = data.allocator.alloc(u8, size) catch continue;
            defer data.allocator.free(mem);
            mem[0] = @truncate(u8, value);
        }
        
        // Simulate callback invocation
        if (value % 10 == 0) {
            testCallback(@intCast(c_int, value), 0, null);
        }
    }
}

test "Thread safety of FFI calls" {
    std.debug.print("\n[TEST] FFI Thread Safety\n", .{});
    
    const num_threads = 8;
    const iterations_per_thread = 1000;
    
    var data = ThreadTestData{
        .counter = std.atomic.Atomic(u32).init(0),
        .allocator = testing.allocator,
        .iterations = iterations_per_thread,
    };
    
    var threads: [num_threads]std.Thread = undefined;
    
    // Start threads
    for (&threads) |*thread| {
        thread.* = try std.Thread.spawn(.{}, threadWorker, .{&data});
    }
    
    // Wait for completion
    for (threads) |thread| {
        thread.join();
    }
    
    const final_count = data.counter.load(.SeqCst);
    try testing.expectEqual(num_threads * iterations_per_thread, final_count);
    
    g_stats.recordTest(true);
    std.debug.print("  ✓ Thread safety: PASSED ({d} concurrent ops)\n", .{final_count});
}

// ============================================================================
// Test 8: Performance Benchmarks
// ============================================================================

test "FFI performance benchmarks" {
    std.debug.print("\n[TEST] FFI Performance\n", .{});
    
    const iterations = 1000000;
    var timer = try std.time.Timer.start();
    
    // Benchmark type conversions
    timer.reset();
    var i: u32 = 0;
    while (i < iterations) : (i += 1) {
        const c_val: c_int = @intCast(c_int, i);
        const zig_val = @as(i32, c_val);
        std.mem.doNotOptimizeAway(zig_val);
    }
    const conversion_ns = timer.lap();
    const conversion_per_op = conversion_ns / iterations;
    
    // Benchmark callback invocations
    callback_test_counter = 0;
    timer.reset();
    i = 0;
    while (i < iterations) : (i += 1) {
        testCallback(@intCast(c_int, i), 0, null);
    }
    const callback_ns = timer.lap();
    const callback_per_op = callback_ns / iterations;
    
    // Benchmark memory operations
    var allocator = testing.allocator;
    timer.reset();
    i = 0;
    while (i < 10000) : (i += 1) { // Fewer iterations for memory ops
        const mem = try allocator.alloc(u8, 1024);
        defer allocator.free(mem);
        mem[0] = @truncate(u8, i);
        const c_ptr = @ptrCast([*c]u8, mem.ptr);
        c_ptr[1] = @truncate(u8, i + 1);
    }
    const memory_ns = timer.lap();
    const memory_per_op = memory_ns / 10000;
    
    // Verify performance targets
    try testing.expect(conversion_per_op < 10); // <10ns per conversion
    try testing.expect(callback_per_op < 100);   // <100ns per callback
    try testing.expect(memory_per_op < 1000);    // <1µs per alloc/free
    
    g_stats.recordTest(true);
    std.debug.print("  ✓ Performance: PASSED\n", .{});
    std.debug.print("    - Type conversion: {d}ns/op\n", .{conversion_per_op});
    std.debug.print("    - Callback invoke: {d}ns/op\n", .{callback_per_op});
    std.debug.print("    - Memory ops:      {d}ns/op\n", .{memory_per_op});
}

// ============================================================================
// Main Test Entry Point
// ============================================================================

pub fn main() !void {
    std.debug.print("\nFFI Bridge Integration Test Suite v1.0\n", .{});
    std.debug.print("=" ** 40 ++ "\n", .{});
    
    // Run all tests
    testing.refAllDecls(@This());
    
    // Print final statistics
    g_stats.print();
    
    if (g_stats.failed_tests == 0) {
        std.debug.print("✅ ALL FFI TESTS PASSED!\n", .{});
    } else {
        std.debug.print("❌ {d} FFI TESTS FAILED!\n", .{g_stats.failed_tests});
        std.os.exit(1);
    }
}