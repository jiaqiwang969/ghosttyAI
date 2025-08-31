//! SessionRedrawDispatcher: 基于 xev 的会话级重绘分发器
//! 作用：合并短时间内的多次重绘请求，按“聚焦优先→其他”顺序投递 App 消息

const Dispatcher = @This();

const std = @import("std");
const xev = @import("../global.zig").xev;
const App = @import("../App.zig");

const Allocator = std.mem.Allocator;
const log = std.log.scoped(.session_redraw);

/// App 邮箱句柄（跨线程安全）
app_mailbox: *App.Mailbox,

/// 事件循环
loop: xev.Loop,

/// 唤醒 handle
wakeup: xev.Async,
wakeup_c: xev.Completion = .{},

/// 合并定时器
timer: xev.Timer,
timer_c: xev.Completion = .{},

/// 合并间隔（纳秒）
coalesce_ns: u64,

/// 待重绘的会话集合
pending: std.StringHashMap(void),

/// 线程控制
thread: ?std.Thread = null,
alloc: Allocator,

pub fn init(alloc: Allocator, app_mailbox: *App.Mailbox, coalesce_ns: u64) !*Dispatcher {
    const self = try alloc.create(Dispatcher);
    errdefer alloc.destroy(self);

    var loop = try xev.Loop.init(.{});
    errdefer loop.deinit();

    var wakeup_h = try xev.Async.init();
    errdefer wakeup_h.deinit();

    var timer_h = try xev.Timer.init();
    errdefer timer_h.deinit();

    self.* = .{
        .app_mailbox = app_mailbox,
        .loop = loop,
        .wakeup = wakeup_h,
        .timer = timer_h,
        .coalesce_ns = coalesce_ns,
        .pending = std.StringHashMap(void).init(alloc),
        .thread = null,
        .alloc = alloc,
    };

    return self;
}

pub fn deinit(self: *Dispatcher) void {
    self.timer.deinit();
    self.wakeup.deinit();
    self.loop.deinit();
    self.pending.deinit();
    self.alloc.destroy(self);
}

pub fn start(self: *Dispatcher) !void {
    // 注册回调
    self.wakeup.wait(&self.loop, &self.wakeup_c, Dispatcher, self, wakeupCallback);
    // 初始时，定时器不 armed，按需在唤醒中启动
    self.thread = try std.Thread.spawn(.{}, threadMain, .{ self });
}

pub fn stop(self: *Dispatcher) void {
    self.loop.stop();
    if (self.thread) |t| t.join();
    self.thread = null;
}

pub fn post(self: *Dispatcher, session_id: []const u8) !void {
    // 复制 key 存入 pending（集合）
    if (!self.pending.contains(session_id)) {
        // 存入一份拷贝，稍后 App 处理并释放
        try self.pending.put(try self.alloc.dupe(u8, session_id), {});
    }
    // 唤醒以安排/重臂定时器
    self.wakeup.notify() catch {};
}

fn threadMain(self: *Dispatcher) void {
    self.loop.run(.until_done) catch |err| {
        log.err("dispatcher loop error: {}", .{err});
    };
}

fn wakeupCallback(
    self_opt: ?*Dispatcher,
    loop: *xev.Loop,
    _: *xev.Completion,
    r: xev.Async.WaitError!void,
) xev.CallbackAction {
    _ = r catch |err| {
        log.err("dispatcher wakeup err={}", .{err});
        return .rearm;
    };
    const self = self_opt.?;
    // 安排/重臂一次性定时器
    var c: xev.Completion = .{};
    self.timer.run(loop, &c, self.coalesce_ns, Dispatcher, self, timerCallback);
    return .rearm;
}

fn timerCallback(
    self_opt: ?*Dispatcher,
    _: *xev.Loop,
    _: *xev.Completion,
    r: xev.Timer.RunError!void,
) xev.CallbackAction {
    _ = r catch |err| {
        log.err("dispatcher timer err={}", .{err});
        return .disarm;
    };

    const self = self_opt.?;

    // 取出所有待处理的 session，向 App 投递 redraw_session 消息
    var it = self.pending.iterator();
    var to_remove: usize = 0;
    while (it.next()) |entry| {
        const sid = entry.key_ptr.*; // owned by pending map
        _ = self.app_mailbox.push(.{ .redraw_session = .{ .session = sid, .own = true } }, .{ .instant = {} });
        to_remove += 1;
    }
    // 移除已投递的键（避免二次释放）
    if (to_remove > 0) {
        self.pending.clearRetainingCapacity();
    }

    return .disarm;
}

// App 不再直接调用 free；由 own 标志决定释放职责


