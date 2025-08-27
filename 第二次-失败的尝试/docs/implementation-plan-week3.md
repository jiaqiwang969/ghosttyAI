# Week 3 Implementation Plan: Real tmux Integration into Ghostty

## Executive Summary
Based on analysis of the tmux source code, we've identified the exact integration points and architecture needed to embed tmux's session management into Ghostty while preserving Ghostty's GPU-rendered UI.

## Architecture Overview

### Key Integration Points

1. **tmux side (C):**
   - `tty.c:1731` - tty_write() function: Central dispatch for all output
   - `control.c:406` - control_write(): Control mode notifications
   - `cmd-send-keys.c:153` - cmd_send_keys_exec(): Key injection logic
   - `screen-write.c` - All tty_write() calls for screen updates

2. **Ghostty side (Zig):**
   - `src/terminal/tmux.zig` - Already has control mode parser
   - `src/terminal/Terminal.zig` - Needs tmux session integration
   - NEW: `src/tmux/libtmuxcore.zig` - FFI bridge to tmux library

## Implementation Steps

### Step 1: Modify tmux to Build as Library (libtmuxcore.dylib)

#### 1.1 Create Callback Interface Header
```c
// tmux/libtmuxcore.h
#ifndef LIBTMUXCORE_H
#define LIBTMUXCORE_H

#include <stddef.h>
#include <stdint.h>

// Callback function types
typedef void (*tmc_output_cb)(void* userdata, const char* data, size_t len);
typedef void (*tmc_screen_update_cb)(void* userdata, int x, int y, 
                                      const char* cell, int attr);
typedef void (*tmc_control_notify_cb)(void* userdata, const char* notification);

// Initialize libtmuxcore with callbacks
struct tmc_callbacks {
    tmc_output_cb output;
    tmc_screen_update_cb screen_update;
    tmc_control_notify_cb control_notify;
    void* userdata;
};

int tmc_initialize(struct tmc_callbacks* callbacks);

// Core session management API
int tmc_new_session(const char* name);
int tmc_new_window(const char* session);
int tmc_split_pane(const char* target, int horizontal);
int tmc_send_keys(const char* target, const char* keys);
char* tmc_capture_pane(const char* target);
char* tmc_list_sessions(void);
char* tmc_list_windows(const char* session);

// Control mode API
int tmc_control_mode_enable(void);
int tmc_control_mode_input(const char* input);

#endif
```

#### 1.2 Modify tty_write() to Use Callbacks
```c
// tmux/tty.c - Add at line 1730, before tty_write()

#ifdef LIBTMUXCORE_BUILD
extern struct tmc_callbacks* g_tmc_callbacks;

void
tty_write(void (*cmdfn)(struct tty *, const struct tty_ctx *),
    struct tty_ctx *ctx)
{
    // Instead of writing to actual TTY, call our callback
    if (g_tmc_callbacks && g_tmc_callbacks->screen_update) {
        // Convert ctx to screen update data
        struct screen *s = ctx->s;
        if (s && ctx->cell) {
            char cell_data[32];
            utf8_to_string(ctx->cell->data, cell_data, sizeof(cell_data));
            g_tmc_callbacks->screen_update(
                g_tmc_callbacks->userdata,
                ctx->ocx, ctx->ocy,
                cell_data,
                ctx->cell->attr
            );
        }
    }
    return;  // Don't do actual TTY output
}
#else
// Original tty_write implementation
void
tty_write(void (*cmdfn)(struct tty *, const struct tty_ctx *),
    struct tty_ctx *ctx)
{
    // ... existing code ...
}
#endif
```

#### 1.3 Modify control_write() for Notifications
```c
// tmux/control.c - Add at line 405, before control_write()

#ifdef LIBTMUXCORE_BUILD
extern struct tmc_callbacks* g_tmc_callbacks;

void
control_write(struct client *c, const char *fmt, ...)
{
    va_list ap;
    char *msg;
    
    va_start(ap, fmt);
    xvasprintf(&msg, fmt, ap);
    va_end(ap);
    
    if (g_tmc_callbacks && g_tmc_callbacks->control_notify) {
        g_tmc_callbacks->control_notify(g_tmc_callbacks->userdata, msg);
    }
    
    free(msg);
}
#else
// Original control_write implementation
// ... existing code ...
#endif
```

