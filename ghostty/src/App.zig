//! App is the primary GUI application for ghostty. This builds the window,
//! sets up the renderer, etc. The primary run loop is started by calling
//! the "run" function.
const App = @This();

const std = @import("std");
const builtin = @import("builtin");
const assert = std.debug.assert;
const Allocator = std.mem.Allocator;
const build_config = @import("build_config.zig");
const apprt = @import("apprt.zig");
const Surface = @import("Surface.zig");
const tracy = @import("tracy");
const input = @import("input.zig");
const configpkg = @import("config.zig");
const Config = configpkg.Config;
const BlockingQueue = @import("datastruct/main.zig").BlockingQueue;
const renderer = @import("renderer.zig");
const font = @import("font/main.zig");
const internal_os = @import("os/main.zig");
const macos = @import("macos");
const objc = @import("objc");
const termio = @import("termio.zig");

const log = std.log.scoped(.app);
const process = std.process;

const SurfaceList = std.ArrayListUnmanaged(*apprt.Surface);
const SessionManager = @import("terminal/SessionManager.zig");
const SessionCore = @import("terminal/SessionCore.zig").SessionCore;
const SessionRedrawDispatcher = @import("terminal/SessionRedrawDispatcher.zig");

/// General purpose allocator
alloc: Allocator,

/// The list of surfaces that are currently active.
surfaces: SurfaceList,

/// This is true if the app that Ghostty is in is focused. This may
/// mean that no surfaces (terminals) are focused but the app is still
/// focused, i.e. may an about window. On macOS, this concept is known
/// as the "active" app while focused windows are known as the
/// "main" window.
///
/// This is used to determine if keyboard shortcuts that are non-global
/// should be processed. If the app is not focused, then we don't want
/// to process keyboard shortcuts that are not global.
///
/// This defaults to true since we assume that the app is focused when
/// Ghostty is initialized but a well behaved apprt should call
/// focusEvent to set this to the correct value right away.
focused: bool = true,

/// The last focused surface. This surface may not be valid;
/// you must always call hasSurface to validate it.
focused_surface: ?*Surface = null,

/// The mailbox that can be used to send this thread messages. Note
/// this is a blocking queue so if it is full you will get errors (or block).
mailbox: Mailbox.Queue,

/// The set of font GroupCache instances shared by surfaces with the
/// same font configuration.
font_grid_set: font.SharedGridSet,

// Used to rate limit desktop notifications. Some platforms (notably macOS) will
// run out of resources if desktop notifications are sent too fast and the OS
// will kill Ghostty.
last_notification_time: ?std.time.Instant = null,
last_notification_digest: u64 = 0,

/// The conditional state of the configuration. See the equivalent field
/// in the Surface struct for more information. In this case, this applies
/// to the app-level config and as a default for new surfaces.
config_conditional_state: configpkg.ConditionalState,

/// Set to false once we've created at least one surface. This
/// never goes true again. This can be used by surfaces to determine
/// if they are the first surface.
first: bool = true,

/// Session manager for terminal-to-terminal communication
session_manager: SessionManager,

/// 会话级重绘分发器（xev Loop + Async + Timer 合并）
redraw_dispatcher: ?*SessionRedrawDispatcher = null,
/// 提供给分发器使用的 App 邮箱（持久化句柄）
dispatcher_mailbox: ?Mailbox = null,

pub const CreateError = Allocator.Error || font.SharedGridSet.InitError;

/// Create a new app instance. This returns a stable pointer to the app
/// instance which is required for callbacks.
pub fn create(alloc: Allocator) CreateError!*App {
    var app = try alloc.create(App);
    errdefer alloc.destroy(app);
    try app.init(alloc);
    return app;
}

/// Initialize the main app instance. This creates the main window, sets
/// up the renderer state, compiles the shaders, etc. This is the primary
/// "startup" logic.
///
/// After calling this function, well behaved apprts should then call
/// `focusEvent` to set the initial focus state of the app.
pub fn init(
    self: *App,
    alloc: Allocator,
) CreateError!void {
    var font_grid_set = try font.SharedGridSet.init(alloc);
    errdefer font_grid_set.deinit();

    self.* = .{
        .alloc = alloc,
        .surfaces = .{},
        .mailbox = .{},
        .font_grid_set = font_grid_set,
        .config_conditional_state = .{},
        .session_manager = SessionManager.init(alloc),
        .redraw_dispatcher = null,
        .dispatcher_mailbox = null,
    };
}

