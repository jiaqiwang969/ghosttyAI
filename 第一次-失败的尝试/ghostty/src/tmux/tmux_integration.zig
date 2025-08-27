// tmux_integration.zig - Main tmux integration module for Ghostty
// Purpose: Provides TmuxHandle to integrate libtmuxcore with Ghostty Terminal
// Updated: Week 6 - Using real libtmuxcore API

const std = @import("std");
const terminalpkg = @import("../terminal/main.zig");

const log = std.log.scoped(.tmux);

// libtmuxcore C API
extern fn tmc_init() c_int;
extern fn tmc_cleanup() void;
extern fn tmc_session_new(name: [*c]const u8, session: *?*anyopaque) c_int;
extern fn tmc_session_attach(session: ?*anyopaque) c_int;
extern fn tmc_session_detach(session: ?*anyopaque) c_int;
extern fn tmc_window_new(session: ?*anyopaque, name: [*c]const u8, window: *?*anyopaque) c_int;
extern fn tmc_window_close(window: ?*anyopaque) c_int;
extern fn tmc_pane_split(window: ?*anyopaque, horizontal: c_int, size_percent: u32, new_pane: *?*anyopaque) c_int;
extern fn tmc_command_execute(command: [*c]const u8) c_int;
extern fn tmc_command_send_keys(pane: ?*anyopaque, keys: [*c]const u8) c_int;
extern fn tmc_input_process_key(key: u8) c_int;  // 关键函数！处理键盘输入

// UI Backend API
extern fn ui_backend_register(vtable: *anyopaque) void;
extern fn ui_backend_unregister() void;
extern fn ui_backend_is_active() c_int;

// UI callback registration
extern fn tmc_register_ui_callbacks(
    user_data: ?*anyopaque,
    on_update: ?*const fn(?*anyopaque) callconv(.C) void
) void;

const TMC_SUCCESS = 0;
const TMC_ERROR_INVALID_PARAM = -1;
const TMC_ERROR_NOT_INITIALIZED = -5;

// UI更新回调函数 - 当tmux屏幕内容改变时被调用
fn tmuxUIUpdateCallback(user_data: ?*anyopaque) callconv(.C) void {
    if (user_data) |data| {
        const handle = @as(*TmuxHandle, @ptrCast(@alignCast(data)));
        
        // TODO: 这里应该触发Ghostty重绘
        // 目前只记录日志
        log.info("tmux UI update triggered, need to refresh Ghostty display", .{});
        
        // 获取tmux的grid数据并更新到Ghostty的Terminal
        // handle.terminal应该被更新为显示tmux的内容
        _ = handle;
    }
}

pub const TmuxHandle = struct {
    allocator: std.mem.Allocator,
    terminal: *terminalpkg.Terminal,
    current_session: ?*anyopaque,
    current_window: ?*anyopaque,
    current_pane: ?*anyopaque,
    is_active: bool,
    
    pub fn init(allocator: std.mem.Allocator, terminal: *terminalpkg.Terminal) !TmuxHandle {
        log.info("Initializing libtmuxcore integration", .{});
        
        // Initialize tmux core library
        const result = tmc_init();
        if (result != TMC_SUCCESS) {
            return error.TmuxInitFailed;
        }
        
        var handle = TmuxHandle{
            .allocator = allocator,
            .terminal = terminal,
            .current_session = null,
            .current_window = null,
            .current_pane = null,
            .is_active = true,
        };
        
        // Create default session
        const session_name = "ghostty-main";
        var session: ?*anyopaque = null;
        const session_result = tmc_session_new(session_name, &session);
        if (session_result == TMC_SUCCESS) {
            handle.current_session = session;
            _ = tmc_session_attach(session);
            
            // Create default window
            const window_name = "shell";
            var window: ?*anyopaque = null;
            const window_result = tmc_window_new(session, window_name, &window);
            if (window_result == TMC_SUCCESS) {
                handle.current_window = window;
            }
        }
        
        log.info("libtmuxcore initialized successfully", .{});
        return handle;
    }
    
    pub fn deinit(self: *TmuxHandle) void {
        if (self.is_active) {
            ui_backend_unregister();
            tmc_cleanup();
            self.is_active = false;
            log.info("libtmuxcore cleaned up", .{});
        }
    }
    
    pub fn splitPane(self: *TmuxHandle, horizontal: bool) !void {
        if (self.current_window == null) {
            return error.NoActiveWindow;
        }
        
        var new_pane: ?*anyopaque = null;
        const result = tmc_pane_split(
            self.current_window,
            if (horizontal) 1 else 0,
            50,
            &new_pane
        );
        
        if (result != TMC_SUCCESS) {
            return error.PaneSplitFailed;
        }
        
        self.current_pane = new_pane;
        log.info("Split pane: {s}", .{if (horizontal) "horizontal" else "vertical"});
    }
    
    pub fn executeCommand(self: *TmuxHandle, command: []const u8) !void {
        const c_command = try self.allocator.dupeZ(u8, command);
        defer self.allocator.free(c_command);
        
        const result = tmc_command_execute(c_command);
        if (result != TMC_SUCCESS) {
            return error.CommandExecutionFailed;
        }
        
        log.info("Executed command: {s}", .{command});
    }
    
    pub fn sendKeys(self: *TmuxHandle, keys: []const u8) !void {
        if (self.current_pane == null) {
            return error.NoActivePane;
        }
        
        const c_keys = try self.allocator.dupeZ(u8, keys);
        defer self.allocator.free(c_keys);
        
        const result = tmc_command_send_keys(self.current_pane, c_keys);
        if (result != TMC_SUCCESS) {
            return error.SendKeysFailed;
        }
    }
    
    pub fn setupCallbacks(self: *TmuxHandle) !void {
        log.info("Setting up tmux UI callbacks", .{});
        
        // 注册UI更新回调，当tmux内容变化时会被调用
        tmc_register_ui_callbacks(
            @ptrCast(self),
            tmuxUIUpdateCallback
        );
        
        log.info("tmux UI callbacks registered", .{});
    }
    
    pub fn handleKeyInput(self: *TmuxHandle, key: u8, mods: u32) !bool {
        _ = self; // 不直接使用self，因为tmc_input_process_key是全局的
        _ = mods; // 目前不处理修饰键
        
        // 直接将所有键盘输入发送到libtmuxcore
        // libtmuxcore会自己处理Ctrl-B前缀和后续命令
        const result = tmc_input_process_key(key);
        
        if (result == TMC_SUCCESS) {
            log.info("tmux processed key: 0x{x:0>2}", .{key});
            return true; // tmux处理了这个键
        }
        
        // 如果tmux不处理，返回false让Ghostty处理
        return false;
    }
};