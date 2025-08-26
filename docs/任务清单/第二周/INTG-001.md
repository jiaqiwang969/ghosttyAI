# INTG-001 第二周任务清单
## zig-ghostty-integration Week 2 Tasks

### 👤 角色定位
**专长**: Zig语言，C-FFI，内存安全，Ghostty集成  
**本周重点**: 完成FFI桥接，实现Ghostty中运行tmux

### 📋 核心任务

## T-301: Zig-C FFI绑定 【最高优先级】
**工期**: 3天 (周二-周四)  
**状态**: 待开始  
**前置依赖**: T-201事件循环接口

### 输入分析
```bash
# 需要绑定的C接口
cache/week1/ARCH-001/ui_backend.h
cache/week1/CORE-001/fixed/tty_write_hooks.h
cache/week1/CORE-002/backend_router.h
cache/week2/CORE-001/event_loop_backend.h (周三获得)
cache/week2/CORE-002/grid_callbacks.h (周四获得)
```

### 输出交付物
```
cache/week2/INTG-001/
├── ffi/
│   ├── c_types.zig          # C类型映射
│   ├── c_interop.zig        # C互操作层
│   ├── callbacks.zig        # 回调包装器
│   └── memory.zig           # 内存管理
├── bindings/
│   ├── ui_backend.zig       # UI后端绑定
│   ├── event_loop.zig       # 事件循环绑定
│   └── grid_ops.zig         # 网格操作绑定
└── tests/
    ├── test_ffi.zig         # FFI测试
    └── test_memory.zig      # 内存安全测试
```

### 实现计划

#### 周二 (8/27) - C类型映射
```zig
// c_types.zig
pub const TtyCtx = extern struct {
    size: u32,
    version: u32,
    ocx: u32,
    ocy: u32,
    orupper: u32,
    orlower: u32,
    wp: ?*anyopaque,
    // ... 其他字段
};

pub const UiBackendOps = extern struct {
    size: usize,
    version: u32,
    cmd_cell: ?*const fn(*UiBackend, *const TtyCtx) callconv(.C) void,
    cmd_cells: ?*const fn(*UiBackend, *const TtyCtx) callconv(.C) void,
    // ... 22个函数指针
};
```

#### 周三 (8/28) - 回调包装器
```zig
// callbacks.zig
const std = @import("std");

pub fn wrapCallback(comptime T: type, zig_fn: anytype) T {
    return struct {
        fn wrapper(args: anytype) callconv(.C) void {
            // 错误处理
            zig_fn(args) catch |err| {
                std.log.err("Callback error: {}", .{err});
            };
        }
    }.wrapper;
}

// 安全的函数指针转换
pub fn createUiBackendOps(impl: anytype) *UiBackendOps {
    var ops = std.heap.c_allocator.create(UiBackendOps) catch unreachable;
    ops.cmd_cell = wrapCallback(@TypeOf(ops.cmd_cell), impl.cmdCell);
    // ...
    return ops;
}
```

#### 周四 (8/29) - 内存管理
```zig
// memory.zig
const std = @import("std");

pub const SafeAllocator = struct {
    c_allocator: std.mem.Allocator,
    
    pub fn alloc(self: *SafeAllocator, comptime T: type, n: usize) ![]T {
        const mem = try self.c_allocator.alloc(T, n);
        // 清零初始化
        @memset(mem, 0);
        return mem;
    }
    
    pub fn free(self: *SafeAllocator, ptr: anytype) void {
        self.c_allocator.free(ptr);
    }
};
```

### 验收标准
- [ ] 所有C结构体正确映射
- [ ] 函数指针类型安全
- [ ] 内存边界检查100%
- [ ] 错误处理完善
- [ ] 零内存泄漏

---

## T-302: Ghostty集成层 【高优先级】
**工期**: 3天 (周三-周五)  
**状态**: 待T-301部分完成

### 输出交付物
```
cache/week2/INTG-001/
├── ghostty/
│   ├── tmux_integration.zig    # 主集成模块
│   ├── terminal_backend.zig    # 终端后端实现
│   ├── event_handler.zig       # 事件处理
│   └── renderer.zig            # 渲染接口
└── examples/
    └── basic_tmux.zig          # 基础示例
```

### 实现步骤