pub fn deinit(self: *App) void {
    if (self.redraw_dispatcher) |d| {
        d.stop();
        d.deinit();
        self.redraw_dispatcher = null;
    }
    // Clean up all our surfaces
    for (self.surfaces.items) |surface| surface.deinit();
    self.surfaces.deinit(self.alloc);

    // Clean up session manager
    self.session_manager.deinit();

    // Clean up our font group cache
    // We should have zero items in the grid set at this point because
    // destroy only gets called when the app is shutting down and this
    // should gracefully close all surfaces.
    assert(self.font_grid_set.count() == 0);
    self.font_grid_set.deinit();
}

pub fn destroy(self: *App) void {
    // Deinitialize the app
    self.deinit();

    // Free the app memory
    self.alloc.destroy(self);
}

/// Tick ticks the app loop. This will drain our mailbox and process those
/// events. This should be called by the application runtime on every loop
/// tick.
pub fn tick(self: *App, rt_app: *apprt.App) !void {
    // Drain our mailbox
    try self.drainMailbox(rt_app);

    // Lazy-init dispatcher once rt_app is available and we have a mailbox
    if (self.redraw_dispatcher == null) {
        var mb: Mailbox = .{ .rt_app = rt_app, .mailbox = &self.mailbox };
        self.dispatcher_mailbox = mb;
        const d = try SessionRedrawDispatcher.init(self.alloc, &self.dispatcher_mailbox.?, 8 * std.time.ns_per_ms);
        try d.start();
        self.redraw_dispatcher = d;
    }
}

/// Update the configuration associated with the app. This can only be
/// called from the main thread. The caller owns the config memory. The
/// memory can be freed immediately when this returns.
pub fn updateConfig(self: *App, rt_app: *apprt.App, config: *const Config) !void {
    // Go through and update all of the surface configurations.
    for (self.surfaces.items) |surface| {
        try surface.core().handleMessage(.{ .change_config = config });
    }

    // Apply our conditional state. If we fail to apply the conditional state
    // then we log and attempt to move forward with the old config.
    // We only apply this to the app-level config because the surface
    // config applies its own conditional state.
    var applied_: ?configpkg.Config = config.changeConditionalState(
        self.config_conditional_state,
    ) catch |err| err: {
        log.warn("failed to apply conditional state to config err={}", .{err});
        break :err null;
    };
    defer if (applied_) |*c| c.deinit();
    const applied: *const configpkg.Config = if (applied_) |*c| c else config;

    // Notify the apprt that the app has changed configuration.
    _ = try rt_app.performAction(
        .app,
        .config_change,
        .{ .config = applied },
    );
}

/// Add an initialized surface. This is really only for the runtime
/// implementations to call and should NOT be called by general app users.
/// The surface must be from the pool.
pub fn addSurface(
    self: *App,
    rt_surface: *apprt.Surface,
) Allocator.Error!void {
    try self.surfaces.append(self.alloc, rt_surface);

    // Register surface with session manager - let it auto-generate a name initially
    const session_name = self.session_manager.registerSession(null, rt_surface.core(), false) catch |err| blk: {
        log.warn("Failed to register surface with SessionManager: {any}", .{err});
        // Fallback to a simple ID
        var buf: [32]u8 = undefined;
        const fallback = std.fmt.bufPrint(&buf, "s{x}", .{@intFromPtr(rt_surface.core()) & 0xFFFF}) catch "unknown";
        break :blk fallback;
    };
    
    // Store the session name in the surface for reference
    @memset(&rt_surface.core().session_id, 0);
    if (session_name.len <= 32) {
        @memcpy(rt_surface.core().session_id[0..session_name.len], session_name);
    }

    // Since we have non-zero surfaces, we can cancel the quit timer.
    // It is up to the apprt if there is a quit timer at all and if it
    // should be canceled.
    _ = rt_surface.rtApp().performAction(
        .app,
        .quit_timer,
        .stop,
    ) catch |err| {
        log.warn("error stopping quit timer err={}", .{err});
    };
}

