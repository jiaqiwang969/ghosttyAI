# 第三周施工方案 - Agent执行手册
# Week 3 Construction - Agent Execution Manual

**版本**: 2.0 (Unified Executor)  
**创建时间**: 2025-08-26 23:00  
**目标**: 由单一agent完成所有集成工作，避免协作开销

---

## 👤 统一执行者：week3-ghostty-tmux-executor

### 执行优势
- **无需协调**: 一个agent掌握全局，无沟通成本
- **快速迭代**: 直接修改，无需等待其他agent
- **全栈能力**: 从C到Zig，从构建到测试
- **资源集中**: 所有Week 2成果统一管理

---

## 🎯 week3-ghostty-tmux-executor 完整任务清单

### 你需要完成的所有任务
1. **T-301-R**: 修改tmux源码集成backend router (Day 1-2)
2. **T-302-R**: 构建libtmuxcore动态库 (Day 2-3)
3. **T-303-R**: 创建Ghostty tmux集成模块 (Day 3-4)
4. **T-304-R**: Terminal模块集成tmux (Day 4)
5. **T-305-R**: 端到端集成测试 (Day 5)
6. **T-306-R**: 性能验证与优化 (Day 6)
7. **T-307-R**: 架构评审 (Day 7)

---

## 📚 统一资源索引

#### 你的任务：修改tmux源码集成backend router
**工期**: Day 1-2  
**Session**: ghostty-core:0

#### 必读文档（按顺序）
1. `/docs/architecture-view/tty-write-interception.md` - 核心设计
2. `/docs/architecture-view/ui-backend-design.md` - 接口规范
3. `cache/week1/CORE-001/backend_router_draft.c` - 你第一周的POC代码

#### 可复用代码
```bash
# 你第一周的探索代码
cat cache/week1/CORE-001/hook_extraction_poc.c    # 22个函数列表
cat cache/week1/CORE-001/backend_router_draft.c   # 路由框架

# 第二周验证的vtable
cat cache/week2/CORE-001/src/event_loop_backend.h # 直接复用！
```

#### 具体步骤
```bash
# Step 1: 分析tmux源码
cd /Users/jqwang/98-ghosttyAI/tmux/
grep -n "tty_write" tty.c                         # 找到Line 1234附近

# Step 2: 创建backend目录
mkdir -p ui_backend/
cp ~/98-ghosttyAI/cache/week2/CORE-001/src/event_loop_backend.h ui_backend/

# Step 3: 修改tty.c
vim tty.c
# 在tty_write函数添加:
#ifdef LIBTMUXCORE_BUILD
    if (ctx->tty->backend && ui_backend_enabled()) {
        ui_backend_dispatch(ctx->tty->backend, cmdfn, ctx);
        return;
    }
#endif

# Step 4: 验证编译
make CFLAGS="-DLIBTMUXCORE_BUILD"
```

#### 交付标准
- [ ] tty.c修改完成，保留原路径
- [ ] ui_backend/目录创建，包含所有头文件
- [ ] 条件编译测试通过
- [ ] 22个tty_cmd_*函数都能路由

---

### CORE-002 (libtmux-core-developer) - T-302-R执行手册

#### 你的任务：构建libtmuxcore动态库
**工期**: Day 2-3  
**Session**: ghostty-core:4

#### 必读文档
1. `/docs/architecture-view/abi-stability.md` - ABI设计原则
2. `cache/week2/Makefile` - 构建模板
3. `/project_spec.md` - 动态库需求（第24行）

#### 可复用资源
```bash
# 第二周的网格优化代码
cache/week2/CORE-002/src/grid_operations_neon.c  # ARM优化，45KB
cache/week2/CORE-002/src/layout_management.c     # 布局管理，8KB

# 构建配置
cache/week2/Makefile                              # 含动态库规则
cache/week2/libtmuxcore.sym                       # 符号版本脚本
```

