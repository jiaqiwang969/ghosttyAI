const std = @import("std");
const Allocator = std.mem.Allocator;

/// SessionManager - Ghostty终端间通信的核心管理器
/// 负责管理所有终端会话并路由消息
pub const SessionManager = @This();

const SessionId = []const u8;
const SessionMap = std.StringHashMap(*SessionInfo);
const CoreMap = std.StringHashMap(*SessionCore);
const LinkList = std.ArrayList(SessionLink);
const PointerMap = std.AutoHashMap(usize, []const u8);  // Map surface pointer to session name
const SessionCore = @import("SessionCore.zig").SessionCore;
const ViewersMap = std.StringHashMap(std.ArrayListUnmanaged(*anyopaque));
const ViewerPointerMap = std.AutoHashMap(usize, []const u8); // viewer surface ptr -> target session id

/// 会话信息
const SessionInfo = struct {
    id: SessionId,  // User-friendly name like "main", "dev", etc.
    surface_ptr: *anyopaque,  // 实际是 *Surface，这里用anyopaque避免循环依赖
    created_time: i64,
    is_remote: bool = false,
    auto_generated: bool = false,  // Whether this name was auto-generated
    
    // 会话统计
    messages_sent: u64 = 0,
    messages_received: u64 = 0,
};

/// 会话链接关系
const SessionLink = struct {
    source: SessionId,
    target: SessionId,
    bidirectional: bool = false,
    created_time: i64,
    
    // 可选的消息过滤器
    filter: ?*const fn (data: []const u8) bool = null,
};

/// 消息类型
pub const Message = struct {
    from: SessionId,
    to: SessionId,
    data: []const u8,
    timestamp: i64,
    wait_response: bool = false,
};

// SessionManager 字段
allocator: Allocator,
sessions: SessionMap,
cores: CoreMap,
pointer_map: PointerMap,  // Quick lookup by pointer
links: LinkList,
message_queue: std.ArrayList(Message),
mutex: std.Thread.Mutex,
next_auto_id: u32,  // For auto-generated names

// 附着关系：一个会话可被多个查看器(Surface)附着
viewers: ViewersMap,
viewer_pointer_map: ViewerPointerMap,

pub fn init(allocator: Allocator) SessionManager {
    return .{
        .allocator = allocator,
        .sessions = SessionMap.init(allocator),
        .cores = CoreMap.init(allocator),
        .pointer_map = PointerMap.init(allocator),
        .links = LinkList.init(allocator),
        .message_queue = std.ArrayList(Message).init(allocator),
        .mutex = std.Thread.Mutex{},
        .next_auto_id = 0,
        .viewers = ViewersMap.init(allocator),
        .viewer_pointer_map = ViewerPointerMap.init(allocator),
    };
}

pub fn deinit(self: *SessionManager) void {
    // 清理所有会话信息
    var iter = self.sessions.iterator();
    while (iter.next()) |entry| {
        self.allocator.free(entry.key_ptr.*);
        self.allocator.destroy(entry.value_ptr.*);
    }
    self.sessions.deinit();
    // 清理 cores
    var citer = self.cores.iterator();
    while (citer.next()) |entry| {
        entry.value_ptr.*.deinit(self.allocator);
        self.allocator.free(entry.key_ptr.*);
    }
    self.cores.deinit();
    self.pointer_map.deinit();
    // 清理 viewers
    var viter = self.viewers.iterator();
    while (viter.next()) |entry| {
        if (entry.value_ptr.*.capacity > 0) entry.value_ptr.*.deinit(self.allocator);
        self.allocator.free(entry.key_ptr.*);
    }
    self.viewers.deinit();
    self.viewer_pointer_map.deinit();
    
    // 清理链接
    for (self.links.items) |link| {
        self.allocator.free(link.source);
        self.allocator.free(link.target);
    }
    self.links.deinit();
    
    // 清理消息队列
    for (self.message_queue.items) |msg| {
        self.allocator.free(msg.from);
        self.allocator.free(msg.to);
        self.allocator.free(msg.data);
    }
    self.message_queue.deinit();
}

