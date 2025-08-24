PROJECT: Ghostty × tmux（libtmuxcore）完全内嵌集成

GOAL: 将 tmux 核心以内嵌库 libtmuxcore 的形式嵌入 Ghostty，弃用 VT/TTY 输出，改用结构化网格与事件回调驱动 Ghostty 渲染与输入，同时保持 tmux 语义与性能优势。

CONSTRAINTS:
	•	复用现有源码（tmux: C；Ghostty: Zig），最小侵入改动（主要集中在 tty_write 输出链与事件循环接入）。
	•	保持 tmux 语义/命令/选项与现有行为一致；可同时构建传统 tmux 可执行文件。
	•	稳定 C ABI v1（不暴露 Zig 类型；明确线程与生命周期约束；向后兼容策略）。
	•	UI 职责前移：边框/状态/分割线与可视化统一由 Ghostty 绘制；core 不再输出状态行/边框字符。
	•	事件循环通过 loop vtable 嵌入宿主（Ghostty）；Linux/macOS 先行，Windows 适配可选。
	•	许可证与发布合规（tmux ISC；按静态/动态链接策略评估）。
	•	每 30 分钟小步提交（功能最小闭环优先），为所有新增功能编写测试，遵循现有代码风格与工具链。

DELIVERABLES:
	1.	libtmuxcore（C 库）
	•	稳定头文件与符号：libtmuxcore.h（事件回调 on_grid/on_layout/on_copy_mode/...、输入 tmc_send_*、循环 tmc_loop_vtable、命令与选项接口、快照 API）。
    参考草案：example/libtmuxcore.h
	•	UI 后端与挂接：为 tty_write(tty_cmd_*) 引入后端路由；实现 Ghostty 后端 将脏区合并为 grid spans 回调；保留传统 TTY 后端。
	•	事件循环桥：用 loop vtable 替代 libevent 的 fd/timer 注册（不改核心逻辑），支持宿主线程唤醒与 post。
	•	构建与包装：生成 .so/.dylib/.a，导出符号版本脚本；保留 tmux 传统构建目标（选择后端）。
	•	文档：嵌入指南、ABI 版本与兼容策略、示例代码与最小样例。
	2.	Ghostty 集成（Zig）
	•	FFI 粘合层：Zig 侧桥接回调到 Ghostty 的终端缓冲/渲染与分割树；输入路由至 tmc_send_keys/text/mouse；支持会话 attach/resize。
    参考样例：example/ghostty_tmx_bridge.zig
	•	渲染与布局：用结构化回调驱动 GPU 合成；Ghostty 负责边框/标签/选区/滚动条；copy-mode 高亮与搜索统一可视化。
	•	配置与开关：-Dtmuxcore=true 特性开关，启动参数/配置项；崩溃/重启的安全分离与可重连。
	•	最小演示：启动嵌入会话，自动建立左右/上下分割，展示 60–120Hz 合成与低延迟输入。
	3.	质量保证与迁移
	•	测试套件：
	•	单测：网格 diff、宽字符/emoji/双宽、滚动边界、属性合并；
	•	集成：vim/neovim、htop、less、git log --graph；
	•	性能：大吞吐与全屏重绘；
	•	Fuzz：输入/屏幕写入状态机。
	•	覆盖新增后端、循环桥与 FFI 路径。
	•	CI & 指标：多平台构建、ABI/符号检查、覆盖率阈值、性能基线、结构化日志与指标（回调频率、合成时延、背压丢帧）。
	•	兼容与迁移：内嵌模式下自动禁用 tmux 状态行/边框；提供回退到传统模式的配置；安全注意事项（socket 路径、权限、剪贴板策略）。
	•	参考计划：详细切割点、周度里程碑与测试策略见说明文档。
    计划文档：example/integration_plan.md