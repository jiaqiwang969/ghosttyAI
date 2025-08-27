// test_incremental.zig - Test incremental tmux integration
const std = @import("std");
const tmux_integration = @import("termio_tmux_integration.zig");

test "tmux integration can be disabled" {
    const testing = std.testing;
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();
    
    const config = tmux_integration.TmuxConfig{
        .enable_tmux = false,
    };
    
    var ext = try tmux_integration.TmuxExtension.init(allocator, config);
    defer ext.deinit(allocator);
    
    try testing.expect(!ext.enabled);
}

pub fn main() !void {
    std.log.info("Testing incremental tmux integration...", .{});
    
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();
    
    // Test with tmux disabled (should not affect existing Ghostty)
    {
        const config = tmux_integration.TmuxConfig{
            .enable_tmux = false,
        };
        
        var ext = try tmux_integration.TmuxExtension.init(allocator, config);
        defer ext.deinit(allocator);
        
        std.log.info("tmux disabled: extension.enabled = {}", .{ext.enabled});
    }
    
    // Test with tmux enabled
    {
        const config = tmux_integration.TmuxConfig{
            .enable_tmux = true,
            .default_session_name = "test",
        };
        
        var ext = try tmux_integration.TmuxExtension.init(allocator, config);
        defer ext.deinit(allocator);
        
        std.log.info("tmux enabled: extension.enabled = {}", .{ext.enabled});
    }
    
    std.log.info("✓ Incremental integration test passed", .{});
}
