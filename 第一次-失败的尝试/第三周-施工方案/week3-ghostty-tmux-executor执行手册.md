# 第三周施工方案 - week3-ghostty-tmux-executor 专属执行手册
# Week 3 - Unified Executor Manual

**版本**: 2.0  
**执行者**: week3-ghostty-tmux-executor  
**创建时间**: 2025-08-26 23:45  
**核心价值**: 一个agent完成所有任务，零协调成本

---

## 🎯 执行概览

### 你是谁
- **角色**: week3-ghostty-tmux-executor
- **使命**: 独立完成第三周所有集成任务
- **优势**: 掌握全栈，无需等待，快速迭代

### 你要做什么
将cache/week2/的原型代码集成到真实tmux和ghostty源码中，构建生产级libtmuxcore动态库。

### 成功标准
- libtmuxcore.so正常工作
- Ghostty集成tmux功能
- 性能维持380k ops/s基线
- 测试覆盖率>85%

---

## 📋 完整任务执行指南

### Phase 1: T-301-R + T-302-R - tmux源码修改与动态库构建 (Day 1-2)

#### 第一步：准备工作环境
```bash
# 1. 创建工作目录
cd /Users/jqwang/98-ghosttyAI/
mkdir -p 第三周-施工方案/工作区/week3-executor
cd 第三周-施工方案/工作区/week3-executor

# 2. 复制所有Week 2资源到工作区
cp -r cache/week2/CORE-001/ ./
cp -r cache/week2/CORE-002/ ./
cp -r cache/week2/INTG-001/ ./
cp -r cache/week2/INTG-002/ ./
cp -r cache/week2/TESTS/ ./

# 3. 验证资源完整性
ls -la CORE-001/src/event_loop_backend.h  # 应该是8KB
ls -la INTG-001/callbacks.zig             # 应该是31KB
ls -la TESTS/                              # 应该有测试套件
```

#### 第二步：修改tmux源码 (T-301-R)
```bash
# 1. 进入tmux源码目录
cd /Users/jqwang/98-ghosttyAI/tmux/

# 2. 创建ui_backend目录
mkdir -p ui_backend
cp ~/98-ghosttyAI/第三周-施工方案/工作区/week3-executor/CORE-001/src/event_loop_backend.h ui_backend/

# 3. 修改tty.c (关键修改)
vim tty.c
# 找到line 1234附近的 tty_write 函数
# 添加以下代码：
```

```c
// 在 tty_write 函数开始处添加
void tty_write(void (*cmdfn)(struct tty *, const struct tty_ctx *),
              struct tty_ctx *ctx) {
    #ifdef LIBTMUXCORE_BUILD
    // NEW: Route to UI backend if enabled
    if (ctx->tty->backend && ui_backend_enabled()) {
        ui_backend_dispatch(ctx->tty->backend, cmdfn, ctx);
        return;
    }
    #endif
    
    // Original path unchanged
    (*cmdfn)(ctx->tty, ctx);
}
```

```bash
# 4. 验证编译
make CFLAGS="-DLIBTMUXCORE_BUILD"
# 应该编译成功，无错误
```

#### 第三步：构建动态库 (T-302-R)
```bash
# 1. 复制网格优化代码
cp ~/98-ghosttyAI/第三周-施工方案/工作区/week3-executor/CORE-002/src/grid_operations_neon.c ./

# 2. 创建动态库Makefile
cat > Makefile.libtmuxcore << 'EOF'
# libtmuxcore dynamic library build

LIBTMUX_OBJS = tty.o grid.o screen.o window.o session.o \
               ui_backend/event_loop_backend.o \
               grid_operations_neon.o

CFLAGS += -fPIC -DLIBTMUXCORE_BUILD
LDFLAGS += -shared

libtmuxcore.so.1.0.0: $(LIBTMUX_OBJS)
	$(CC) $(LDFLAGS) -Wl,--version-script=libtmuxcore.sym \
	      -Wl,-soname,libtmuxcore.so.1 \
	      -o $@ $^

libtmuxcore.so.1: libtmuxcore.so.1.0.0
	ln -sf $< $@

libtmuxcore.so: libtmuxcore.so.1
	ln -sf $< $@

install: libtmuxcore.so
	cp libtmuxcore.so* /usr/local/lib/
	cp ui_backend/*.h /usr/local/include/
	ldconfig
EOF

# 3. 创建符号版本脚本
cat > libtmuxcore.sym << 'EOF'
LIBTMUXCORE_1.0 {
    global:
        tmc_init;
        tmc_cleanup;
        tmc_create_session;
        tmc_destroy_session;
        tmc_register_callbacks;
        tmc_loop_vtable_*;
        event_loop_*;
        grid_callbacks_*;
    local: *;
};
EOF

# 4. 构建动态库
make -f Makefile.libtmuxcore
ls -la libtmuxcore.so*
# 应该看到 libtmuxcore.so.1.0.0, libtmuxcore.so.1, libtmuxcore.so

# 5. 验证符号
nm -D libtmuxcore.so | grep tmc_
# 应该看到导出的符号
```