/// Delete the surface from the known surface list. This will NOT call the
/// destructor or free the memory.
pub fn deleteSurface(self: *App, rt_surface: *apprt.Surface) void {
    // Unregister from session manager using stored name or pointer
    if (self.session_manager.getSessionByPointer(rt_surface.core())) |session_name| {
        self.session_manager.unregisterSession(session_name);
    }

    // If this surface is the focused surface then we need to clear it.
    // There was a bug where we relied on hasSurface to return false and
    // just let focused surface be but the allocator was reusing addresses
    // after free and giving false positives, so we must clear it.
    if (self.focused_surface) |focused| {
        if (focused == rt_surface.core()) {
            self.focused_surface = null;
        }
    }

    var i: usize = 0;
    while (i < self.surfaces.items.len) {
        if (self.surfaces.items[i] == rt_surface) {
            _ = self.surfaces.swapRemove(i);
            continue;
        }

        i += 1;
    }

    // If we have no surfaces, we can start the quit timer. It is up to the
    // apprt to determine if this is necessary.
    if (self.surfaces.items.len == 0) _ = rt_surface.rtApp().performAction(
        .app,
        .quit_timer,
        .start,
    ) catch |err| {
        log.warn("error starting quit timer err={}", .{err});
    };
}

/// The last focused surface. This is only valid while on the main thread
/// before tick is called.
pub fn focusedSurface(self: *const App) ?*Surface {
    const surface = self.focused_surface orelse return null;
    if (!self.hasSurface(surface)) return null;
    return surface;
}

/// Returns true if confirmation is needed to quit the app. It is up to
/// the apprt to call this.
pub fn needsConfirmQuit(self: *const App) bool {
    for (self.surfaces.items) |v| {
        if (v.core().needsConfirmQuit()) return true;
    }

    return false;
}

/// Drain the mailbox.
fn drainMailbox(self: *App, rt_app: *apprt.App) !void {
    while (self.mailbox.pop()) |message| {
        log.debug("mailbox message={s}", .{@tagName(message)});
        switch (message) {
            .open_config => try self.performAction(rt_app, .open_config),
            .new_window => |msg| try self.newWindow(rt_app, msg),
            .close => |surface| self.closeSurface(surface),
            .surface_message => |msg| try self.surfaceMessage(msg.surface, msg.message),
            .redraw_session => |rs| {
                // rs.session 是 dispatcher 分配的字符串，使用后释放
                self.broadcastRedrawForSession(rt_app, rs.session) catch |err| {
                    log.warn("broadcastRedrawForSession failed: {}", .{err});
                };
                if (self.redraw_dispatcher) |d| d.freeSessionId(rs.session);
            },
            .redraw_surface => |surface| try self.redrawSurface(rt_app, surface),
            .redraw_inspector => |surface| self.redrawInspector(rt_app, surface),
            .send_to_session => |msg| {
                self.sendToSession(msg.from_surface, msg.target, msg.message) catch |err| {
                    log.warn("Failed to send to session: {}", .{err});
                };
            },
            .register_session => |msg| {
                self.registerSessionWithName(msg.surface, msg.name) catch |err| {
                    log.warn("Failed to register session: {}", .{err});
                };
            },
            .get_session => |msg| {
                if (self.getSessionName(msg.surface)) |name| {
                    // Send the name back to the surface for display
                    msg.surface.displaySessionName(name);
                }
            },
            .attach_render => |msg| {
                self.attachRender(msg.from_surface, msg.target) catch |err| {
                    log.warn("Failed to attach-render: {}", .{err});
                };
            },
            .attach_render_reset => |msg| {
                self.attachRenderReset(msg.from_surface) catch |err| {
                    log.warn("Failed to detach-render: {}", .{err});
                };
            },
            .attach_io => |msg| {
                self.attachIO(msg.from_surface, msg.target) catch |err| {
                    log.warn("Failed to attach-io: {}", .{err});
                };
            },
            .detach_io => |msg| {
                self.detachIO(msg.from_surface) catch |err| {
                    log.warn("Failed to detach-io: {}", .{err});
                };
            },
            .core_pty_start => |msg| {
                self.corePtyStart(msg.from_surface, msg.target) catch |err| {
                    log.warn("Failed to core-pty-start: {}", .{err});
                };
            },

            // If we're quitting, then we set the quit flag and stop
            // draining the mailbox immediately. This lets us defer
            // mailbox processing to the next tick so that the apprt
            // can try to quit as quickly as possible.
            .quit => {
                log.info("quit message received, short circuiting mailbox drain", .{});
                try self.performAction(rt_app, .quit);
                return;
            },
        }
    }
}

