# OPS-001 任务清单 - DevOps工程师
## 第一周：构建系统和CI基础设施

### 任务概览
**角色**：CI/CD Engineer  
**任务ID**：Build System Setup  
**预计工期**：2天 (持续支持)  
**开始时间**：2025-01-06 (周一)  
**主要交付**：2025-01-07 (周二)  

### 主要任务：建立完整的构建和测试基础设施

#### 输入 (Inputs)
1. **项目结构**
   - `/Users/jqwang/98-ghosttyAI/tmux/` - tmux 源码
   - `/Users/jqwang/98-ghosttyAI/ghostty/` - Ghostty 源码
   - 各开发人员的代码输出

2. **构建需求**
   - macOS ARM64 (M4) 优化
   - 支持增量构建
   - 自动化测试集成
   - PlantUML 文档生成

3. **参考文档**
   - DevOps agent 定义文件
   - 现有的 Makefile 示例

#### 输出 (Outputs)
1. **Makefile** - 主构建文件
   ```makefile
   # 核心目标
   all: build-tmux build-ghostty build-libtmuxcore
   
   # 构建目标
   build-tmux:          # 构建传统 tmux
   build-ghostty:       # 构建 Ghostty
   build-libtmuxcore:   # 构建库 (Week 1 准备)
   
   # 测试目标
   test:               # 运行所有测试
   test-unit:          # 单元测试
   test-integration:   # 集成测试
   coverage:           # 生成覆盖率报告
   
   # PlantUML 目标
   validate-puml:      # 验证所有 PUML 文件
   generate-diagrams:  # 生成 SVG 图表
   
   # 清理目标
   clean:              # 清理构建产物
   distclean:          # 完全清理
   ```

