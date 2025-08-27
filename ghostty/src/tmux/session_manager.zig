// session_manager.zig - tmux session management for Ghostty
// Purpose: Handle tmux session lifecycle (create, attach, detach, persist)
// Date: 2025-08-26
// Task: T-503 - Day 3 Session Management

const std = @import("std");
const Terminal = @import("../terminal/Terminal.zig");
const tmux_bridge = @import("tmux_terminal_bridge.zig");

const log = std.log.scoped(.tmux_session);

pub const SessionManager = struct {
    allocator: std.mem.Allocator,
    sessions: std.ArrayList(Session),
    active_session: ?*Session,
    persistence_path: []const u8,
    
    pub const Session = struct {
        id: u32,
        name: []const u8,
        terminal: *Terminal,
        created: i64,
        last_activity: i64,
        window_count: u32,
        pane_count: u32,
        is_attached: bool,
        
        pub fn init(allocator: std.mem.Allocator, name: []const u8, terminal: *Terminal) !Session {
            return Session{
                .id = @intCast(u32, std.time.timestamp()),
                .name = try allocator.dupe(u8, name),
                .terminal = terminal,
                .created = std.time.timestamp(),
                .last_activity = std.time.timestamp(),
                .window_count = 1,
                .pane_count = 1,
                .is_attached = true,
            };
        }
        
        pub fn deinit(self: *Session, allocator: std.mem.Allocator) void {
            allocator.free(self.name);
        }
        
        pub fn updateActivity(self: *Session) void {
            self.last_activity = std.time.timestamp();
        }
    };
    
    pub fn init(allocator: std.mem.Allocator) !SessionManager {
        const home = std.os.getenv("HOME") orelse "/tmp";
        const persistence_path = try std.fmt.allocPrint(
            allocator,
            "{s}/.ghostty/tmux_sessions.json",
            .{home},
        );
        
        return SessionManager{
            .allocator = allocator,
            .sessions = std.ArrayList(Session).init(allocator),
            .active_session = null,
            .persistence_path = persistence_path,
        };
    }
    
    pub fn deinit(self: *SessionManager) void {
        for (self.sessions.items) |*session| {
            session.deinit(self.allocator);
        }
        self.sessions.deinit();
        self.allocator.free(self.persistence_path);
    }
    
    pub fn createSession(self: *SessionManager, name: []const u8, terminal: *Terminal) !*Session {
        log.info("Creating tmux session: {s}", .{name});
        
        // Check if session already exists
        for (self.sessions.items) |*session| {
            if (std.mem.eql(u8, session.name, name)) {
                return error.SessionAlreadyExists;
            }
        }
        
        // Create new session
        var session = try Session.init(self.allocator, name, terminal);
        try self.sessions.append(session);
        
        // Make it active
        self.active_session = &self.sessions.items[self.sessions.items.len - 1];
        
        // Initialize tmux integration for this session
        try tmux_bridge.initTmuxIntegration(terminal, self.allocator);
        
        log.info("Session created: {s} (id: {})", .{ name, session.id });
        return self.active_session.?;
    }
    
    pub fn attachSession(self: *SessionManager, name: []const u8) !void {
        log.info("Attaching to session: {s}", .{name});
        
        for (self.sessions.items) |*session| {
            if (std.mem.eql(u8, session.name, name)) {
                // Detach current session if any
                if (self.active_session) |active| {
                    active.is_attached = false;
                }
                
                // Attach to new session
                session.is_attached = true;
                session.updateActivity();
                self.active_session = session;
                
                log.info("Attached to session: {s}", .{name});
                return;
            }
        }
        
        return error.SessionNotFound;
    }
    
    pub fn detachSession(self: *SessionManager) void {
        if (self.active_session) |session| {
            log.info("Detaching from session: {s}", .{session.name});
            session.is_attached = false;
            session.updateActivity();
            self.active_session = null;
        }
    }
    
    pub fn listSessions(self: *SessionManager) []const Session {
        return self.sessions.items;
    }
    
    pub fn deleteSession(self: *SessionManager, name: []const u8) !void {
        for (self.sessions.items, 0..) |*session, i| {
            if (std.mem.eql(u8, session.name, name)) {
                log.info("Deleting session: {s}", .{name});
                
                // Can't delete active session
                if (self.active_session == session) {
                    return error.SessionIsActive;
                }
                
                // Clean up and remove
                session.deinit(self.allocator);
                _ = self.sessions.swapRemove(i);
                return;
            }
        }
        
        return error.SessionNotFound;
    }
    
    pub fn renameSession(self: *SessionManager, old_name: []const u8, new_name: []const u8) !void {
        // Check new name doesn't exist
        for (self.sessions.items) |*session| {
            if (std.mem.eql(u8, session.name, new_name)) {
                return error.SessionNameExists;
            }
        }
        
        // Find and rename
        for (self.sessions.items) |*session| {
            if (std.mem.eql(u8, session.name, old_name)) {
                self.allocator.free(session.name);
                session.name = try self.allocator.dupe(u8, new_name);
                session.updateActivity();
                log.info("Renamed session: {s} -> {s}", .{ old_name, new_name });
                return;
            }
        }
        
        return error.SessionNotFound;
    }
    
    pub fn saveState(self: *SessionManager) !void {
        log.info("Saving session state to: {s}", .{self.persistence_path});
        
        // Create directory if needed
        const dir_path = std.fs.path.dirname(self.persistence_path) orelse return error.InvalidPath;
        std.fs.makeDirAbsolute(dir_path) catch |err| switch (err) {
            error.PathAlreadyExists => {},
            else => return err,
        };
        
        // Serialize sessions to JSON
        var file = try std.fs.createFileAbsolute(self.persistence_path, .{});
        defer file.close();
        
        var json_stream = std.json.writeStream(file.writer(), .{ .whitespace = .indent_2 });
        try json_stream.beginObject();
        
        try json_stream.objectField("version");
        try json_stream.write(@as(u32, 1));
        
        try json_stream.objectField("sessions");
        try json_stream.beginArray();
        
        for (self.sessions.items) |session| {
            try json_stream.beginObject();
            try json_stream.objectField("id");
            try json_stream.write(session.id);
            try json_stream.objectField("name");
            try json_stream.write(session.name);
            try json_stream.objectField("created");
            try json_stream.write(session.created);
            try json_stream.objectField("last_activity");
            try json_stream.write(session.last_activity);
            try json_stream.objectField("window_count");
            try json_stream.write(session.window_count);
            try json_stream.objectField("pane_count");
            try json_stream.write(session.pane_count);
            try json_stream.endObject();
        }
        
        try json_stream.endArray();
        try json_stream.endObject();
        
        log.info("Saved {} sessions", .{self.sessions.items.len});
    }
    
    pub fn loadState(self: *SessionManager) !void {
        log.info("Loading session state from: {s}", .{self.persistence_path});
        
        const file = std.fs.openFileAbsolute(self.persistence_path, .{}) catch |err| switch (err) {
            error.FileNotFound => {
                log.info("No saved state found", .{});
                return;
            },
            else => return err,
        };
        defer file.close();
        
        // For now, just log that we would load
        // Full JSON parsing would be implemented here
        const stat = try file.stat();
        log.info("Found saved state file ({} bytes)", .{stat.size});
    }
};