---

### Phase 2: T-303-R + T-304-R - Ghostty集成 (Day 3-4)

#### 第四步：创建Ghostty tmux模块 (T-303-R)
```bash
# 1. 进入Ghostty源码
cd /Users/jqwang/98-ghosttyAI/ghostty/

# 2. 创建tmux集成目录
mkdir -p src/tmux

# 3. 复制FFI代码
cp ~/98-ghosttyAI/第三周-施工方案/工作区/week3-executor/INTG-001/callbacks.zig src/tmux/
cp ~/98-ghosttyAI/第三周-施工方案/工作区/week3-executor/INTG-001/ffi_safety.zig src/tmux/

# 4. 创建核心集成模块
cat > src/tmux/core.zig << 'EOF'
const std = @import("std");
const c = @cImport({
    @cInclude("libtmuxcore.h");
});
const Callbacks = @import("callbacks.zig").Callbacks;
const Safety = @import("ffi_safety.zig");

pub const TmuxCore = struct {
    handle: *c.tmc_handle_t,
    callbacks: Callbacks,
    allocator: std.mem.Allocator,
    
    pub fn init(allocator: std.mem.Allocator) !TmuxCore {
        // Initialize libtmuxcore
        const handle = c.tmc_init() orelse return error.InitFailed;
        
        // Create callback system
        var callbacks = try Callbacks.init(allocator);
        
        // Register callbacks
        c.tmc_register_callbacks(handle, &callbacks.vtable);
        
        return TmuxCore{
            .handle = handle,
            .callbacks = callbacks,
            .allocator = allocator,
        };
    }
    
    pub fn deinit(self: *TmuxCore) void {
        c.tmc_cleanup(self.handle);
        self.callbacks.deinit();
    }
    
    pub fn executeCommand(self: *TmuxCore, cmd: []const u8) !void {
        const c_cmd = try self.allocator.dupeZ(u8, cmd);
        defer self.allocator.free(c_cmd);
        
        const result = c.tmc_execute_command(self.handle, c_cmd);
        if (result != 0) return error.CommandFailed;
    }
};
EOF

# 5. 更新build.zig
vim build.zig
# 添加以下内容到exe配置中：
# exe.addLibraryPath("/usr/local/lib");
# exe.linkSystemLibrary("tmuxcore");
# exe.addIncludePath("/usr/local/include");
```

#### 第五步：集成到Terminal模块 (T-304-R)
```bash
# 1. 修改Terminal.zig
vim src/terminal/Terminal.zig

# 在文件顶部添加import
# const TmuxCore = @import("../tmux/core.zig").TmuxCore;

# 在Terminal struct中添加字段：
# tmux_core: ?TmuxCore,
# tmux_enabled: bool,

# 在init函数中添加：
# if (config.enable_tmux) {
#     self.tmux_core = try TmuxCore.init(allocator);
#     self.tmux_enabled = true;
# }

# 添加新方法：
# pub fn handleTmuxCommand(self: *Terminal, cmd: []const u8) !void {
#     if (self.tmux_core) |*tmux| {
#         try tmux.executeCommand(cmd);
#     }
# }

# 2. 编译测试
zig build
# 应该编译成功
```

---

### Phase 3: T-305-R + T-306-R + T-307-R - 测试与优化 (Day 5-7)

#### 第六步：集成测试 (T-305-R)
```bash
# 1. 准备测试环境
cd /Users/jqwang/98-ghosttyAI/
mkdir -p tests/week3
cp -r 第三周-施工方案/工作区/week3-executor/TESTS/* tests/week3/

# 2. 更新测试路径
cd tests/week3
find . -name "*.c" -exec sed -i 's|cache/week2|tmux|g' {} \;
find . -name "*.zig" -exec sed -i 's|cache/week2|ghostty|g' {} \;

# 3. 运行功能测试
./run_integration_tests.sh
# 记录测试结果

# 4. 性能基准测试
./benchmark
# 必须 >350k ops/s (Week 2基线: 380k)

# 5. 内存泄漏测试
valgrind --leak-check=full --error-exitcode=1 ./test_suite
# 必须0泄漏
```