#### 执行步骤
```bash
# Step 1: 创建Makefile.libtmuxcore
cd /Users/jqwang/98-ghosttyAI/tmux/
cat > Makefile.libtmuxcore << 'EOF'
LIBTMUX_OBJS = tty.o grid.o screen.o window.o session.o \
               ui_backend/event_loop_backend.o \
               grid_operations_neon.o

libtmuxcore.so.1.0.0: $(LIBTMUX_OBJS)
    $(CC) -shared -fPIC \
          -Wl,--version-script=libtmuxcore.sym \
          -Wl,-soname,libtmuxcore.so.1 \
          -o $@ $^ $(LDFLAGS)

libtmuxcore.so.1: libtmuxcore.so.1.0.0
    ln -sf $< $@

libtmuxcore.so: libtmuxcore.so.1
    ln -sf $< $@
EOF

# Step 2: 复制优化代码
cp ~/98-ghosttyAI/cache/week2/CORE-002/src/grid_operations_neon.c .

# Step 3: 创建符号版本脚本
cat > libtmuxcore.sym << 'EOF'
LIBTMUXCORE_1.0 {
    global:
        tmc_init;
        tmc_cleanup;
        tmc_create_session;
        tmc_*;
    local: *;
};
EOF

# Step 4: 构建
make -f Makefile.libtmuxcore
```

#### 交付标准
- [ ] libtmuxcore.so.1.0.0 生成成功
- [ ] 符号正确导出（nm -D检查）
- [ ] pkg-config文件创建
- [ ] 安装脚本编写完成

---

### INTG-001 (zig-ghostty-integration) - T-303-R执行手册

#### 你的任务：创建Ghostty的tmux集成模块
**工期**: Day 3-4  
**Session**: ghostty-integration:0

#### 必读文档
1. `cache/week2/INTG-001/callbacks.zig` - 你的FFI实现（31KB）
2. `cache/week2/INTG-001/ffi_safety.zig` - 内存安全层（15KB）
3. `/docs/architecture-view/frame-batching.md` - 帧批处理设计

#### 可复用代码
```zig
// 你第二周实现的核心callback系统
// cache/week2/INTG-001/callbacks.zig

pub const Callbacks = struct {
    vtable: c.tmc_ui_vtable_t,
    frame_buffer: FrameBuffer,
    pending_spans: [1024]Span,
    span_count: usize,
    
    // 这个onFrame实现已经验证，直接用！
    pub fn onFrame(...) callconv(.C) void {
        // 31KB的实现细节
    }
};
```

#### 执行步骤
```bash
# Step 1: 创建tmux模块目录
cd /Users/jqwang/98-ghosttyAI/ghostty/
mkdir -p src/tmux

# Step 2: 迁移FFI代码
cp ~/98-ghosttyAI/cache/week2/INTG-001/callbacks.zig src/tmux/
cp ~/98-ghosttyAI/cache/week2/INTG-001/ffi_safety.zig src/tmux/

# Step 3: 创建核心集成文件
cat > src/tmux/core.zig << 'EOF'
const std = @import("std");
const c = @cImport({
    @cInclude("libtmuxcore.h");
});
const Callbacks = @import("callbacks.zig").Callbacks;

pub const TmuxCore = struct {
    handle: *c.tmc_handle_t,
    callbacks: Callbacks,
    allocator: std.mem.Allocator,
    
    pub fn init(allocator: std.mem.Allocator) !TmuxCore {
        // 使用week2验证的初始化流程
        const handle = c.tmc_init() orelse return error.InitFailed;
        
        var self = TmuxCore{
            .handle = handle,
            .callbacks = try Callbacks.init(allocator),
            .allocator = allocator,
        };
        
        // 注册回调
        c.tmc_register_callbacks(handle, &self.callbacks.vtable);
        
        return self;
    }
};
EOF

# Step 4: 更新build.zig
vim build.zig
# 添加:
# exe.addLibraryPath("/usr/local/lib");
# exe.linkSystemLibrary("tmuxcore");
```

#### 交付标准
- [ ] src/tmux/目录结构完整
- [ ] FFI callbacks移植成功
- [ ] 内存安全层集成
- [ ] build.zig正确链接libtmuxcore

---

### INTG-002 (integration-dev) - T-304-R执行手册

#### 你的任务：Terminal模块集成tmux
**工期**: Day 4  
**Session**: ghostty-integration:4

