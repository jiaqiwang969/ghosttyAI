// ffi_safety.zig - FFI safety utilities for tmux integration
// Purpose: Provide safe FFI boundary crossing utilities
// Date: 2025-08-26
// Task: T-303-R - Create Ghostty tmux integration module

const std = @import("std");

/// Safe C string conversion
pub fn toCString(allocator: std.mem.Allocator, zig_str: []const u8) ![:0]const u8 {
    return allocator.dupeZ(u8, zig_str);
}

/// Safe C string to Zig slice conversion
pub fn fromCString(c_str: [*:0]const u8) []const u8 {
    return std.mem.span(c_str);
}

/// Safe nullable pointer handling
pub fn safeUnwrap(comptime T: type, ptr: ?*T) ?*T {
    return ptr orelse return null;
}

/// Safe callback context casting
pub fn castUserData(comptime T: type, user_data: ?*anyopaque) ?*T {
    if (user_data == null) return null;
    return @ptrCast(*T, @alignCast(@alignOf(T), user_data.?));
}

/// Memory fence for FFI boundary
pub inline fn memoryFence() void {
    @fence(.SeqCst);
}

/// Safe array bounds checking for FFI
pub fn checkBounds(index: usize, len: usize) bool {
    return index < len;
}

/// Zero-initialize a struct for C compatibility
pub fn zeroInit(comptime T: type) T {
    return std.mem.zeroes(T);
}

/// Validate UTF-8 from C strings
pub fn validateUtf8(data: []const u8) bool {
    _ = std.unicode.utf8ValidateSlice(data) catch return false;
    return true;
}

/// Safe integer conversion for FFI
pub fn safeIntCast(comptime DestType: type, value: anytype) !DestType {
    const info = @typeInfo(DestType);
    if (info != .Int) {
        @compileError("DestType must be an integer type");
    }
    
    const src_type = @TypeOf(value);
    const src_info = @typeInfo(src_type);
    if (src_info != .Int) {
        @compileError("Source value must be an integer");
    }
    
    // Check for overflow
    if (value > std.math.maxInt(DestType) or value < std.math.minInt(DestType)) {
        return error.Overflow;
    }
    
    return @intCast(DestType, value);
}

test "FFI safety utilities" {
    const allocator = std.testing.allocator;
    
    // Test C string conversion
    const c_str = try toCString(allocator, "test");
    defer allocator.free(c_str);
    try std.testing.expectEqualStrings("test", c_str);
    
    // Test UTF-8 validation
    try std.testing.expect(validateUtf8("valid utf-8"));
    try std.testing.expect(!validateUtf8("\xFF\xFE"));
    
    // Test safe int cast
    const small: i32 = 42;
    const casted = try safeIntCast(u16, small);
    try std.testing.expectEqual(@as(u16, 42), casted);
}