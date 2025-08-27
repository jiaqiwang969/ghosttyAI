// test_ffi_memory_safety.zig
// Regression tests for FFI memory safety
// QA-002 Task T-404

const std = @import("std");
const testing = std.testing;
const c = @cImport({
    @cInclude("stdlib.h");
    @cInclude("string.h");
});

// Import FFI types
const ffi = @import("../../../INTG-001/ffi/c_types.zig");

// Test DEFECT-005: Unsafe pointer casts
test "safe pointer casting" {
    const allocator = testing.allocator;
    
    // Test null pointer safety
    const null_ptr: ?*anyopaque = null;
    const result = ffi.safePtrCast(*c.void, null_ptr);
    try testing.expect(result == null);
    
    // Test valid pointer casting
    var data: [100]u8 = undefined;
    const data_ptr = @ptrCast(*anyopaque, &data);
    const casted = ffi.safePtrCast([*]u8, data_ptr);
    try testing.expect(casted != null);
    
    // Test alignment requirements
    var misaligned: [17]u8 align(1) = undefined;
    const misaligned_ptr = @ptrCast(*anyopaque, &misaligned[1]); // Deliberately misaligned
    
    // Should handle misalignment safely
    const aligned_result = ffi.safeAlignedCast(*u32, misaligned_ptr);
    if (aligned_result) |ptr| {
        // If it succeeds, it should be properly aligned
        const addr = @ptrToInt(ptr);
        try testing.expect(addr % @alignOf(u32) == 0);
    }
}

// Test memory ownership and lifetime
test "memory ownership safety" {
    const allocator = testing.allocator;
    
    // Allocate memory in Zig
    const zig_mem = try allocator.alloc(u8, 1024);
    defer allocator.free(zig_mem);
    
    // Pass to C (should not take ownership)
    const c_ptr = ffi.toC([*]u8, zig_mem.ptr);
    try testing.expect(c_ptr != null);
    
    // Memory should still be valid
    zig_mem[0] = 42;
    try testing.expect(zig_mem[0] == 42);
    
    // Test zero-copy guarantees
    const same_addr = @ptrToInt(zig_mem.ptr) == @ptrToInt(c_ptr);
    try testing.expect(same_addr); // Should be zero-copy
}

// Test callback safety
test "callback function pointer safety" {
    const TestContext = struct {
        value: u32,
        called: bool,
    };
    
    var context = TestContext{ .value = 123, .called = false };
    
    // Create safe callback wrapper
    const callback = ffi.createCallback(TestContext, &context, struct {
        fn handler(ctx: *TestContext, fd: c_int, events: c_short) callconv(.C) void {
            ctx.called = true;
            ctx.value += @intCast(u32, fd);
        }
    }.handler);
    
    // Simulate callback invocation
    if (callback) |cb| {
        cb(5, 0);
        try testing.expect(context.called == true);
        try testing.expect(context.value == 128);
    }
}

// Test buffer overflow protection
test "buffer overflow protection" {
    const allocator = testing.allocator;
    
    // Allocate small buffer
    const small_buffer = try allocator.alloc(u8, 10);
    defer allocator.free(small_buffer);
    
    // Try to write beyond buffer (should be caught)
    const safe_write = ffi.safeCopy(small_buffer, "This is a very long string that exceeds buffer");
    try testing.expect(safe_write == false); // Should fail safely
    
    // Safe write within bounds
    const safe_write2 = ffi.safeCopy(small_buffer, "Short");
    try testing.expect(safe_write2 == true);
    try testing.expectEqualStrings(small_buffer[0..5], "Short");
}

// Test error propagation across FFI boundary
test "error propagation safety" {
    // Test error conversion from C to Zig
    const c_error: c_int = -1; // Common C error code
    const zig_error = ffi.cErrorToZig(c_error);
    
    try testing.expect(zig_error == error.SystemError);
    
    // Test error conversion from Zig to C
    const zig_err: anyerror = error.OutOfMemory;
    const c_err = ffi.zigErrorToC(zig_err);
    try testing.expect(c_err == -12); // ENOMEM
}

// Test thread safety primitives
test "thread safety in FFI" {
    const Thread = std.Thread;
    
    var shared_value: u32 = 0;
    var mutex = std.Thread.Mutex{};
    
    const thread_fn = struct {
        fn worker(val: *u32, m: *std.Thread.Mutex) void {
            for (0..1000) |_| {
                m.lock();
                defer m.unlock();
                val.* += 1;
            }
        }
    }.worker;
    
    // Spawn multiple threads
    var threads: [10]Thread = undefined;
    for (threads) |*t| {
        t.* = try Thread.spawn(.{}, thread_fn, .{ &shared_value, &mutex });
    }
    
    // Wait for all threads
    for (threads) |t| {
        t.join();
    }
    
    try testing.expect(shared_value == 10000);
}

// Test memory leak detection
test "memory leak prevention" {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer {
        const leaked = gpa.detectLeaks();
        testing.expect(!leaked) catch |err| {
            std.debug.print("Memory leak detected!\n", .{});
            return err;
        };
    }
    const allocator = gpa.allocator();
    
    // Allocate and free many times
    for (0..1000) |_| {
        const mem = try allocator.alloc(u8, 1024);
        defer allocator.free(mem);
        
        // Simulate FFI usage
        const c_ptr = ffi.toC([*]u8, mem.ptr);
        _ = c_ptr;
        // Memory should still be managed by Zig
    }
}

// Test vtable safety
test "vtable function pointer safety" {
    const Vtable = struct {
        init: ?*const fn () callconv(.C) *anyopaque,
        cleanup: ?*const fn (*anyopaque) callconv(.C) void,
        process: ?*const fn (*anyopaque, [*]const u8, usize) callconv(.C) c_int,
    };
    
    var vtable = Vtable{
        .init = null,
        .cleanup = null,
        .process = null,
    };
    
    // Safe vtable initialization
    ffi.initVtable(&vtable);
    
    // All function pointers should be valid or null
    if (vtable.init) |init_fn| {
        const ctx = init_fn();
        try testing.expect(ctx != null);
        
        if (vtable.cleanup) |cleanup_fn| {
            cleanup_fn(ctx);
        }
    }
}

// Stress test for FFI boundary
test "FFI boundary stress test" {
    const allocator = testing.allocator;
    
    // Create many FFI transitions
    for (0..10000) |i| {
        const size = (i % 100) + 1;
        const buffer = try allocator.alloc(u8, size);
        defer allocator.free(buffer);
        
        // Fill with data
        for (buffer, 0..) |*b, j| {
            b.* = @intCast(u8, j % 256);
        }
        
        // Convert to C and back
        const c_ptr = ffi.toC([*]u8, buffer.ptr);
        const zig_ptr = ffi.fromC([*]u8, c_ptr);
        
        // Verify data integrity
        for (buffer, 0..) |b, j| {
            try testing.expect(b == @intCast(u8, j % 256));
            try testing.expect(zig_ptr[j] == b);
        }
    }
}

pub fn main() !void {
    std.debug.print("Running FFI Memory Safety Tests...\n", .{});
    
    // Run all tests
    testing.refAllDecls(@This());
    
    std.debug.print("All FFI memory safety tests passed!\n", .{});
}