#### 周三下午 - 初步集成
```zig
// tmux_integration.zig
const std = @import("std");
const c = @import("../ffi/c_interop.zig");
const ghostty = @import("ghostty");

pub const TmuxIntegration = struct {
    backend: *c.UiBackend,
    terminal: *ghostty.Terminal,
    event_loop: *c.EventLoop,
    
    pub fn init(terminal: *ghostty.Terminal) !*TmuxIntegration {
        var self = try std.heap.c_allocator.create(TmuxIntegration);
        self.terminal = terminal;
        
        // 初始化libtmuxcore
        self.backend = c.create_ghostty_backend();
        self.event_loop = c.event_loop_create();
        
        // 注册回调
        c.backend_router_register_ui(self.backend);
        
        return self;
    }
    
    pub fn run(self: *TmuxIntegration) !void {
        // 主循环
        while (true) {
            try self.processEvents();
            try self.render();
        }
    }
};
```

#### 周四全天 - 事件和渲染
```zig
// event_handler.zig
pub fn processInput(self: *TmuxIntegration, input: ghostty.Input) !void {
    switch (input) {
        .key => |k| try self.sendKey(k),
        .mouse => |m| try self.sendMouse(m),
        .resize => |s| try self.resize(s),
    }
}

// renderer.zig
pub fn render(self: *TmuxIntegration) !void {
    const frame = self.backend.getCurrentFrame();
    try self.terminal.renderFrame(frame);
}
```

#### 周五 - 完善和测试
- 错误恢复机制
- 性能优化
- 内存泄漏检查

### 验收标准
- [ ] tmux会话成功启动
- [ ] 输入输出正常工作
- [ ] 无崩溃运行1小时
- [ ] 内存使用稳定

---

## 📅 每日详细计划

### 周一 (8/26)
- 09:00 - 周会，确认任务优先级
- 10:00 - 回顾第一周backend_ghostty.c
- 14:00 - 准备FFI设计文档
- 16:00 - 搭建Zig测试环境
- 17:00 - 日报

### 周二 (8/27)
- 09:00 - 站会
- 09:30 - 开始T-301 C类型映射
- 11:00 - 实现c_types.zig
- 14:00 - 实现c_interop.zig基础
- 16:00 - 单元测试
- 17:00 - 与CORE-001同步

### 周三 (8/28)
- 09:00 - 站会
- 09:30 - 实现callbacks.zig
- 11:00 - 获取event_loop_backend.h
- 14:00 - 开始T-302初步集成
- 16:00 - 测试基础集成
- 17:00 - 风险评估

### 周四 (8/29)
- 09:00 - 站会
- 09:30 - 完成memory.zig
- 11:00 - 获取grid_callbacks.h
- 14:00 - 实现事件处理
- 16:00 - 实现渲染接口
- 17:00 - 集成测试

### 周五 (8/30)
- 09:00 - 站会
- 09:30 - 错误处理完善
- 11:00 - 性能测试
- 14:00 - 最终集成测试
- 15:00 - Demo准备
- 16:00 - 文档整理
- 17:00 - Demo展示

## 🔄 协作依赖

### 关键输入
| 时间 | 需要获取 | 来源 |
|------|----------|------|
| 周三10:00 | event_loop_backend.h | CORE-001 |
| 周四10:00 | grid_callbacks.h | CORE-002 |
| 周四14:00 | 性能基准 | INTG-003 |

### 关键输出
| 时间 | 交付内容 | 交给 |
|------|----------|------|
| 周四17:00 | FFI bindings | QA-001 |
| 周五14:00 | 集成完成 | ALL |
| 周五17:00 | Demo | PM |

## 🎯 性能目标
- FFI调用开销: <100ns
- 内存overhead: <5%
- 首次渲染: <16ms
- 稳定帧率: 60fps

## 🚨 风险管理

| 风险 | 影响 | 缓解措施 |
|------|------|----------|
| C ABI不兼容 | 高 | 严格测试每个结构体 |
| 内存泄漏 | 高 | 使用Zig的defer和errdefer |
| 性能问题 | 中 | 提前做micro-benchmark |
| Ghostty API变化 | 低 | 抽象层隔离 |

## 📝 测试计划

### 单元测试
- [ ] 每个FFI函数测试
- [ ] 内存分配/释放测试
- [ ] 错误处理测试

### 集成测试
- [ ] 基础tmux命令
- [ ] 窗口分割
- [ ] 复制模式
- [ ] 1小时稳定性测试

### 性能测试
- [ ] FFI调用benchmark
- [ ] 渲染性能测试
- [ ] 内存使用监控

## ✅ 完成标准
- [ ] T-301 100%完成
- [ ] T-302 100%完成
- [ ] 0内存泄漏
- [ ] Demo可展示
- [ ] 文档完整

---

**角色**: INTG-001 (zig-ghostty-integration)  
**更新日期**: 2025-08-25