/// 注册新会话 (支持用户自定义名称)
pub fn registerSession(
    self: *SessionManager,
    id_optional: ?[]const u8,  // null means auto-generate
    surface_ptr: *anyopaque,
    is_remote: bool,
) ![]const u8 {  // Returns the actual session ID used
    self.mutex.lock();
    defer self.mutex.unlock();
    
    // Generate or use provided ID
    const id = if (id_optional) |user_id| blk: {
        // Check if name already exists
        if (self.sessions.contains(user_id)) {
            return error.SessionNameExists;
        }
        break :blk try self.allocator.dupe(u8, user_id);
    } else blk: {
        // Auto-generate a simple name
        var buf: [16]u8 = undefined;
        const auto_id = try std.fmt.bufPrint(&buf, "s{d}", .{self.next_auto_id});
        self.next_auto_id += 1;
        break :blk try self.allocator.dupe(u8, auto_id);
    };
    errdefer self.allocator.free(id);
    
    // 创建会话信息
    const info = try self.allocator.create(SessionInfo);
    info.* = .{
        .id = id,
        .surface_ptr = surface_ptr,
        .created_time = std.time.milliTimestamp(),
        .is_remote = is_remote,
        .auto_generated = (id_optional == null),
    };
    
    try self.sessions.put(id, info);
    try self.pointer_map.put(@intFromPtr(surface_ptr), id);

    // 确保 SessionCore 存在（以默认尺寸创建骨架）
    if (!self.cores.contains(id)) {
        var core = try SessionCore.init(self.allocator, id, 80, 24);
        errdefer core.deinit(self.allocator);
        try self.cores.put(try self.allocator.dupe(u8, id), core);
    }
    
    std.log.info("Registered session: {s} (remote: {any})", .{ 
        id, 
        is_remote 
    });
    
    return id;
}

/// Get session name by surface pointer
pub fn getSessionByPointer(self: *SessionManager, surface_ptr: *anyopaque) ?[]const u8 {
    self.mutex.lock();
    defer self.mutex.unlock();
    
    return self.pointer_map.get(@intFromPtr(surface_ptr));
}

/// 注销会话
pub fn unregisterSession(self: *SessionManager, id: []const u8) void {
    self.mutex.lock();
    defer self.mutex.unlock();
    
    if (self.sessions.fetchRemove(id)) |entry| {
        // Remove from pointer map
        _ = self.pointer_map.remove(@intFromPtr(entry.value.surface_ptr));
        
        // 删除相关的链接
        var i: usize = 0;
        while (i < self.links.items.len) {
            const link = self.links.items[i];
            if (std.mem.eql(u8, link.source, id) or 
                std.mem.eql(u8, link.target, id)) {
                _ = self.links.swapRemove(i);
                self.allocator.free(link.source);
                self.allocator.free(link.target);
            } else {
                i += 1;
            }
        }
        
        self.allocator.free(entry.key);
        self.allocator.destroy(entry.value);
        
        std.log.info("Unregistered session: {s}", .{id});
    }
}

/// 通过会话ID获取对应的 Surface 指针（anyopaque）。找不到返回 null。
pub fn getSurfaceById(self: *SessionManager, id: []const u8) ?*anyopaque {
    self.mutex.lock();
    defer self.mutex.unlock();

    if (self.sessions.get(id)) |info| {
        return info.surface_ptr;
    }
    return null;
}

/// 获取/创建 SessionCore
pub fn getOrCreateCore(self: *SessionManager, id: []const u8) !*SessionCore {
    self.mutex.lock();
    defer self.mutex.unlock();
    if (self.cores.get(id)) |core| return core;
    var core = try SessionCore.init(self.allocator, id, 80, 24);
    errdefer core.deinit(self.allocator);
    try self.cores.put(try self.allocator.dupe(u8, id), core);
    return core;
}

/// 将某个查看器附着到指定会话（仅登记关系，不做渲染/IO）
pub fn attachViewer(self: *SessionManager, session_id: []const u8, viewer_surface: *anyopaque) !void {
    self.mutex.lock();
    defer self.mutex.unlock();

    // Ensure viewer list exists and get a pointer to it
    var list_ptr: *std.ArrayListUnmanaged(*anyopaque) = blk: {
        if (self.viewers.getPtr(session_id)) |p| break :blk p;
        const sid_copy = try self.allocator.dupe(u8, session_id);
        try self.viewers.put(sid_copy, .{});
        break :blk self.viewers.getPtr(sid_copy).?;
    };

    // Append viewer to list
    try list_ptr.append(self.allocator, viewer_surface);
    // Update maps
    try self.viewer_pointer_map.put(@intFromPtr(viewer_surface), try self.allocator.dupe(u8, session_id));
}

/// 将查看器从其附着的会话上分离
pub fn detachViewer(self: *SessionManager, viewer_surface: *anyopaque) void {
    self.mutex.lock();
    defer self.mutex.unlock();

    const sid = self.viewer_pointer_map.fetchRemove(@intFromPtr(viewer_surface)) orelse return;
    const session_id = sid.value;
    if (self.viewers.get(session_id)) |*list| {
        var i: usize = 0;
        while (i < list.items.len) {
            if (list.items[i] == viewer_surface) {
                _ = list.swapRemove(i);
                continue;
            }
            i += 1;
        }
        if (list.items.len == 0) {
            _ = self.viewers.remove(session_id);
            self.allocator.free(session_id);
        }
    }
}