pub fn closeSurface(self: *App, surface: *Surface) void {
    if (!self.hasSurface(surface)) return;
    surface.close();
}

pub fn focusSurface(self: *App, surface: *Surface) void {
    if (!self.hasSurface(surface)) return;
    self.focused_surface = surface;
}

fn redrawSurface(
    self: *App,
    rt_app: *apprt.App,
    surface: *apprt.Surface,
) !void {
    if (!self.hasRtSurface(surface)) return;

    _ = try rt_app.performAction(
        .{ .surface = surface.core() },
        .render,
        {},
    );
}

fn redrawInspector(self: *App, rt_app: *apprt.App, surface: *apprt.Surface) void {
    if (!self.hasRtSurface(surface)) return;
    rt_app.redrawInspector(surface);
}

/// Send a command to another terminal session
pub fn sendToSession(self: *App, from: *Surface, target_name: []const u8, message: []const u8) !void {
    // Get sender's session name
    const from_name = self.session_manager.getSessionByPointer(from) orelse "unknown";
    
    // Send via SessionManager which will handle routing
    try self.session_manager.sendToSession(from_name, target_name, message, false);
}

/// Register a surface with a custom session name
pub fn registerSessionWithName(self: *App, surface: *Surface, name: []const u8) !void {
    // First unregister if already registered
    if (self.session_manager.getSessionByPointer(surface)) |old_name| {
        self.session_manager.unregisterSession(old_name);
    }
    
    // Register with new name
    const actual_name = try self.session_manager.registerSession(name, surface, false);
    
    // Update surface's session_id field
    @memset(&surface.session_id, 0);
    if (actual_name.len <= 32) {
        @memcpy(surface.session_id[0..actual_name.len], actual_name);
    }
}

/// Get the current session name for a surface
pub fn getSessionName(self: *App, surface: *Surface) ?[]const u8 {
    return self.session_manager.getSessionByPointer(surface);
}

/// 在当前 surface 上将渲染目标切换为目标会话对应 Surface 的 terminal 指针
pub fn attachRender(self: *App, from: *Surface, target_name: []const u8) !void {
    // 默认使用 SessionCore 渲染；若不可用则回退到 Surface 终端
    var used_core_path = false;
    core_path: {
        const core = self.session_manager.getOrCreateCore(target_name) catch |err| {
            log.warn("attach-render core path error: {} (fallback)", .{err});
            break :core_path;
        };
        if (!core.has_pty) break :core_path;
        from.renderer_state.mutex.lock();
        from.renderer_state.terminal = core.getTerminalForViewing();
        from.renderer_state.terminal.flags.dirty.clear = true;
        from.renderer_state.mutex.unlock();
        used_core_path = true;
        log.info("attach-render via SessionCore for {s}", .{target_name});

        // 同步输入目标，实现统一的 attach（渲染+输入）
        if (from.io_attach_target) |old| from.alloc.free(old);
        from.io_attach_target = try from.alloc.dupe(u8, target_name);
    }

    if (!used_core_path) {
        const ptr_any = self.session_manager.getSurfaceById(target_name) orelse return error.SessionNotFound;
        const target_surface = @as(*Surface, @ptrCast(@alignCast(ptr_any)));
        from.renderer_state.mutex.lock();
        from.renderer_state.terminal = &target_surface.io.terminal;
        from.renderer_state.terminal.flags.dirty.clear = true;
        from.renderer_state.mutex.unlock();
        log.info("attach-render via Surface terminal for {s}", .{target_name});
    }

    // 登记查看器附着关系（仅元数据）
    self.session_manager.attachViewer(target_name, from) catch |err| {
        log.warn("attachViewer registry failed: {}", .{err});
    };

    // 请求重绘
    try from.queueRender();

    // 文本反馈
    from.renderer_state.mutex.lock();
    const t = from.renderer_state.terminal;
    t.carriageReturn();
    t.linefeed() catch {};
    t.printString("[ghostty] attach-render -> ") catch {};
    t.printString(target_name) catch {};
    t.linefeed() catch {};
    from.renderer_state.mutex.unlock();
}

