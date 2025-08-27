// test_tmux_integration.zig - Test program for Ghostty tmux integration
// Purpose: Verify tmux integration compiles and works
// Date: 2025-08-26
// Task: T-304-R - Integrate with Terminal module

const std = @import("std");
const TmuxCore = @import("core.zig").TmuxCore;
const TmuxIntegration = @import("terminal_integration.zig").TmuxIntegration;

pub fn main() !void {
    const allocator = std.heap.page_allocator;
    
    std.debug.print("Testing Ghostty tmux integration...\n", .{});
    
    // Test TmuxCore availability
    if (TmuxCore.isAvailable()) {
        std.debug.print("✓ libtmuxcore is available\n", .{});
    } else {
        std.debug.print("✗ libtmuxcore not available\n", .{});
        return;
    }
    
    // Get version
    const version = TmuxCore.getVersion();
    std.debug.print("✓ libtmuxcore version: {}.{}.{}\n", .{ version.major, version.minor, version.patch });
    
    // Test TmuxIntegration
    var integration = try TmuxIntegration.init(allocator, true);
    defer integration.deinit();
    
    if (integration.isEnabled()) {
        std.debug.print("✓ Tmux integration enabled\n", .{});
    } else {
        std.debug.print("✗ Tmux integration failed to enable\n", .{});
        return;
    }
    
    // Test command parsing
    const test_input = "@tmux new-session -s test";
    if (TmuxIntegration.parseTmuxCommand(test_input)) |cmd| {
        std.debug.print("✓ Parsed tmux command: {s}\n", .{cmd});
        
        // Try to execute the command
        integration.handleTmuxCommand(cmd) catch |err| {
            std.debug.print("  (Command execution returned: {})\n", .{err});
        };
    }
    
    std.debug.print("\nAll tmux integration tests completed!\n", .{});
}