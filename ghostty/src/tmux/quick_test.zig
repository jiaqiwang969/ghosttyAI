const std = @import("std");

// Minimal test to verify FFI works
extern fn tmc_init() c_int;
extern fn tmc_cleanup() void;
extern fn tmc_get_version() u32;

pub fn main() !void {
    std.debug.print("=== Quick tmux FFI Test ===\n", .{});
    
    const result = tmc_init();
    if (result == 0) {
        std.debug.print("✓ tmc_init successful\n", .{});
    } else {
        std.debug.print("✗ tmc_init failed: {}\n", .{result});
    }
    
    const version = tmc_get_version();
    std.debug.print("✓ Version: 0x{x:0>8}\n", .{version});
    
    tmc_cleanup();
    std.debug.print("✓ Cleanup successful\n", .{});
}
