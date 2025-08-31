//! SessionCore - 持久会话核心（骨架）
//! 目标：由其真正拥有 Terminal/PTY/进程；当前仅实现最小可编译骨架与自有 Terminal。

pub const SessionCore = @This();

const std = @import("std");
const Allocator = std.mem.Allocator;
// Import terminal public API via directory main
const terminalpkg = @import("main.zig");
const termio = @import("../termio.zig");
const renderer = @import("../renderer.zig");
const xev = @import("../global.zig").xev;
const configpkg = @import("../config.zig");
const Command = @import("../Command.zig");
const internal_os = @import("../os/main.zig");
const global_state = &@import("../global.zig").state;

/// 会话标识
id: []const u8,

/// 自有 Terminal（后续扩展到 PTY/进程）
owned_terminal: terminalpkg.Terminal,

/// 附加的查看器（Surface 指针，避免循环依赖，此处用 anyopaque）
attached_surfaces: std.ArrayListUnmanaged(*anyopaque) = .{},

/// 预留：PTY 与子进程（占位，不启用）
has_pty: bool = false,
io: ?*termio.Termio = null,
io_thread: ?*termio.Thread = null,
/// 可选：直接引用现有 Surface 的终端用于查看（过渡期方案）
view_terminal_ptr: ?*terminalpkg.Terminal = null,

/// Core 自有的渲染占位（无 viewer 时保持有效）
renderer_mutex_core: ?*std.Thread.Mutex = null,
renderer_state_core: ?renderer.State = null,
renderer_mailbox_core: ?*renderer.Thread.Mailbox = null,
renderer_wakeup_core: ?xev.Async = null,

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
        .io = null,
        .io_thread = null,
        .view_terminal_ptr = null,
    };
    return core;
}

pub fn deinit(self: *SessionCore, alloc: Allocator) void {
    self.owned_terminal.deinit(alloc);
    if (self.attached_surfaces.capacity > 0) self.attached_surfaces.deinit(alloc);
    if (self.renderer_mailbox_core) |mb| mb.destroy(alloc);
    if (self.renderer_wakeup_core) |*w| w.deinit();
    if (self.renderer_mutex_core) |m| alloc.destroy(m);
    if (self.io_thread) |t| t.deinit();
    if (self.io) |i| alloc.destroy(i);
    alloc.free(self.id);
    alloc.destroy(self);
}

/// 返回用于查看的 Terminal 指针
pub fn getTerminalForViewing(self: *SessionCore) *terminalpkg.Terminal {
    if (self.view_terminal_ptr) |p| return p;
    return &self.owned_terminal;
}

/// 过渡：使用已有 Surface 的终端作为查看源，并声明 has_pty=true（因其已有 IO 驱动）
pub fn adoptExistingTerminal(self: *SessionCore, term_ptr: *terminalpkg.Terminal) void {
    self.view_terminal_ptr = term_ptr;
    self.has_pty = true;
}

/// 可选：在 SessionCore 内部启动 PTY+shell，并将输出连接到 owned_terminal。
/// 仅在启用开关时由上层调用。默认不启用。
pub fn spawnPtyShell(
    self: *SessionCore,
    alloc: Allocator,
    surface_mailbox: @import("../apprt.zig").surface.Mailbox,
    size: renderer.Size,
    full_config: *const @import("../config.zig").Config,
) !void {
    // Only early-return if Core already owns a PTY (io != null). If has_pty
    // is true due to adoptExistingTerminal, we still want to spawn our own PTY
    // to take over ownership for persistence.
    if (self.has_pty and self.io != null) return; // already core-owned

    // Prepare env map (fallback to process env if apprt env unavailable)
    var env = internal_os.getEnvMap(alloc) catch std.process.EnvMap.init(alloc);
    errdefer env.deinit();

    // Build Exec backend similar to Surface.init
    const command: ?configpkg.Command =
        @field(full_config.*, "initial-command") orelse full_config.*.command;

    var io_exec = try termio.Exec.init(alloc, .{
        .command = command,
        .env = env,
        .env_override = full_config.*.env,
        .shell_integration = full_config.*.@"shell-integration",
        .shell_integration_features = full_config.*.@"shell-integration-features",
        .working_directory = full_config.*.@"working-directory",
        .resources_dir = global_state.resources_dir.host(),
        .term = full_config.*.term,
        .linux_cgroup = Command.linux_cgroup_default,
    });
    errdefer io_exec.deinit();

    var io_mailbox = try termio.Mailbox.initSPSC(alloc);
    errdefer io_mailbox.deinit(alloc);

    // Initialize Termio owned by SessionCore
    // Ensure core renderer placeholders
    if (self.renderer_mutex_core == null) {
        const m = try alloc.create(std.Thread.Mutex);
        m.* = .{};
        self.renderer_mutex_core = m;
    }
    if (self.renderer_mailbox_core == null) {
        self.renderer_mailbox_core = try renderer.Thread.Mailbox.create(alloc);
    }
    if (self.renderer_wakeup_core == null) {
        const w = try xev.Async.init();
        self.renderer_wakeup_core = w;
    }
    if (self.renderer_state_core == null) {
        self.renderer_state_core = .{ .mutex = self.renderer_mutex_core.?, .terminal = &self.owned_terminal };
    }

    // Allocate Termio on heap to keep stable address
    const io_inst = try alloc.create(termio.Termio);
    try termio.Termio.init(io_inst, alloc, .{
        .size = size,
        .full_config = full_config,
        .config = try termio.Termio.DerivedConfig.init(alloc, full_config),
        .backend = .{ .exec = io_exec },
        .mailbox = io_mailbox,
        .renderer_state = &self.renderer_state_core.?,
        .renderer_wakeup = self.renderer_wakeup_core.?,
        .renderer_mailbox = self.renderer_mailbox_core.?,
        .surface_mailbox = surface_mailbox,
    });

    // 让查看器指向 Termio 内部终端（由 IO 驱动）
    self.renderer_state_core.?.terminal = &io_inst.terminal;
    self.view_terminal_ptr = &io_inst.terminal;

    // 启动 IO 线程
    const io_thr = try alloc.create(termio.Thread);
    io_thr.* = try termio.Thread.init(alloc);
    _ = try std.Thread.spawn(.{}, termio.Thread.threadMain, .{ io_thr, io_inst });

    self.io = io_inst;
    self.io_thread = io_thr;
    self.has_pty = true;
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