/// 发送消息到指定会话
pub fn sendToSession(
    self: *SessionManager,
    from_id: []const u8,
    target_id: []const u8,
    data: []const u8,
    wait_response: bool,
) !void {
    self.mutex.lock();
    defer self.mutex.unlock();
    
    // 查找目标会话
    const target_info = self.sessions.get(target_id) orelse {
        std.log.warn("Target session not found: {s}", .{target_id});
        return error.SessionNotFound;
    };
    
    // 更新统计
    if (self.sessions.get(from_id)) |from_info| {
        from_info.messages_sent += 1;
    }
    target_info.messages_received += 1;
    
    // 创建消息
    const msg = Message{
        .from = try self.allocator.dupe(u8, from_id),
        .to = try self.allocator.dupe(u8, target_id),
        .data = try self.allocator.dupe(u8, data),
        .timestamp = std.time.milliTimestamp(),
        .wait_response = wait_response,
    };
    
    try self.message_queue.append(msg);
    
    std.log.debug("Message queued: {s} -> {s} ({d} bytes)", .{
        from_id,
        target_id,
        data.len,
    });
    
    // 这里应该触发实际的消息发送
    // 在实际实现中，这会调用 Surface 的方法来写入 PTY
    try self.deliverMessage(msg);
}

/// 建立会话间的链接
pub fn linkSessions(
    self: *SessionManager,
    source_id: []const u8,
    target_id: []const u8,
    bidirectional: bool,
) !void {
    self.mutex.lock();
    defer self.mutex.unlock();
    
    // 验证两个会话都存在
    if (self.sessions.get(source_id) == null) {
        return error.SourceSessionNotFound;
    }
    if (self.sessions.get(target_id) == null) {
        return error.TargetSessionNotFound;
    }
    
    // 检查是否已经存在链接
    for (self.links.items) |link| {
        if (std.mem.eql(u8, link.source, source_id) and
            std.mem.eql(u8, link.target, target_id)) {
            return error.LinkAlreadyExists;
        }
    }
    
    // 创建新链接
    const link = SessionLink{
        .source = try self.allocator.dupe(u8, source_id),
        .target = try self.allocator.dupe(u8, target_id),
        .bidirectional = bidirectional,
        .created_time = std.time.milliTimestamp(),
    };
    
    try self.links.append(link);
    
    std.log.info("Sessions linked: {s} {s} {s}", .{
        source_id,
        if (bidirectional) "<->" else "->",
        target_id,
    });
}

/// 路由输出到链接的会话
pub fn routeOutput(
    self: *SessionManager,
    from_session: []const u8,
    output: []const u8,
) !void {
    self.mutex.lock();
    defer self.mutex.unlock();
    
    for (self.links.items) |link| {
        // 检查是否需要转发
        var should_forward = false;
        var target: ?[]const u8 = null;
        
        if (std.mem.eql(u8, link.source, from_session)) {
            should_forward = true;
            target = link.target;
        } else if (link.bidirectional and 
                   std.mem.eql(u8, link.target, from_session)) {
            should_forward = true;
            target = link.source;
        }
        
        if (should_forward and target != null) {
            // 应用过滤器（如果有）
            if (link.filter) |filter| {
                if (!filter(output)) continue;
            }
            
            try self.sendToSession(from_session, target.?, output, false);
        }
    }
}

/// 列出所有活动会话
pub fn listSessions(self: *SessionManager, writer: anytype) !void {
    self.mutex.lock();
    defer self.mutex.unlock();
    
    try writer.print("Active Sessions:\n", .{});
    try writer.print("================\n", .{});
    
    var iter = self.sessions.iterator();
    while (iter.next()) |entry| {
        const info = entry.value_ptr.*;
        const age_ms = std.time.milliTimestamp() - info.created_time;
        const age_sec = @divFloor(age_ms, 1000);
        
        try writer.print("  {s} [{s}{s}] - Age: {d}s, Sent: {d}, Recv: {d}\n", .{
            info.id,
            if (info.is_remote) "R" else "L",
            if (self.isSessionLinked(info.id)) "*" else " ",
            age_sec,
            info.messages_sent,
            info.messages_received,
        });
    }
    
    if (self.links.items.len > 0) {
        try writer.print("\nActive Links:\n", .{});
        try writer.print("=============\n", .{});
        for (self.links.items) |link| {
            try writer.print("  {s} {s} {s}\n", .{
                link.source,
                if (link.bidirectional) "<->" else "->",
                link.target,
            });
        }
    }
}

/// 检查会话是否被链接
fn isSessionLinked(self: *SessionManager, id: []const u8) bool {
    for (self.links.items) |link| {
        if (std.mem.eql(u8, link.source, id) or
            std.mem.eql(u8, link.target, id)) {
            return true;
        }
    }
    return false;
}

