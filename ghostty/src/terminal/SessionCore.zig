//! SessionCore - 持久会话核心（骨架）
//! 目标：由其真正拥有 Terminal/PTY/进程；当前仅实现最小可编译骨架与自有 Terminal。

pub const SessionCore = @This();

const std = @import("std");
const Allocator = std.mem.Allocator;
// Import terminal public API via directory main
const terminalpkg = @import("main.zig");

/// 会话标识
id: []const u8,

/// 自有 Terminal（后续扩展到 PTY/进程）
owned_terminal: terminalpkg.Terminal,

/// 附加的查看器（Surface 指针，避免循环依赖，此处用 anyopaque）
attached_surfaces: std.ArrayListUnmanaged(*anyopaque) = .{},

/// 预留：PTY 与子进程（占位，不启用）
has_pty: bool = false,

pub const InitError = anyerror;

pub fn init(alloc: Allocator, id: []const u8, cols: u16, rows: u16) InitError!*SessionCore {
    const core = try alloc.create(SessionCore);
    errdefer alloc.destroy(core);

    // 初始化一个最小可用终端（尺寸参数可由调用方传入）
    var term = try terminalpkg.Terminal.init(alloc, .{ .cols = cols, .rows = rows });
    errdefer term.deinit(alloc);

    core.* = .{
        .id = try alloc.dupe(u8, id),
        .owned_terminal = term,
        .attached_surfaces = .{},
        .has_pty = false,
    };
    return core;
}

pub fn deinit(self: *SessionCore, alloc: Allocator) void {
    self.owned_terminal.deinit(alloc);
    if (self.attached_surfaces.capacity > 0) self.attached_surfaces.deinit(alloc);
    alloc.free(self.id);
    alloc.destroy(self);
}

/// 返回用于查看的 Terminal 指针
pub fn getTerminalForViewing(self: *SessionCore) *terminalpkg.Terminal {
    return &self.owned_terminal;
}

/// 附加/分离查看器（占位实现）
pub fn attachViewer(self: *SessionCore, alloc: Allocator, surface_ptr: *anyopaque) Allocator.Error!void {
    try self.attached_surfaces.append(alloc, surface_ptr);
}

pub fn detachViewer(self: *SessionCore, surface_ptr: *anyopaque) void {
    var i: usize = 0;
    while (i < self.attached_surfaces.items.len) {
        if (self.attached_surfaces.items[i] == surface_ptr) {
            _ = self.attached_surfaces.swapRemove(i);
            continue;
        }
        i += 1;
    }
}