#### 必读资源
1. `cache/week2/INTG-002/terminal_integration_poc.zig` - 集成原型
2. `ghostty/src/terminal/Terminal.zig` - 目标文件
3. `cache/week2/测试报告.md` - 集成点验证

#### 执行步骤
```zig
// 修改 ghostty/src/terminal/Terminal.zig

const TmuxCore = @import("../tmux/core.zig").TmuxCore;

pub const Terminal = struct {
    // 添加字段
    tmux_core: ?TmuxCore,
    tmux_enabled: bool,
    
    // 在init中条件初始化
    pub fn init(allocator: std.mem.Allocator, config: Config) !Terminal {
        var self = Terminal{
            // ... existing fields
            .tmux_core = null,
            .tmux_enabled = config.enable_tmux,
        };
        
        if (config.enable_tmux) {
            self.tmux_core = try TmuxCore.init(allocator);
        }
        
        return self;
    }
    
    // 添加tmux命令处理
    pub fn processTmuxCommand(self: *Terminal, cmd: []const u8) !void {
        if (self.tmux_core) |*tmux| {
            try tmux.executeCommand(cmd);
        }
    }
};
```

---

### QA-002 (qa-test-engineer) - T-305-R执行手册

#### 你的任务：端到端集成测试
**工期**: Day 5  
**Session**: ghostty-quality:1

#### 可复用测试资源
```bash
# 第二周91%覆盖率的测试套件
cache/week2/TESTS/
├── integration_tests/     # 12个集成测试
├── performance_tests/     # 8个性能测试
├── memory_tests/         # Valgrind配置
└── coverage_report.html  # 覆盖率报告
```

#### 测试执行步骤
```bash
# Step 1: 迁移测试套件
cp -r ~/98-ghosttyAI/cache/week2/TESTS/ ~/98-ghosttyAI/tests/week3/

# Step 2: 更新测试路径
cd tests/week3/
sed -i 's|cache/week2|tmux|g' integration_tests/*.c
sed -i 's|cache/week2|ghostty|g' integration_tests/*.c

# Step 3: 运行测试序列
# 构建验证
cd ~/98-ghosttyAI/tmux && make -f Makefile.libtmuxcore
cd ~/98-ghosttyAI/ghostty && zig build

# 功能测试
./run_integration_tests.sh
./run_performance_tests.sh
valgrind --leak-check=full ./test_suite

# Step 4: 生成报告
gcov *.c
genhtml coverage.info -o coverage_week3/
```

#### 测试检查清单
- [ ] libtmuxcore.so加载成功
- [ ] 基本tmux命令工作（new, split, kill）
- [ ] 性能基线对比（<5%退化）
- [ ] 内存泄漏检查（0泄漏）
- [ ] vim/neovim兼容性
- [ ] 覆盖率>85%

---

## 📋 资源快速索引

### 代码库位置
```
/Users/jqwang/98-ghosttyAI/
├── tmux/                 # 修改目标
├── ghostty/              # 集成目标
├── cache/week1/          # 第一周探索代码
├── cache/week2/          # 第二周验证代码（主要复用源）
├── docs/                 # 设计文档
└── 第三周-施工方案/      # 本周工作目录
```

### 关键文件大小参考
```
event_loop_backend.h   8KB   ✅ 直接复用
grid_operations_neon.c 45KB  ✅ 性能关键
callbacks.zig         31KB  ✅ FFI核心
ffi_safety.zig       15KB  ✅ 内存安全
测试套件              2.3MB  ✅ 91%覆盖
```

### 性能基线数据
```yaml
必须维持的指标:
  事件循环开销: <1%
  网格操作: >350k ops/s  
  FFI开销: <100ns
  内存/session: <10MB
  FPS: >60
```

---

## 🚨 紧急联系与升级路径

### 遇到问题时
1. **技术问题**: 查看cache/week2/的对应实现
2. **设计疑问**: 参考/docs/architecture-view/
3. **构建问题**: 使用cache/week2/Makefile模板
4. **测试失败**: 对比cache/week2/TESTS/expected_results/

### 升级到PM
- 性能退化>5%
- 核心功能破坏
- 编译失败超过2小时
- 发现设计缺陷

---

**记住**: 60%的代码已在第二周验证，专注于集成而非重新实现！