#### 1.4 Create Makefile for Dynamic Library
```makefile
# tmux/Makefile.libtmuxcore
CC = clang
CFLAGS = -fPIC -DLIBTMUXCORE_BUILD -I. -O2
LDFLAGS = -dynamiclib -install_name @rpath/libtmuxcore.dylib

OBJS = cmd.o cmd-send-keys.o cmd-capture-pane.o cmd-list-sessions.o \
       cmd-list-windows.o cmd-new-session.o cmd-new-window.o \
       cmd-split-window.o session.o window.o grid.o \
       screen-write.o control.o control-notify.o \
       layout.o layout-custom.o layout-set.o \
       utf8.o options.o environ.o \
       libtmuxcore.o

libtmuxcore.dylib: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

libtmuxcore.o: libtmuxcore.c
	$(CC) $(CFLAGS) -c libtmuxcore.c

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o libtmuxcore.dylib
```

### Step 2: Implement tmux Library Wrapper
```c
// tmux/libtmuxcore.c
#include "tmux.h"
#include "libtmuxcore.h"

struct tmc_callbacks* g_tmc_callbacks = NULL;
struct tmuxproc* g_tmux_proc = NULL;
struct clients clients;
struct sessions sessions;

int tmc_initialize(struct tmc_callbacks* callbacks) {
    g_tmc_callbacks = callbacks;
    
    // Initialize tmux subsystems
    TAILQ_INIT(&clients);
    RB_INIT(&sessions);
    
    // Initialize options
    global_options = options_create(NULL);
    global_s_options = options_create(NULL);
    global_w_options = options_create(NULL);
    
    // Set up basic defaults
    options_set_number(global_s_options, "base-index", 0);
    options_set_number(global_w_options, "automatic-rename", 1);
    
    return 0;
}

int tmc_new_session(const char* name) {
    struct session *s;
    struct environ *env;
    
    env = environ_create();
    environ_update(global_s_options, env, NULL);
    
    s = session_create(name, NULL, NULL, env, NULL, NULL);
    if (s == NULL)
        return -1;
    
    // Notify through callback
    if (g_tmc_callbacks && g_tmc_callbacks->control_notify) {
        char msg[256];
        snprintf(msg, sizeof(msg), "%%session-changed $%u %s", s->id, s->name);
        g_tmc_callbacks->control_notify(g_tmc_callbacks->userdata, msg);
    }
    
    return s->id;
}

int tmc_send_keys(const char* target, const char* keys) {
    struct cmd_find_state fs;
    struct window_pane *wp;
    
    // Parse target (session:window.pane format)
    if (cmd_find_target(&fs, NULL, target, CMD_FIND_PANE, 0) != 0)
        return -1;
    
    wp = fs.wp;
    if (wp == NULL)
        return -1;
    
    // Inject keys character by character
    for (const char* p = keys; *p; p++) {
        struct key_event event = {0};
        event.key = *p;
        window_pane_key(wp, NULL, fs.s, fs.wl, event.key, NULL);
    }
    
    return 0;
}

char* tmc_capture_pane(const char* target) {
    struct cmd_find_state fs;
    struct window_pane *wp;
    struct screen *s;
    char *buf;
    size_t len;
    
    if (cmd_find_target(&fs, NULL, target, CMD_FIND_PANE, 0) != 0)
        return NULL;
    
    wp = fs.wp;
    s = &wp->base;
    
    // Capture screen content
    buf = grid_string_cells(s->grid, 0, 0, screen_size_x(s),
                            NULL, 0, NULL);
    
    return buf;
}

char* tmc_list_sessions(void) {
    struct session *s;
    char *result = NULL;
    size_t len = 0;
    
    RB_FOREACH(s, sessions, &sessions) {
        char line[256];
        snprintf(line, sizeof(line), "%s: %u windows (created %lld)\n",
                s->name, winlink_count(&s->windows),
                (long long)s->creation_time.tv_sec);
        
        size_t line_len = strlen(line);
        result = xrealloc(result, len + line_len + 1);
        memcpy(result + len, line, line_len);
        len += line_len;
    }
    
    if (result)
        result[len] = '\0';
    
    return result;
}
```

### Step 3: Create Zig FFI Bridge

