# Ghostty终端间通信实现方案

## 1. 核心设计理念

将Ghostty的App层转变为类似tmux server的通信中枢，实现终端A ↔ 终端B的双向通信。

## 2. 实现架构

```
终端A (Surface A)          App层（通信中枢）         终端B (Surface B)
      │                         │                          │
      │  1. 发送命令            │                          │
      ├────────────────────────▶│                          │
      │  (通过快捷键/命令)       │  2. 路由到目标          │
      │                         ├─────────────────────────▶│
      │                         │     (写入PTY)            │
      │                         │                          │ 3. 执行命令
      │                         │                          │
      │  5. 接收结果            │  4. 返回输出            │
      │◀────────────────────────┼──────────────────────────│
      │                         │                          │
```

## 3. 具体实现步骤

### 3.1 扩展IPC Actions（apprt/ipc.zig）

```zig
// 在 apprt/ipc.zig 添加新的Actions
pub const Action = union(enum) {
    new_window: NewWindow,
    
    // 新增：发送到会话
    send_to_session: SendToSession,
    
    // 新增：链接会话（建立双向通信）
    link_sessions: LinkSessions,
    
    // 新增：列出所有会话
    list_sessions: ListSessions,
    
    pub const SendToSession = struct {
        session_id: []const u8,  // 目标会话ID
        data: []const u8,         // 要发送的数据
        wait_response: bool,      // 是否等待响应
    };
    
    pub const LinkSessions = struct {
        source_id: []const u8,    // 源会话ID
        target_id: []const u8,    // 目标会话ID
        bidirectional: bool,      // 是否双向链接
    };
};
```

### 3.2 在App层添加SessionManager（App.zig）

```zig
// 在 App.zig 中添加
pub const SessionManager = struct {
    const SessionMap = std.hash_map.HashMap(
        SessionId,
        *Surface,
        std.hash_map.StringContext,
        80
    );
    
    const SessionLink = struct {
        source: SessionId,
        target: SessionId,
        bidirectional: bool,
        filter: ?*const fn(data: []const u8) bool,  // 可选的数据过滤器
    };
    
    sessions: SessionMap,
    links: std.ArrayList(SessionLink),
    
    pub fn init(alloc: Allocator) SessionManager {
        return .{
            .sessions = SessionMap.init(alloc),
            .links = std.ArrayList(SessionLink).init(alloc),
        };
    }
    
    /// 注册一个Surface作为可通信的会话
    pub fn registerSession(self: *SessionManager, id: []const u8, surface: *Surface) !void {
        try self.sessions.put(try self.alloc.dupe(u8, id), surface);
    }
    
    /// 发送数据到目标会话
    pub fn sendToSession(
        self: *SessionManager,
        target_id: []const u8,
        data: []const u8,
    ) !void {
        const target_surface = self.sessions.get(target_id) orelse 
            return error.SessionNotFound;
        
        // 直接写入目标终端的PTY（类似tmux的bufferevent_write）
        try target_surface.io.backend.write(data);
    }
    
    /// 建立会话间的链接
    pub fn linkSessions(
        self: *SessionManager,
        source_id: []const u8,
        target_id: []const u8,
        bidirectional: bool,
    ) !void {
        // 验证两个会话都存在
        if (self.sessions.get(source_id) == null or 
            self.sessions.get(target_id) == null) {
            return error.SessionNotFound;
        }
        
        try self.links.append(.{
            .source = try self.alloc.dupe(u8, source_id),
            .target = try self.alloc.dupe(u8, target_id),
            .bidirectional = bidirectional,
            .filter = null,
        });
    }
    
    /// 处理来自终端的输出，并路由到链接的会话
    pub fn routeOutput(
        self: *SessionManager,
        from_session: []const u8,
        output: []const u8,
    ) !void {
        for (self.links.items) |link| {
            if (std.mem.eql(u8, link.source, from_session)) {
                try self.sendToSession(link.target, output);
            } else if (link.bidirectional and std.mem.eql(u8, link.target, from_session)) {
                try self.sendToSession(link.source, output);
            }
        }
    }
};
```

### 3.3 修改Surface以支持会话ID（Surface.zig）

```zig
// 在 Surface.zig 添加
pub const Surface = struct {
    // ... 现有字段 ...
    
    /// 会话标识符（用于终端间通信）
    session_id: ?[]const u8 = null,
    
    /// 是否启用输出路由
    output_routing_enabled: bool = false,
    
    pub fn init(...) !Surface {
        // ... 现有初始化代码 ...
        
        // 生成唯一的会话ID
        var buf: [32]u8 = undefined;
        const id = try std.fmt.bufPrint(&buf, "surface-{}-{}", .{
            std.time.milliTimestamp(),
            self.rt_surface,
        });
        self.session_id = try alloc.dupe(u8, id);
        
        // 注册到App的SessionManager
        try self.app.session_manager.registerSession(self.session_id.?, &self);
    }
    
    /// 拦截终端输出并路由
    pub fn interceptOutput(self: *Surface, data: []const u8) !void {
        if (self.output_routing_enabled and self.session_id) |id| {
            try self.app.session_manager.routeOutput(id, data);
        }
    }
};
```