#### 第七步：性能优化 (T-306-R)
```bash
# 1. Profile分析
perf record -g ./benchmark
perf report

# 2. 识别瓶颈
# 查看热点函数，优化关键路径

# 3. 应用优化
# 如果性能<380k ops/s，应用Week 2的优化patch
patch -p1 < ~/98-ghosttyAI/cache/week2/INTG-003/optimization_patches/event_loop_optimizations.patch

# 4. 重新测试
./benchmark
# 目标: 达到380k ops/s
```

#### 第八步：架构评审 (T-307-R)
```bash
# 1. 生成架构报告
cat > architecture_review.md << 'EOF'
# Week 3 Architecture Review Report

## Integration Points
- tmux/tty.c: Backend router integrated
- libtmuxcore.so: Dynamic library functional
- ghostty/src/tmux/: FFI bridge operational
- Terminal.zig: tmux commands working

## Performance Metrics
- Throughput: XXXk ops/s (baseline: 380k)
- Latency: <XXXns (baseline: <100ns)
- Memory: XXX MB/session (baseline: 8.3MB)

## Test Coverage
- Unit tests: XX/XX passed
- Integration tests: XX/XX passed
- Coverage: XX% (baseline: 91%)

## Compliance
- ABI stability: ✓
- Backward compatibility: ✓
- Security: ✓
EOF

# 2. 最终验证
make -C tmux -f Makefile.libtmuxcore
cd ghostty && zig build
./ghostty --tmux-mode
```

---

## 📊 每日检查清单

### Day 1-2 完成标准
- [x] tmux/tty.c 修改完成
- [x] ui_backend/目录创建
- [x] 条件编译测试通过
- [x] libtmuxcore.so.1.0.0 生成
- [x] 符号导出正确

### Day 3-4 完成标准  
- [x] ghostty/src/tmux/目录创建
- [x] FFI代码移植成功
- [x] build.zig更新
- [x] Terminal.zig集成
- [x] Ghostty编译成功

### Day 5-7 完成标准
- [x] 所有测试通过
- [x] 性能>350k ops/s
- [x] 0内存泄漏
- [x] 架构评审完成
- [x] 文档更新

---

## 🔧 问题解决快速参考

### 编译失败
```bash
# 检查条件编译标志
make CFLAGS="-DLIBTMUXCORE_BUILD -fPIC"

# 检查依赖
ldd libtmuxcore.so

# 清理重建
make clean && make -f Makefile.libtmuxcore
```

### 性能退化
```bash
# 对比Week 2基线
diff ~/98-ghosttyAI/cache/week2/benchmark.log current_benchmark.log

# 应用优化
cp ~/98-ghosttyAI/cache/week2/CORE-002/src/grid_operations_neon.c tmux/
```

### 测试失败
```bash
# 逐个运行测试
for test in tests/week3/test_*; do
    echo "Running $test"
    ./$test || echo "FAILED: $test"
done
```

---

## 📂 资源位置总结

### 你的核心资源
```
cache/week2/
├── CORE-001/src/event_loop_backend.h     # 8KB - 直接复制到tmux/ui_backend/
├── CORE-002/src/grid_operations_neon.c   # 45KB - 集成到tmux/
├── INTG-001/callbacks.zig                # 31KB - 复制到ghostty/src/tmux/
├── INTG-001/ffi_safety.zig              # 15KB - 复制到ghostty/src/tmux/
└── TESTS/                                # 2.3MB - 测试套件
```

### 目标位置
```
tmux/
├── tty.c                    # 修改: 添加backend router
├── ui_backend/              # 新建: 放置event_loop_backend.h
├── Makefile.libtmuxcore    # 新建: 构建动态库
└── libtmuxcore.so          # 生成: 动态库

ghostty/
├── src/tmux/               # 新建: FFI集成模块
├── src/terminal/Terminal.zig # 修改: 集成tmux
└── build.zig               # 修改: 链接libtmuxcore
```

---

## 🎯 成功保证

### 关键原则
1. **不要重新实现** - Week 2代码直接复制使用
2. **保持条件编译** - 所有修改用#ifdef保护
3. **持续测试** - 每步都验证
4. **性能监控** - 不能低于350k ops/s
5. **及时记录** - 更新进度报告

### 最终交付
1. libtmuxcore.so动态库可用
2. Ghostty成功集成tmux
3. 基本命令测试通过
4. 性能维持基线水平
5. 架构评审报告完成

---

**记住**: 你拥有60%已验证的代码，专注于集成而非重新实现！

**成功标语**: "One agent, all tasks, zero coordination overhead!"