```zig
// ghostty/src/tmux/libtmuxcore.zig
const std = @import("std");
const c = @cImport({
    @cInclude("libtmuxcore.h");
});

pub const TmuxCore = struct {
    callbacks: c.tmc_callbacks,
    allocator: std.mem.Allocator,
    
    // Callback storage
    output_buffer: std.ArrayList(u8),
    notifications: std.ArrayList([]const u8),
    
    pub fn init(allocator: std.mem.Allocator) !TmuxCore {
        var self = TmuxCore{
            .callbacks = undefined,
            .allocator = allocator,
            .output_buffer = std.ArrayList(u8).init(allocator),
            .notifications = std.ArrayList([]const u8).init(allocator),
        };
        
        // Set up callbacks
        self.callbacks = .{
            .output = outputCallback,
            .screen_update = screenUpdateCallback,
            .control_notify = controlNotifyCallback,
            .userdata = @ptrCast(*anyopaque, &self),
        };
        
        const result = c.tmc_initialize(&self.callbacks);
        if (result != 0) return error.InitializationFailed;
        
        return self;
    }
    
    pub fn deinit(self: *TmuxCore) void {
        self.output_buffer.deinit();
        for (self.notifications.items) |notification| {
            self.allocator.free(notification);
        }
        self.notifications.deinit();
    }
    
    // Callback implementations
    fn outputCallback(userdata: ?*anyopaque, data: [*c]const u8, len: usize) callconv(.C) void {
        const self = @ptrCast(*TmuxCore, @alignCast(@alignOf(TmuxCore), userdata));
        const slice = data[0..len];
        self.output_buffer.appendSlice(slice) catch {};
    }
    
    fn screenUpdateCallback(
        userdata: ?*anyopaque,
        x: c_int,
        y: c_int,
        cell: [*c]const u8,
        attr: c_int,
    ) callconv(.C) void {
        const self = @ptrCast(*TmuxCore, @alignCast(@alignOf(TmuxCore), userdata));
        // Forward to Ghostty's terminal grid
        // This is where we update Ghostty's screen with tmux content
        _ = self;
        _ = x;
        _ = y;
        _ = cell;
        _ = attr;
    }
    
    fn controlNotifyCallback(userdata: ?*anyopaque, notification: [*c]const u8) callconv(.C) void {
        const self = @ptrCast(*TmuxCore, @alignCast(@alignOf(TmuxCore), userdata));
        const str = std.mem.sliceTo(notification, 0);
        const copy = self.allocator.dupe(u8, str) catch return;
        self.notifications.append(copy) catch {};
    }
    
    // Public API
    pub fn newSession(self: *TmuxCore, name: []const u8) !u32 {
        const c_name = try self.allocator.dupeZ(u8, name);
        defer self.allocator.free(c_name);
        
        const result = c.tmc_new_session(c_name);
        if (result < 0) return error.SessionCreationFailed;
        
        return @intCast(u32, result);
    }
    
    pub fn sendKeys(self: *TmuxCore, target: []const u8, keys: []const u8) !void {
        const c_target = try self.allocator.dupeZ(u8, target);
        defer self.allocator.free(c_target);
        
        const c_keys = try self.allocator.dupeZ(u8, keys);
        defer self.allocator.free(c_keys);
        
        const result = c.tmc_send_keys(c_target, c_keys);
        if (result != 0) return error.SendKeysFailed;
    }
    
    pub fn capturePan(self: *TmuxCore, target: []const u8) ![]const u8 {
        const c_target = try self.allocator.dupeZ(u8, target);
        defer self.allocator.free(c_target);
        
        const result = c.tmc_capture_pane(c_target);
        if (result == null) return error.CaptureFailed;
        
        defer c.free(result);
        return try self.allocator.dupe(u8, std.mem.sliceTo(result, 0));
    }
    
    pub fn listSessions(self: *TmuxCore) ![]const u8 {
        const result = c.tmc_list_sessions();
        if (result == null) return "";
        
        defer c.free(result);
        return try self.allocator.dupe(u8, std.mem.sliceTo(result, 0));
    }
};
```

### Step 4: Integrate into Ghostty Terminal