// Tests
test "SessionManager basic operations" {
    const testing = std.testing;
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();
    
    // Create mock terminal
    var terminal = try Terminal.init(allocator, .{
        .cols = 80,
        .rows = 24,
        .max_scrollback = 1000,
        .default_modes = .{},
    });
    defer terminal.deinit(allocator);
    
    // Create session manager
    var mgr = try SessionManager.init(allocator);
    defer mgr.deinit();
    
    // Create session
    const session = try mgr.createSession("test-session", &terminal);
    try testing.expectEqual(@as(usize, 1), mgr.sessions.items.len);
    try testing.expectEqualStrings("test-session", session.name);
    try testing.expect(session.is_attached);
    
    // List sessions
    const sessions = mgr.listSessions();
    try testing.expectEqual(@as(usize, 1), sessions.len);
    
    // Detach
    mgr.detachSession();
    try testing.expect(!session.is_attached);
    try testing.expect(mgr.active_session == null);
    
    // Reattach
    try mgr.attachSession("test-session");
    try testing.expect(session.is_attached);
    try testing.expect(mgr.active_session == session);
    
    // Rename
    try mgr.renameSession("test-session", "renamed-session");
    try testing.expectEqualStrings("renamed-session", session.name);
    
    // Delete (should fail while attached)
    try testing.expectError(error.SessionIsActive, mgr.deleteSession("renamed-session"));
    
    // Detach and delete
    mgr.detachSession();
    try mgr.deleteSession("renamed-session");
    try testing.expectEqual(@as(usize, 0), mgr.sessions.items.len);
}

test "SessionManager persistence" {
    const testing = std.testing;
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();
    
    var terminal = try Terminal.init(allocator, .{
        .cols = 80,
        .rows = 24,
        .max_scrollback = 1000,
        .default_modes = .{},
    });
    defer terminal.deinit(allocator);
    
    var mgr = try SessionManager.init(allocator);
    defer mgr.deinit();
    
    // Create multiple sessions
    _ = try mgr.createSession("session-1", &terminal);
    _ = try mgr.createSession("session-2", &terminal);
    _ = try mgr.createSession("session-3", &terminal);
    
    // Save state
    try mgr.saveState();
    
    // Would test loading in real implementation
    try mgr.loadState();
}