/// 恢复本地渲染：让 from.surface 重新指向自身 io.terminal
pub fn attachRenderReset(self: *App, from: *Surface) !void {

    from.renderer_state.mutex.lock();
    from.renderer_state.terminal = &from.io.terminal;
    from.renderer_state.terminal.flags.dirty.clear = true;
    from.renderer_state.mutex.unlock();

    // 解除查看器附着登记
    self.session_manager.detachViewer(from);

    try from.queueRender();

    from.renderer_state.mutex.lock();
    const t = from.renderer_state.terminal;
    t.carriageReturn();
    t.linefeed() catch {};
    t.printString("[ghostty] detach-render -> local") catch {};
    t.linefeed() catch {};
    from.renderer_state.mutex.unlock();
}

/// 读取布尔环境变量，true/1/yes/on 视为开启
fn envFlagEnabled(name: []const u8) bool {
    var gpa = std.heap.page_allocator;
    const val = process.getEnvVarOwned(gpa, name) catch return false;
    defer gpa.free(val);
    return std.mem.eql(u8, val, "1")
        or std.ascii.eqlIgnoreCase(val, "true")
        or std.ascii.eqlIgnoreCase(val, "yes")
        or std.ascii.eqlIgnoreCase(val, "on");
}

/// 绑定输入：将 from 的键盘输入转发到 target 会话对应的 Surface 的 PTY
pub fn attachIO(self: *App, from: *Surface, target_name: []const u8) !void {
    // legacy lookup retained intentionally to keep side effects of registration
    _ = self.session_manager.getSurfaceById(target_name);

    // 保存目标名到 from.io_attach_target（先释放旧值）
    if (from.io_attach_target) |old| from.alloc.free(old);
    from.io_attach_target = try from.alloc.dupe(u8, target_name);

    // 反馈
    from.renderer_state.mutex.lock();
    const t = from.renderer_state.terminal;
    t.carriageReturn();
    t.linefeed() catch {};
    t.printString("[ghostty] attach-io -> ") catch {};
    t.printString(target_name) catch {};
    t.linefeed() catch {};
    from.renderer_state.mutex.unlock();

    // 轻量重绘
    try from.queueRender();

    // no-op
}

/// 解除输入转发
pub fn detachIO(self: *App, from: *Surface) !void {
    _ = self;
    if (from.io_attach_target) |old| {
        from.alloc.free(old);
        from.io_attach_target = null;
    }

    from.renderer_state.mutex.lock();
    const t = from.renderer_state.terminal;
    t.carriageReturn();
    t.linefeed() catch {};
    t.printString("[ghostty] detach-io -> local") catch {};
    t.linefeed() catch {};
    from.renderer_state.mutex.unlock();
    try from.queueRender();
}

/// 启动 SessionCore 的 PTY 并将渲染绑定到 core 终端
pub fn corePtyStart(self: *App, from: *Surface, target_opt: ?[]const u8) !void {
    const target_name = target_opt orelse (self.getSessionName(from) orelse return error.SessionNotFound);
    const core = try self.session_manager.getOrCreateCore(target_name);
    // 尝试启动（如果已存在则直接返回）
    var cfg = try @import("config.zig").Config.default(self.alloc);
    defer cfg.deinit();

    // Force SessionCore cursor to bar (vertical beam) for this startup path
    cfg.@"cursor-style" = @import("terminal/main.zig").CursorStyle.bar;

    core.spawnPtyShell(
        self.alloc,
        .{ .surface = from, .app = .{ .rt_app = from.rt_app, .mailbox = &self.mailbox } },
        from.size,
        &cfg,
    ) catch |err| {
        log.warn("corePtyStart spawn failed: {}", .{err});
        return err;
    };

    from.renderer_state.mutex.lock();
    from.renderer_state.terminal = core.getTerminalForViewing();
    from.renderer_state.terminal.flags.dirty.clear = true;
    from.renderer_state.mutex.unlock();
    // 同步输入目标为该会话，实现“渲染+输入”统一
    if (from.io_attach_target) |old| from.alloc.free(old);
    from.io_attach_target = try from.alloc.dupe(u8, target_name);
    try from.queueRender();
}