```zig
// ghostty/src/terminal/Terminal.zig - Add tmux integration

const TmuxCore = @import("../tmux/libtmuxcore.zig").TmuxCore;

pub const Terminal = struct {
    // ... existing fields ...
    
    /// Optional tmux core for session management
    tmux_core: ?TmuxCore = null,
    
    /// Current tmux session/window/pane address
    tmux_address: ?[]const u8 = null,
    
    pub fn init(allocator: std.mem.Allocator) !Terminal {
        var self = Terminal{
            // ... existing initialization ...
        };
        
        // Initialize tmux core if requested
        if (config.enable_tmux_integration) {
            self.tmux_core = try TmuxCore.init(allocator);
            
            // Create default session
            const session_id = try self.tmux_core.?.newSession("ghostty");
            self.tmux_address = try std.fmt.allocPrint(
                allocator,
                "{}:0.0",
                .{session_id},
            );
        }
        
        return self;
    }
    
    pub fn deinit(self: *Terminal) void {
        if (self.tmux_core) |*core| {
            core.deinit();
        }
        // ... existing cleanup ...
    }
    
    /// Send keys through tmux session management
    pub fn sendKeys(self: *Terminal, keys: []const u8) !void {
        if (self.tmux_core) |*core| {
            if (self.tmux_address) |addr| {
                try core.sendKeys(addr, keys);
            }
        } else {
            // Fallback to direct terminal input
            try self.directInput(keys);
        }
    }
    
    /// Capture current pane content
    pub fn capturePane(self: *Terminal) ![]const u8 {
        if (self.tmux_core) |*core| {
            if (self.tmux_address) |addr| {
                return try core.capturePane(addr);
            }
        }
        // Fallback to direct screen capture
        return try self.screen.capture();
    }
    
    /// Create new tmux window
    pub fn newWindow(self: *Terminal) !void {
        if (self.tmux_core) |*core| {
            // Parse session from address
            const session = self.tmux_address.?[0..std.mem.indexOf(u8, self.tmux_address.?, ":").?];
            _ = try core.newWindow(session);
            
            // Update address to new window
            // ...
        }
    }
    
    /// Split current pane
    pub fn splitPane(self: *Terminal, horizontal: bool) !void {
        if (self.tmux_core) |*core| {
            if (self.tmux_address) |addr| {
                _ = try core.splitPane(addr, horizontal);
                
                // Update UI to show split
                // ...
            }
        }
    }
};
```

### Step 5: Build Integration

```zig
// ghostty/build.zig - Add tmux library linking

pub fn build(b: *std.Build) void {
    // ... existing build setup ...
    
    // Link libtmuxcore if tmux integration is enabled
    if (options.enable_tmux_integration) {
        exe.linkSystemLibrary("tmuxcore");
        exe.addLibraryPath("/usr/local/lib");
        exe.addIncludePath("/Users/jqwang/98-ghosttyAI/tmux");
    }
    
    // ... rest of build ...
}
```

## Testing Plan

### Phase 1: Library Compilation
```bash
cd /Users/jqwang/98-ghosttyAI/tmux
make -f Makefile.libtmuxcore
otool -L libtmuxcore.dylib  # Verify dynamic library
nm libtmuxcore.dylib | grep tmc_  # Check exported symbols
```

### Phase 2: FFI Bridge Testing
```zig
// Test basic tmux operations
test "tmux core initialization" {
    var core = try TmuxCore.init(testing.allocator);
    defer core.deinit();
    
    const session_id = try core.newSession("test");
    try testing.expect(session_id > 0);
}

test "send keys and capture" {
    var core = try TmuxCore.init(testing.allocator);
    defer core.deinit();
    
    const session_id = try core.newSession("test");
    const addr = try std.fmt.allocPrint(testing.allocator, "{}:0.0", .{session_id});
    defer testing.allocator.free(addr);
    
    try core.sendKeys(addr, "echo hello");
    const output = try core.capturePane(addr);
    defer testing.allocator.free(output);
    
    try testing.expect(std.mem.indexOf(u8, output, "hello") != null);
}
```

### Phase 3: Integration Testing
```bash
# Test with actual Ghostty
cd /Users/jqwang/98-ghosttyAI/ghostty
zig build -Denable-tmux-integration=true
./zig-out/bin/ghostty

# Inside Ghostty, test tmux commands
# These should work through the embedded library:
tmux new-session -s test
tmux send-keys -t test:0 "echo hello" Enter
tmux capture-pane -t test:0 -p
```

## Performance Validation

### Baseline Metrics (from Week 2)
- Throughput: 380k ops/s
- Event Loop Overhead: 0.8%
- FFI Latency: <100ns
- Memory per Session: 8.3MB

### Validation Tests
```c
// Benchmark callback overhead
void benchmark_callbacks() {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < 1000000; i++) {
        tmc_send_keys("test:0", "a");
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    // Should maintain <100ns per operation
}
```

## Risk Mitigation

1. **Symbol Conflicts**: Use namespace prefix `tmc_` for all exported functions
2. **Memory Management**: Clear ownership boundaries at FFI interface
3. **Thread Safety**: Single-threaded event loop model
4. **Performance**: Direct callback invocation, no intermediate queues

## Success Criteria

✅ libtmuxcore.dylib compiles and links
✅ All tmux session management commands work
✅ No UI takeover - Ghostty rendering preserved
✅ Existing automation scripts work unchanged
✅ Performance within 5% of baseline
✅ No memory leaks in 24-hour test

## Next Steps

1. **Immediate**: Compile libtmuxcore.dylib with minimal functions
2. **Day 2**: Implement FFI bridge and test basic operations
3. **Day 3**: Integrate into Ghostty Terminal.zig
4. **Day 4**: Test with actual automation scripts
5. **Day 5**: Performance optimization and memory leak testing