/// 实际投递消息（调用Surface方法）
fn deliverMessage(self: *SessionManager, msg: Message) !void {
    // Get target session info  
    const target_info = self.sessions.get(msg.to) orelse {
        return error.SessionNotFound;
    };
    
    // Import Surface type to call its methods
    const Surface = @import("../Surface.zig");
    const surface = @as(*Surface, @ptrCast(@alignCast(target_info.surface_ptr)));
    
    // Import termio for creating proper message format
    const termio = @import("../termio.zig");
    
    // Check if the message is a @ghostty command
    const CMD_PREFIX = "@ghostty";
    if (std.mem.startsWith(u8, msg.data, CMD_PREFIX)) {
        // This is a @ghostty command - inject it as if it was typed
        // We need to trigger the command capture mechanism
        
        // First, show that we received the command
        const terminal_ptr = @import("../terminal/main.zig");
        
        // Lock the terminal and display a message
        surface.renderer_state.mutex.lock();
        defer surface.renderer_state.mutex.unlock();
        const t: *terminal_ptr.Terminal = surface.renderer_state.terminal;
        
        // Show received command notification
        t.carriageReturn();
        t.linefeed() catch {};
        t.printString("Received from ") catch {};
        t.printString(msg.from) catch {};
        t.printString(": ") catch {};
        t.linefeed() catch {};
        
        // Now inject the command into the command buffer as if typed
        // This simulates the user typing the command
        surface.command_buffer.clearRetainingCapacity();
        surface.command_buffer.appendSlice(msg.data) catch {};
        
        // Process the command directly (similar to what happens on Enter key)
        const line = std.mem.trim(u8, msg.data, " \r\n");
        
        if (std.mem.startsWith(u8, line, CMD_PREFIX ++ " ")) {
            // Extract and process the subcommand
            const subcmd = line[CMD_PREFIX.len + 1..];
            
            // Process different subcommands
            if (std.mem.startsWith(u8, subcmd, "send ")) {
                // Nested send command - be careful to avoid infinite loops!
                // For now, just display it
                t.printString("  (nested @ghostty send - displaying only)") catch {};
                t.linefeed() catch {};
                t.printString("  Command: ") catch {};
                t.printString(subcmd) catch {};
                t.linefeed() catch {};
            } else if (std.mem.startsWith(u8, subcmd, "session")) {
                // Session command - process it
                t.printString("  Processing session command...") catch {};
                t.linefeed() catch {};
                // Could trigger actual session registration here
            } else {
                // Other commands
                t.printString("  Command: ") catch {};
                t.printString(subcmd) catch {};
                t.linefeed() catch {};
            }
        }
        
        // Clear command buffer after processing
        surface.command_buffer.clearRetainingCapacity();
    } else {
        // Regular message - write to PTY as before
        const write_msg = try termio.Message.writeReq(self.allocator, msg.data);
        surface.io.queueMessage(write_msg, .unlocked);
    }
    
    std.log.debug("Message delivered to {s}: {s}", .{
        msg.to,
        msg.data,
    });
}

// ============= 测试代码 =============

test "SessionManager basic operations" {
    const allocator = std.testing.allocator;
    
    var manager = SessionManager.init(allocator);
    defer manager.deinit();
    
    // 模拟的Surface指针
    var surface_a: u32 = 1;
    var surface_b: u32 = 2;
    
    // 注册两个会话 with custom names
    const id_a = try manager.registerSession("main", &surface_a, false);
    const id_b = try manager.registerSession("dev", &surface_b, false);
    
    // Auto-generated name
    const id_c = try manager.registerSession(null, &surface_a, false);
    _ = id_c;
    
    // 发送消息
    try manager.sendToSession(id_a, id_b, "Hello from main", false);
    
    // 建立双向链接
    try manager.linkSessions(id_a, id_b, true);
    
    // 路由输出
    try manager.routeOutput(id_a, "ls -la");
    
    // 列出会话
    const stdout = std.io.getStdOut().writer();
    try manager.listSessions(stdout);
}

test "SessionManager remote session" {
    const allocator = std.testing.allocator;
    
    var manager = SessionManager.init(allocator);
    defer manager.deinit();
    
    var local_surface: u32 = 1;
    var remote_surface: u32 = 2;
    
    // 注册本地和远程会话
    try manager.registerSession("local", &local_surface, false);
    try manager.registerSession("remote-ssh", &remote_surface, true);
    
    // 建立到远程的链接
    try manager.linkSessions("local", "remote-ssh", true);
    
    // 发送命令到远程
    try manager.sendToSession("local", "remote-ssh", "pwd\n", false);
}