### 3.4 实现命令接口

```zig
// 添加新的命令处理
pub fn handleTerminalCommand(self: *Surface, cmd: []const u8) !void {
    // 解析ghostty特殊命令
    if (std.mem.startsWith(u8, cmd, "@send ")) {
        // @send session-id "command"
        const parts = try parseCommand(cmd);
        const target_id = parts[1];
        const data = parts[2];
        
        try self.app.session_manager.sendToSession(target_id, data);
    } else if (std.mem.startsWith(u8, cmd, "@link ")) {
        // @link session-id
        const target_id = cmd[6..];
        if (self.session_id) |my_id| {
            try self.app.session_manager.linkSessions(my_id, target_id, true);
        }
    } else if (std.mem.eql(u8, cmd, "@sessions")) {
        // 列出所有活动会话
        try self.listActiveSessions();
    }
}
```

## 4. SSH远程通信支持

### 4.1 利用OSC序列建立通信通道

```zig
// 在终端中注入OSC序列来建立通信
pub fn setupRemoteCommunication(self: *Surface) !void {
    // 使用OSC 777（Ghostty专用）建立通信通道
    const setup_sequence = "\x1b]777;ghostty-comm;setup;{}\x07";
    try self.io.backend.write(std.fmt.allocPrint(
        self.alloc,
        setup_sequence,
        .{self.session_id.?}
    ));
}

// 处理来自远程终端的OSC响应
pub fn handleOSC777(self: *Surface, params: []const u8) !void {
    if (std.mem.startsWith(u8, params, "ghostty-comm;")) {
        const cmd = params[13..];
        if (std.mem.startsWith(u8, cmd, "data;")) {
            // 接收远程终端发送的数据
            const data = cmd[5..];
            try self.processRemoteData(data);
        }
    }
}
```

### 4.2 SSH环境变量传递

```bash
# 修改 shell-integration/zsh/ghostty-integration
if [[ "$GHOSTTY_SHELL_FEATURES" == *ssh-* ]]; then
    ssh() {
        # 添加Ghostty会话信息
        ssh_opts+=(-o "SetEnv GHOSTTY_SESSION_ID=$GHOSTTY_SESSION_ID")
        ssh_opts+=(-o "SetEnv GHOSTTY_COMM_ENABLED=1")
        
        # 在远程端自动设置通信
        command ssh "${ssh_opts[@]}" "$@" -t "
            export GHOSTTY_SESSION_ID='$GHOSTTY_SESSION_ID-remote'
            export GHOSTTY_COMM_ENABLED=1
            exec \$SHELL -l
        "
    }
fi
```

## 5. 用户使用示例

### 5.1 本地终端间通信

```bash
# 终端A
$ ghostty --session-id terminal-a

# 终端B  
$ ghostty --session-id terminal-b

# 在终端A中发送命令到终端B
$ echo "ls -la" | ghostty send terminal-b

# 或使用内置命令
$ @send terminal-b "echo 'Hello from A'"

# 建立双向链接
$ @link terminal-b

# 之后所有输出都会同步到终端B
```

### 5.2 SSH远程通信

```bash
# 本地终端A
$ ghostty --session-id local-terminal

# SSH连接（会自动传递会话信息）
$ ssh user@remote-host

# 在远程终端中
remote$ @register-remote local-terminal

# 现在可以从本地发送命令到远程
local$ @send remote "ls /etc"

# 或从远程发送回本地
remote$ @send local-terminal "echo 'Data from remote'"
```

### 5.3 实用场景

1. **协作编程**：两个终端实时同步，一个编码，一个查看输出
2. **远程协助**：通过SSH连接后，本地可以直接发送命令到远程执行
3. **自动化测试**：一个终端运行测试，另一个终端监控日志
4. **教学演示**：教师终端的操作实时同步到学生终端

## 6. 实现优势

1. **原生集成**：不需要额外的tmux进程，直接在Ghostty内实现
2. **更好的性能**：减少了进程间通信开销
3. **更灵活的控制**：可以实现更细粒度的过滤和路由
4. **SSH透明支持**：通过OSC序列，SSH连接也能参与通信网络
5. **保持简洁**：用户界面简单，不需要学习tmux的复杂命令

## 7. 实现路线图

### Phase 1：基础通信（1周）
- [ ] 扩展IPC Actions
- [ ] 实现SessionManager
- [ ] 基本的send_to_session功能

### Phase 2：双向链接（1周）
- [ ] 实现link_sessions
- [ ] 输出拦截和路由
- [ ] 会话管理命令

### Phase 3：SSH支持（2周）
- [ ] OSC 777通信协议
- [ ] Shell integration集成
- [ ] 远程会话注册

### Phase 4：高级功能（2周）
- [ ] 数据过滤器
- [ ] 会话组管理
- [ ] 持久化会话信息

这个方案完全可行，而且比tmux更加优雅！