/// Create a new window
pub fn newWindow(self: *App, rt_app: *apprt.App, msg: Message.NewWindow) !void {
    const target: apprt.Target = target: {
        const parent = msg.parent orelse break :target .app;
        if (self.hasSurface(parent)) break :target .{ .surface = parent };
        break :target .app;
    };

    _ = try rt_app.performAction(
        target,
        .new_window,
        {},
    );
}

/// Handle an app-level focus event. This should be called whenever
/// the focus state of the entire app containing Ghostty changes.
/// This is separate from surface focus events. See the `focused`
/// field for more information.
pub fn focusEvent(self: *App, focused: bool) void {
    // Prevent redundant focus events
    if (self.focused == focused) return;

    log.debug("focus event focused={}", .{focused});
    self.focused = focused;
}

/// Returns true if the given key event would trigger a keybinding
/// if it were to be processed. This is useful for determining if
/// a key event should be sent to the terminal or not.
pub fn keyEventIsBinding(
    self: *App,
    rt_app: *apprt.App,
    event: input.KeyEvent,
) bool {
    _ = self;

    switch (event.action) {
        .release => return false,
        .press, .repeat => {},
    }

    // If we have a keybinding for this event then we return true.
    return rt_app.config.keybind.set.getEvent(event) != null;
}

/// Handle a key event at the app-scope. If this key event is used,
/// this will return true and the caller shouldn't continue processing
/// the event. If the event is not used, this will return false.
///
/// If the app currently has focus then all key events are processed.
/// If the app does not have focus then only global key events are
/// processed.
pub fn keyEvent(
    self: *App,
    rt_app: *apprt.App,
    event: input.KeyEvent,
) bool {
    switch (event.action) {
        // We don't care about key release events.
        .release => return false,

        // Continue processing key press events.
        .press, .repeat => {},
    }

    // Get the keybind entry for this event. We don't support key sequences
    // so we can look directly in the top-level set.
    const entry = rt_app.config.keybind.set.getEvent(event) orelse return false;
    const leaf: input.Binding.Set.Leaf = switch (entry.value_ptr.*) {
        // Sequences aren't supported. Our configuration parser verifies
        // this for global keybinds but we may still get an entry for
        // a non-global keybind.
        .leader => return false,

        // Leaf entries are good
        .leaf => |leaf| leaf,
    };

    // If we aren't focused, then we only process global keybinds.
    if (!self.focused and !leaf.flags.global) return false;

    // Global keybinds are done using performAll so that they
    // can target all surfaces too.
    if (leaf.flags.global) {
        self.performAllAction(rt_app, leaf.action) catch |err| {
            log.warn("error performing global keybind action action={s} err={}", .{
                @tagName(leaf.action),
                err,
            });
        };

        return true;
    }

    // Must be focused to process non-global keybinds
    assert(self.focused);
    assert(!leaf.flags.global);

    // If we are focused, then we process keybinds only if they are
    // app-scoped. Otherwise, we do nothing. Surface-scoped should
    // be processed by Surface.keyEvent.
    const app_action = leaf.action.scoped(.app) orelse return false;
    self.performAction(rt_app, app_action) catch |err| {
        log.warn("error performing app keybind action action={s} err={}", .{
            @tagName(app_action),
            err,
        });
    };

    return true;
}

/// Call to notify Ghostty that the color scheme for the app has changed.
/// "Color scheme" in this case refers to system themes such as "light/dark".
pub fn colorSchemeEvent(
    self: *App,
    rt_app: *apprt.App,
    scheme: apprt.ColorScheme,
) !void {
    const new_scheme: configpkg.ConditionalState.Theme = switch (scheme) {
        .light => .light,
        .dark => .dark,
    };

    // If our scheme didn't change, then we don't do anything.
    if (self.config_conditional_state.theme == new_scheme) return;

    // Setup our conditional state which has the current color theme.
    self.config_conditional_state.theme = new_scheme;

    // Request our configuration be reloaded because the new scheme may
    // impact the colors of the app.
    _ = try rt_app.performAction(
        .app,
        .reload_config,
        .{ .soft = true },
    );
}

