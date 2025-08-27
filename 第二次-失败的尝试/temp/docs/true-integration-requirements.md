# Ghostty × tmux 真正的集成需求：可编程会话控制

## 核心洞察：tmux = 终端会话编排平台

基于你提供的使用场景，tmux的真正价值不是UI（分屏），而是**可编程的会话控制API**。

## 你的实际使用场景

### 1. 自动化任务调度
```bash
# 延迟3分钟后向特定窗口发送命令
sleep 180 && tmux send-keys -t tmux-orc:0 'python3 claude_control.py status' Enter
```

### 2. Claude Agent编排
```bash
# 向不同的Claude实例发送任务
tmux send-keys -t "agentic-seek:3" "Research this topic..."
tmux send-keys -t "coder-agent:0" "Implement this feature..."
```

### 3. 状态监控与捕获
```bash
# 捕获agent输出，分析进度
tmux capture-pane -t ghostty-core:0 -p | grep "COMPLETE"
```

## tmux核心API集合（44个文件）

```
tmux-only/
├── control/ (9个文件) ⭐⭐⭐ 最重要
│   ├── cmd-send-keys.c        # 发送按键/命令到指定pane
│   ├── cmd-capture-pane.c     # 捕获pane的文本内容
│   ├── cmd-pipe-pane.c        # 管道化pane输出
│   ├── control.c              # 控制模式协议
│   ├── control-notify.c       # 事件通知系统
│   ├── cmd-list-sessions.c    # 列出所有会话
│   ├── cmd-list-windows.c     # 列出所有窗口
│   ├── cmd-list-panes.c       # 列出所有面板
│   └── cmd-list-clients.c     # 列出连接的客户端
│
├── session/ (7个文件)
├── window/ (6个文件)
├── pane/ (6个文件)
├── layout/ (4个文件)
├── commands/ (3个文件)
└── support/ (9个文件)
```

## Ghostty集成架构 - 支持会话控制

### 1. 会话寻址系统
```zig
// Ghostty需要实现tmux的寻址格式
pub const Address = struct {
    session: ?[]const u8,     // session名称
    window: ?usize,           // window索引或名称
    pane: ?usize,            // pane索引
    
    // 解析 "session:window.pane" 格式
    pub fn parse(target: []const u8) Address {
        // 支持各种格式：
        // "0" - 当前session的window 0
        // "mysession:" - mysession的当前window
        // "mysession:2.1" - mysession的window 2的pane 1
    }
};
```

### 2. 命令注入能力
```zig
pub const CommandInjector = struct {
    // 核心功能：向任意pane发送输入
    pub fn sendKeys(target: []const u8, keys: []const u8) !void {
        const addr = Address.parse(target);
        const pane = try findPane(addr);
        
        // 处理特殊按键
        if (std.mem.eql(u8, keys, "Enter")) {
            try pane.terminal.processKey(.enter);
        } else if (std.mem.startsWith(u8, keys, "C-")) {
            // 处理Ctrl组合键
        } else {
            // 普通文本
            try pane.terminal.processText(keys);
        }
    }
    
    // 直接执行shell命令
    pub fn executeCommand(target: []const u8, cmd: []const u8) !void {
        const pane = try findPane(target);
        const full_cmd = try std.fmt.allocPrint(allocator, "{s}\n", .{cmd});
        try pane.pty.write(full_cmd);
    }
};
```

### 3. 输出捕获能力
```zig
pub const OutputCapture = struct {
    // 捕获pane的文本内容
    pub fn capturePane(target: []const u8, options: CaptureOptions) ![]const u8 {
        const pane = try findPane(target);
        
        if (options.join_lines) {
            return pane.terminal.screen.getJoinedText();
        } else if (options.escape_sequences) {
            return pane.terminal.screen.getAnsiText();
        } else {
            return pane.terminal.screen.getPlainText();
        }
    }
    
    // 持续监控输出
    pub fn pipePane(target: []const u8, callback: fn([]const u8) void) !void {
        const pane = try findPane(target);
        pane.addOutputListener(callback);
    }
};
```

### 4. 会话管理能力
```zig
pub const SessionManager = struct {
    sessions: std.StringHashMap(*Session),
    
    // 创建持久会话
    pub fn newSession(name: []const u8, options: SessionOptions) !*Session {
        const session = try Session.create(name);
        session.persistent = options.detach; // 可以断开重连
        try self.sessions.put(name, session);
        return session;
    }
    
    // 列出所有会话（供脚本查询）
    pub fn listSessions() ![]const SessionInfo {
        var list = std.ArrayList(SessionInfo).init(allocator);
        var it = self.sessions.iterator();
        while (it.next()) |entry| {
            try list.append(.{
                .name = entry.key_ptr.*,
                .windows = entry.value_ptr.*.windows.items.len,
                .attached = entry.value_ptr.*.hasClient(),
                .created = entry.value_ptr.*.created_time,
            });
        }
        return list.toOwnedSlice();
    }
    
    // 附加到现有会话
    pub fn attachSession(name: []const u8) !void {
        const session = self.sessions.get(name) orelse return error.SessionNotFound;
        try self.setCurrentSession(session);
    }
};
```

### 5. 脚本接口层
```zig
// 暴露给外部脚本调用的接口
pub const ScriptAPI = struct {
    // 通过命令行工具调用
    pub fn handleCommand(args: [][]const u8) !void {
        const cmd = args[0];
        
        if (std.mem.eql(u8, cmd, "send-keys")) {
            const target = args[1];
            const keys = args[2];
            try CommandInjector.sendKeys(target, keys);
            
        } else if (std.mem.eql(u8, cmd, "capture-pane")) {
            const target = args[1];
            const output = try OutputCapture.capturePane(target, .{});
            try std.io.getStdOut().writeAll(output);
            
        } else if (std.mem.eql(u8, cmd, "list-sessions")) {
            const sessions = try SessionManager.listSessions();
            try printSessions(sessions);
        }
    }
};
```

## 实际使用效果

### 在Ghostty中运行你的脚本
```bash
# 你的原始脚本可以直接工作！
./schedule_with_note.sh 3 "Check agent progress" "ghostty:0"

# Ghostty内部处理：
# 1. 解析 "ghostty:0" 地址
# 2. 找到对应的pane
# 3. 3分钟后注入命令
# 4. 执行并显示结果
```

### 新的Ghostty命令行工具
```bash
# Ghostty提供兼容tmux的命令接口
ghostty send-keys -t "session:0" "Hello World" Enter
ghostty capture-pane -t "session:1" -p
ghostty list-sessions
ghostty new-session -s "dev-env" -d
```

## 实施优先级

### Phase 1: 核心控制API (最紧急)
1. ✅ `send-keys` - 发送输入
2. ✅ `capture-pane` - 捕获输出
3. ✅ 会话寻址系统

### Phase 2: 会话管理
1. `new-session` - 创建会话
2. `attach-session` - 附加会话
3. `list-sessions` - 列出会话

### Phase 3: 高级功能
1. `pipe-pane` - 持续输出
2. 控制模式协议
3. 事件通知系统

## 为什么这才是真正的需求

你的使用场景表明，tmux对你的价值是：

1. **编排多个AI Agent** - 每个在独立的pane中运行
2. **自动化交互** - 定时发送命令，捕获响应
3. **状态监控** - 实时查看各个任务的进度
4. **持久化会话** - 断开后任务继续运行

这不是简单的"分屏"，而是一个**可编程的终端会话编排平台**！

Ghostty集成tmux后，将成为一个强大的AI Agent运行环境。