2. **build/** - 构建输出目录结构
   ```
   build/
   ├── tmux/           # tmux 构建输出
   ├── ghostty/        # Ghostty 构建输出
   ├── libtmuxcore/    # 库构建输出
   ├── tests/          # 测试结果
   ├── coverage/       # 覆盖率报告
   └── logs/           # 构建日志
   ```

3. **.github/workflows/ci.yml** - CI 配置
   ```yaml
   name: CI
   on: [push, pull_request]
   jobs:
     build:
       runs-on: macos-latest
       steps:
         - uses: actions/checkout@v2
         - name: Build
           run: make all
         - name: Test
           run: make test
         - name: Coverage
           run: make coverage
   ```

4. **scripts/** - 构建辅助脚本
   ```
   scripts/
   ├── check-deps.sh      # 依赖检查
   ├── setup-env.sh       # 环境设置
   ├── run-tests.sh       # 测试运行器
   └── generate-report.sh # 报告生成
   ```

#### 具体步骤 (Steps)

**周一：基础构建系统**
1. **9:00-10:00** - 环境评估和依赖检查
   ```bash
   #!/bin/bash
   # check-deps.sh
   check_command() {
       command -v $1 >/dev/null 2>&1 || {
           echo "Installing $1..."
           brew install $1
       }
   }
   
   check_command pkg-config
   check_command libevent
   check_command zig
   check_command plantuml
   ```

2. **10:00-12:00** - 创建主 Makefile
3. **13:00-15:00** - 实现 build-tmux 目标
4. **15:00-17:00** - 实现 build-ghostty 目标
5. **17:00-18:00** - 测试构建系统

**周二：测试和文档集成**
1. **9:00-11:00** - 集成测试框架
2. **11:00-12:00** - PlantUML 集成
3. **13:00-15:00** - CI 配置
4. **15:00-16:00** - 性能优化（并行构建）
5. **16:00-17:00** - 文档编写
6. **17:00-18:00** - 交付和培训

**持续支持（周三-周五）**
- 每日构建验证
- 解决构建问题
- 优化构建时间
- 支持新增代码集成

#### Makefile 核心实现
```makefile
# Ghostty × tmux Integration Build System
# DevOps Engineer: OPS-001

# === 配置 ===
PROJECT_ROOT := /Users/jqwang/98-ghosttyAI
TMUX_SRC := $(PROJECT_ROOT)/tmux
GHOSTTY_SRC := $(PROJECT_ROOT)/ghostty
BUILD_DIR := $(PROJECT_ROOT)/build

# 架构检查
UNAME_M := $(shell uname -m)
ifneq ($(UNAME_M),arm64)
    $(error Requires macOS ARM64)
endif

# 编译器配置
CC := clang
CFLAGS := -arch arm64 -O2 -mtune=native -Wall -Wextra
LDFLAGS := -arch arm64

# === 主目标 ===
.PHONY: all
all: deps build-tmux build-ghostty validate-puml

# === 构建目标 ===
.PHONY: build-tmux
build-tmux: deps-tmux
	@echo "[BUILD] Building tmux for ARM64..."
	@mkdir -p $(BUILD_DIR)/tmux
	cd $(TMUX_SRC) && \
	./autogen.sh && \
	./configure --prefix=$(BUILD_DIR)/tmux \
	            CFLAGS="$(CFLAGS)" \
	            LDFLAGS="$(LDFLAGS)" && \
	$(MAKE) -j8 && \
	$(MAKE) install
	@echo "[SUCCESS] tmux built: $(BUILD_DIR)/tmux/bin/tmux"

.PHONY: build-ghostty
build-ghostty: deps-ghostty
	@echo "[BUILD] Building Ghostty for ARM64..."
	@mkdir -p $(BUILD_DIR)/ghostty
	cd $(GHOSTTY_SRC) && \
	zig build -Doptimize=ReleaseFast \
	          -Dtarget=aarch64-macos \
	          --prefix $(BUILD_DIR)/ghostty
	@echo "[SUCCESS] Ghostty built: $(BUILD_DIR)/ghostty/"

# === 测试目标 ===
.PHONY: test
test: test-unit test-integration

.PHONY: test-unit
test-unit:
	@echo "[TEST] Running unit tests..."
	@$(BUILD_DIR)/tests/run_unit_tests

.PHONY: test-integration
test-integration:
	@echo "[TEST] Running integration tests..."
	@scripts/run-tests.sh integration

.PHONY: coverage
coverage:
	@echo "[COVERAGE] Generating coverage report..."
	@mkdir -p $(BUILD_DIR)/coverage
	@lcov --capture --directory . --output-file $(BUILD_DIR)/coverage/coverage.info
	@genhtml $(BUILD_DIR)/coverage/coverage.info --output-directory $(BUILD_DIR)/coverage/html

# === PlantUML 目标 ===
.PHONY: validate-puml
validate-puml:
	@echo "[PUML] Validating PlantUML files..."
	@find docs -name "*.puml" -exec plantuml -checkonly {} \;

.PHONY: generate-diagrams
generate-diagrams:
	@echo "[PUML] Generating diagrams..."
	@find docs -name "*.puml" -exec plantuml -tsvg {} \;

# === 依赖管理 ===
.PHONY: deps
deps: deps-tmux deps-ghostty deps-dev

.PHONY: deps-tmux
deps-tmux:
	@scripts/check-deps.sh tmux

.PHONY: deps-ghostty
deps-ghostty:
	@scripts/check-deps.sh ghostty

.PHONY: deps-dev
deps-dev:
	@scripts/check-deps.sh dev

# === 清理 ===
.PHONY: clean
clean:
	@echo "[CLEAN] Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)

.PHONY: distclean
distclean: clean
	@echo "[CLEAN] Deep clean..."
	@find . -name "*.o" -delete
	@find . -name "*.a" -delete
```

#### 验收标准 (Acceptance Criteria)
- [ ] make build-tmux 成功构建
- [ ] make build-ghostty 成功构建  
- [ ] 构建时间 <2分钟（增量）
- [ ] 测试集成正常工作
- [ ] PlantUML 验证和生成工作
- [ ] CI 配置就绪
- [ ] 文档完整

#### 性能目标 (Performance Goals)
- 全量构建：<5分钟
- 增量构建：<30秒
- 测试运行：<2分钟
- 并行度：利用所有 CPU 核心

#### 协作要求 (Collaboration)
- **支持所有开发**：解决构建问题
- **协作 QA**：集成测试框架
- **每日验证**：确保主分支可构建

#### 监控和报告 (Monitoring)
```bash
# 构建时间统计
time make all

# 构建大小分析
du -sh build/*

# 依赖树分析
otool -L build/tmux/bin/tmux
```

#### 每日汇报 (Daily Report)
```
STATUS [OPS-001] [日期]
构建状态：[成功/失败]
构建时间：[分钟]
测试通过率：[百分比]
问题修复：[数量]
优化：[具体项]
支持请求：[处理数]
```