/// Perform a binding action. This only accepts actions that are scoped
/// to the app. Callers can use performAllAction to perform any action
/// and any non-app-scoped actions will be performed on all surfaces.
pub fn performAction(
    self: *App,
    rt_app: *apprt.App,
    action: input.Binding.Action.Scoped(.app),
) !void {
    switch (action) {
        .unbind => unreachable,
        .ignore => {},
        .quit => _ = try rt_app.performAction(.app, .quit, {}),
        .new_window => _ = try self.newWindow(rt_app, .{ .parent = null }),
        .open_config => _ = try rt_app.performAction(.app, .open_config, {}),
        .reload_config => _ = try rt_app.performAction(.app, .reload_config, .{}),
        .close_all_windows => _ = try rt_app.performAction(.app, .close_all_windows, {}),
        .toggle_quick_terminal => _ = try rt_app.performAction(.app, .toggle_quick_terminal, {}),
        .toggle_visibility => _ = try rt_app.performAction(.app, .toggle_visibility, {}),
        .check_for_updates => _ = try rt_app.performAction(.app, .check_for_updates, {}),
        .show_gtk_inspector => _ = try rt_app.performAction(.app, .show_gtk_inspector, {}),
        .undo => _ = try rt_app.performAction(.app, .undo, {}),

        .redo => _ = try rt_app.performAction(.app, .redo, {}),
    }
}

/// Perform an app-wide binding action. If the action is surface-specific
/// then it will be performed on all surfaces. To perform only app-scoped
/// actions, use performAction.
pub fn performAllAction(
    self: *App,
    rt_app: *apprt.App,
    action: input.Binding.Action,
) !void {
    switch (action.scope()) {
        // App-scoped actions are handled by the app so that they aren't
        // repeated for each surface (since each surface forwards
        // app-scoped actions back up).
        .app => try self.performAction(
            rt_app,
            action.scoped(.app).?, // asserted through the scope match
        ),

        // Surface-scoped actions are performed on all surfaces. Errors
        // are logged but processing continues.
        .surface => for (self.surfaces.items) |surface| {
            _ = surface.core().performBindingAction(action) catch |err| {
                log.warn("error performing binding action on surface ptr={X} err={}", .{
                    @intFromPtr(surface),
                    err,
                });
            };
        },
    }
}

/// Broadcast redraw to all viewers attached to the same session as this surface.
fn broadcastRedrawForSurface(self: *App, rt_app: *apprt.App, surface: *Surface) !void {
    const session_name = self.session_manager.getSessionByPointer(surface) orelse return;
    const viewers = self.session_manager.copyViewers(session_name, self.alloc) catch return;
    defer self.alloc.free(viewers);

    // If no viewers registered, redraw this surface only as a fallback
    if (viewers.len == 0) {
        _ = try rt_app.performAction(.{ .surface = surface }, .render, {});
        return;
    }

    // Focus-first: redraw focused viewer first
    const fs_opt = self.focused_surface;
    if (fs_opt) |fs| {
        if (self.hasSurface(fs)) {
            _ = try rt_app.performAction(.{ .surface = fs }, .render, {});
        }
    }

    // Redraw remaining viewers (skip focused surface if present)
    for (viewers) |vp| {
        const v = @as(*Surface, @ptrCast(@alignCast(vp)));
        if (!self.hasSurface(v)) continue;
        if (fs_opt) |fs| if (v == fs) continue;
        _ = try rt_app.performAction(.{ .surface = v }, .render, {});
    }
}

fn broadcastRedrawForSession(self: *App, rt_app: *apprt.App, session_name: []const u8) !void {
    const viewers = self.session_manager.copyViewers(session_name, self.alloc) catch return;
    defer self.alloc.free(viewers);

    // Focus-first
    if (self.focused_surface) |fs| {
        if (self.hasSurface(fs)) {
            _ = try rt_app.performAction(.{ .surface = fs }, .render, {});
        }
    }
    // Others
    for (viewers) |vp| {
        const v = @as(*Surface, @ptrCast(@alignCast(vp)));
        if (!self.hasSurface(v)) continue;
        if (self.focused_surface) |fs| if (v == fs) continue;
        _ = try rt_app.performAction(.{ .surface = v }, .render, {});
    }
}

/// Handle a window message
fn surfaceMessage(self: *App, surface: *Surface, msg: apprt.surface.Message) !void {
    // We want to ensure our window is still active. Window messages
    // are quite rare and we normally don't have many windows so we do
    // a simple linear search here.
    if (self.hasSurface(surface)) {
        try surface.handleMessage(msg);
    }

    // Window was not found, it probably quit before we handled the message.
    // Not a problem.
}

fn hasSurface(self: *const App, surface: *const Surface) bool {
    for (self.surfaces.items) |v| {
        if (v.core() == surface) return true;
    }

    return false;
}

fn hasRtSurface(self: *const App, surface: *apprt.Surface) bool {
    for (self.surfaces.items) |v| {
        if (v == surface) return true;
    }

    return false;
}

/// The message types that can be sent to the app thread.
pub const Message = union(enum) {
    // Open the configuration file
    open_config: void,

    /// Create a new terminal window.
    new_window: NewWindow,

    /// Close a surface. This notifies the runtime that a surface
    /// should close.
    close: *Surface,

    /// Quit
    quit: void,

    /// A message for a specific surface.
    surface_message: struct {
        surface: *Surface,
        message: apprt.surface.Message,
    },

    /// Redraw a surface. This only has an effect for runtimes that
    /// use single-threaded draws. To redraw a surface for all runtimes,
    /// wake up the renderer thread. The renderer thread will send this
    /// message if it needs to.
    redraw_surface: *apprt.Surface,

    /// Redraw the inspector. This is called whenever some non-OS event
    /// causes the inspector to need to be redrawn.
    redraw_inspector: *apprt.Surface,

    /// Redraw all viewers of a session (dispatched by SessionRedrawDispatcher)
    redraw_session: struct { session: []const u8 },

    /// Send a command to another terminal session
    send_to_session: struct {
        from_surface: *Surface,
        target: []const u8,  // Target session name
        message: []const u8,
    },
    
    /// Register a session with a custom name
    register_session: struct {
        surface: *Surface,
        name: []const u8,
    },
    
    /// Get current session name for a surface
    get_session: struct {
        surface: *Surface,
    },

    /// Attach current surface's renderer to target session's terminal (render-only)
    attach_render: struct {
        from_surface: *Surface,
        target: []const u8,
    },

    /// Reset current surface to render its own local terminal
    attach_render_reset: struct {
        from_surface: *Surface,
    },

    /// Attach current surface's input to target session's PTY
    attach_io: struct {
        from_surface: *Surface,
        target: []const u8,
    },

    /// Detach current surface's input forwarding
    detach_io: struct {
        from_surface: *Surface,
    },

    /// Start SessionCore-owned PTY for a session, then rebind renderer to Core terminal
    core_pty_start: struct {
        from_surface: *Surface,
        target: ?[]const u8 = null,
    },

    const NewWindow = struct {
        /// The parent surface
        parent: ?*Surface = null,
    };
};

/// Mailbox is the way that other threads send the app thread messages.
pub const Mailbox = struct {
    /// The type used for sending messages to the app thread.
    pub const Queue = BlockingQueue(Message, 64);

    rt_app: *apprt.App,
    mailbox: *Queue,

    /// Send a message to the surface.
    pub fn push(self: Mailbox, msg: Message, timeout: Queue.Timeout) Queue.Size {
        const result = self.mailbox.push(msg, timeout);

        // Wake up our app loop
        self.rt_app.wakeup();

        return result;
    }
};

// Wasm API.
pub const Wasm = if (!builtin.target.isWasm()) struct {} else struct {
    const wasm = @import("os/wasm.zig");
    const alloc = wasm.alloc;

    // export fn app_new(config: *Config) ?*App {
    //     return app_new_(config) catch |err| { log.err("error initializing app err={}", .{err});
    //         return null;
    //     };
    // }
    //
    // fn app_new_(config: *Config) !*App {
    //     const app = try App.create(alloc, config);
    //     errdefer app.destroy();
    //
    //     const result = try alloc.create(App);
    //     result.* = app;
    //     return result;
    // }
    //
    // export fn app_free(ptr: ?*App) void {
    //     if (ptr) |v| {
    //         v.destroy();
    //         alloc.destroy(v);
    //     }
    // }
};
